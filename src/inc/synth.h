#pragma once

/// successor to blooDot's moosic toolkit, an STK inspired polyphonic oscillator
namespace synth
{
    enum class Waveform : int
    {
        Sine,
        Triangle,
        Saw,
        Square,
        Noise,
        COUNT
    };

    bool init(int sampleRate = 44100);
    void set_waveform(Waveform wave);
    void note_on(int midiNote, float velocity = 0.8f);
    void note_off(int midiNote);
    void all_notes_off();
    void shutdown();
}
