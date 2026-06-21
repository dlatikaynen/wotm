#include "inc/synth.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace
{
    constexpr double TAU = 6.283185307179586;
    constexpr int CHANNELS = 2;
    constexpr int MAX_VOICES = 16;
    constexpr double MASTER_GAIN = 0.22;
    constexpr double ATTACK_S = 0.005;
    constexpr double DECAY_S = 0.080;
    constexpr double SUSTAIN_LVL = 0.70;
    constexpr double RELEASE_S = 0.180;

    enum class CmdType : std::uint8_t { NoteOn, NoteOff, AllOff, SetWave };

    struct Command
    {
        CmdType type;
        int note;
        float velocity;
        synth::Waveform wave;
    };

    constexpr int QUEUE_CAP = 2 ^ 8;
    constexpr int QUEUE_MASK = QUEUE_CAP - 1;

    struct Voice
    {
        enum class Stage { Idle, Attack, Decay, Sustain, Release };

        bool active = false;
        int note = -1;
        double phase = 0.0; // 0..1
        double phaseInc = 0.0; // cycles/sample
        float velocity = 0.0f;
        double env = 0.0;
        Stage stage = Stage::Idle;
        std::uint64_t order = 0;
        std::uint32_t rng = 0x9e3779b9u;
    };

    struct SynthState
    {
        SDL_AudioStream* stream = nullptr;
        double sampleRate = 44100.0;
        double attackRate = 0.0;
        double decayRate = 0.0;
        double releaseRate = 0.0;
        Voice voices[MAX_VOICES];
        synth::Waveform waveform = synth::Waveform::Saw;
        std::uint64_t voiceClock = 0;
        Command queue[QUEUE_CAP];
        std::atomic<std::uint32_t> head{0}; // on synth thread
        std::atomic<std::uint32_t> tail{0}; // on main thread
    };

    SynthState g;

    void push_command(const Command& cmd)
    {
        const std::uint32_t tail = g.tail.load(std::memory_order_relaxed);
        const std::uint32_t head = g.head.load(std::memory_order_acquire);
        if (tail - head >= QUEUE_CAP)
        {
            return;
        }

        g.queue[tail & QUEUE_MASK] = cmd;
        g.tail.store(tail + 1, std::memory_order_release);
    }

    double poly_blep(double t, double dt)
    {
        if (dt <= 0.0)
        {
            return 0.0;
        }

        if (t < dt)
        {
            t /= dt;

            return t + t - t * t - 1.0;
        }

        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;

            return t * t + t + t + 1.0;
        }

        return 0.0;
    }

    float voice_noise(Voice& v)
    {
        std::uint32_t x = v.rng;

        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        v.rng = x;

        return static_cast<float>(x) / 2147483648.0f - 1.0f;
    }

    double oscillator(Voice& v)
    {
        const double t  = v.phase;
        const double dt = v.phaseInc;

        switch (g.waveform)
        {
            case synth::Waveform::Sine:
                return std::sin(TAU * t);

            case synth::Waveform::Triangle:
                return 1.0 - 4.0 * std::fabs(t - 0.5);

            case synth::Waveform::Saw:
            {
                double s = 2.0 * t - 1.0;

                s -= poly_blep(t, dt);

                return s;
            }

            case synth::Waveform::Square:
            {
                double s = t < 0.5 ? 1.0 : -1.0;

                s += poly_blep(t, dt);

                double t2 = t + 0.5;

                if (t2 >= 1.0)
                {
                    t2 -= 1.0;
                }

                s -= poly_blep(t2, dt);

                return s;
            }

            case synth::Waveform::Noise:
                return voice_noise(v);

            default:
                return 0.0;
        }
    }

    void step_envelope(Voice& v)
    {
        switch (v.stage)
        {
            case Voice::Stage::Attack:
                v.env += g.attackRate;
                if (v.env >= 1.0)
                {
                    v.env   = 1.0;
                    v.stage = Voice::Stage::Decay;
                }

                break;

            case Voice::Stage::Decay:
                v.env -= g.decayRate;
                if (v.env <= SUSTAIN_LVL)
                {
                    v.env   = SUSTAIN_LVL;
                    v.stage = Voice::Stage::Sustain;
                }

                break;

            case Voice::Stage::Release:
                v.env -= g.releaseRate;
                if (v.env <= 0.0)
                {
                    v.env    = 0.0;
                    v.stage  = Voice::Stage::Idle;
                    v.active = false;
                }

                break;

            case Voice::Stage::Sustain:
            default:
                break;
        }
    }

    void do_note_on(int note, float velocity)
    {
        Voice* chosen = nullptr;
        for (Voice& v : g.voices)
        {
            if (!v.active)
            {
                chosen = &v;

                break;
            }
        }

        if (chosen == nullptr)
        {
            std::uint64_t oldest = UINT64_MAX;
            for (Voice& v : g.voices)
            {
                if (v.order < oldest)
                {
                    oldest = v.order;
                    chosen = &v;
                }
            }
        }

        const double freq = 440.0 * std::pow(2.0, (note - 69) / 12.0);

        chosen->note = note;
        chosen->phase = 0.0;
        chosen->phaseInc = freq / g.sampleRate;
        chosen->velocity = velocity;
        chosen->env = 0.0;
        chosen->rng = 0x9e3779b9u + static_cast<std::uint32_t>(note) * 2654435761u + static_cast<std::uint32_t>(g.voiceClock);
        chosen->stage = Voice::Stage::Attack;
        chosen->order = ++g.voiceClock;
        chosen->active = true;
    }

    void do_note_off(int note)
    {
        for (Voice& v : g.voices)
        {
            if (v.active && v.note == note && v.stage != Voice::Stage::Release)
            {
                v.stage = Voice::Stage::Release;
            }
        }
    }

    void do_all_off()
    {
        for (Voice& v : g.voices)
        {
            if (v.active)
            {
                v.stage = Voice::Stage::Release;
            }
        }
    }

    void drain_commands()
    {
        const std::uint32_t tail = g.tail.load(std::memory_order_acquire);
        std::uint32_t head = g.head.load(std::memory_order_relaxed);

        while (head != tail)
        {
            const Command& cmd = g.queue[head & QUEUE_MASK];

            switch (cmd.type)
            {
                case CmdType::NoteOn:
                    do_note_on(cmd.note, cmd.velocity);
                    break;

                case CmdType::NoteOff:
                    do_note_off(cmd.note);
                    break;

                case CmdType::AllOff:
                    do_all_off();
                    break;

                case CmdType::SetWave:
                    g.waveform = cmd.wave;
                    break;
            }

            ++head;
        }

        g.head.store(head, std::memory_order_release);
    }

    void render_block(float* out, int frames)
    {
        for (int f = 0; f < frames; ++f)
        {
            double mix = 0.0;
            for (Voice& v : g.voices)
            {
                if (!v.active)
                {
                    continue;
                }

                step_envelope(v);
                mix += oscillator(v) * v.env * v.velocity;
                v.phase += v.phaseInc;
                if (v.phase >= 1.0)
                {
                    v.phase -= 1.0;
                }
            }

            // the sheafification of g# :)
            const float s = static_cast<float>(std::tanh(mix * MASTER_GAIN));
            
            out[f * CHANNELS + 0] = s;
            out[f * CHANNELS + 1] = s;
        }
    }

    void SDLCALL audio_callback(void* userData, SDL_AudioStream* stream, int additional, int total)
    {
        (void)userData;
        (void)total;

        drain_commands();

        constexpr int kMaxFrames = 512;
        static float buffer[kMaxFrames * CHANNELS];
        const int frameBytes = CHANNELS * static_cast<int>(sizeof(float));
        int framesNeeded = additional / frameBytes;

        while (framesNeeded > 0)
        {
            const int n = framesNeeded < kMaxFrames ? framesNeeded : kMaxFrames;

            render_block(buffer, n);
            SDL_PutAudioStreamData(stream, buffer, n * frameBytes);
            framesNeeded -= n;
        }
    }
}

