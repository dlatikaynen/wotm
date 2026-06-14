@echo off
setlocal enabledelayedexpansion

REM nob for wasm

cd /d "%~dp0"

if not exist build-em\build.ninja (
    echo [nob] configuring build-em ...
    call emcmake cmake -S . -B build-em || goto :fatal
)

echo [nob] watching src\, CMakeLists.txt, index.html ^(Ctrl+C to stop^)
set "LAST="

:loop
REM newest modification time in ticks, across all watched inputs
for /f %%T in ('powershell -NoProfile -Command "(@(Get-ChildItem -File -Recurse src; Get-Item CMakeLists.txt,index.html) | Measure-Object -Maximum -Property LastWriteTimeUtc).Maximum.Ticks"') do set "NEWEST=%%T"

if not "!NEWEST!"=="!LAST!" (
    set "LAST=!NEWEST!"
    echo.
    echo [nob] change detected, re-nobbing ...
    cmake --build build-em
    if errorlevel 1 (
        echo [nob] FAILED, fix and save to retry
    ) else (
        copy /y index.html build-em\ >nul
        echo [nob] OK, browser will auto-reload
    )
)

timeout /t 1 /nobreak >nul
goto :loop

:fatal
echo [nob] emcmake configure failed
exit /b 1