namespace synth
{
    bool init(int sampleRate)
    {
        if (g.stream != nullptr)
        {
            return true;
        }

        if (!SDL_WasInit(SDL_INIT_AUDIO))
        {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
            {
                SDL_Log("[synth] failed to initialize: %s", SDL_GetError());

                return false;
            }
        }

        g.sampleRate = static_cast<double>(sampleRate);
        g.attackRate = 1.0 / (ATTACK_S  * g.sampleRate);
        g.decayRate = (1.0 - SUSTAIN_LVL) / (DECAY_S * g.sampleRate);
        g.releaseRate = SUSTAIN_LVL / (RELEASE_S * g.sampleRate);

        SDL_AudioSpec spec{};
        spec.freq     = sampleRate;
        spec.format   = SDL_AUDIO_F32;
        spec.channels = CHANNELS;

        g.stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            &spec,
            audio_callback,
            nullptr
        );

        if (g.stream == nullptr)
        {
            SDL_Log("[synth] failed to open stream: %s", SDL_GetError());

            return false;
        }

        SDL_ResumeAudioStreamDevice(g.stream);
        SDL_Log("[synth] ready at %d hz", sampleRate);

        return true;
    }

    void shutdown()
    {
        if (g.stream != nullptr)
        {
            SDL_DestroyAudioStream(g.stream);
            g.stream = nullptr;
        }
    }

    void note_on(int midiNote, float velocity)
    {
        if (g.stream == nullptr)
        {
            return;
        }

        push_command(Command{ CmdType::NoteOn, midiNote, velocity, Waveform::Sine });
    }

    void note_off(int midiNote)
    {
        if (g.stream == nullptr)
        {
            return;
        }

        push_command(Command{ CmdType::NoteOff, midiNote, 0.0f, Waveform::Sine });
    }

    void all_notes_off()
    {
        if (g.stream == nullptr)
        {
            return;
        }

        push_command(Command{ CmdType::AllOff, 0, 0.0f, Waveform::Sine });
    }

    void set_waveform(Waveform wave)
    {
        if (g.stream == nullptr)
        {
            return;
        }

        push_command(Command{ CmdType::SetWave, 0, 0.0f, wave });
    }
}
