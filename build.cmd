@echo off
setlocal enabledelayedexpansion

REM nob for wasm

cd /d "%~dp0"

if not exist build-em\build.ninja (
    echo [nob] configuring build-em ...
    call emcmake cmake -S . -B build-em || goto :fatal
)

echo [nob] watching src\, data\, CMakeLists.txt, index.html ^(Ctrl+C to stop^)
set "LAST="
set "LAST_DATA="

:loop
for /f %%T in ('powershell -NoProfile -Command "(@(Get-ChildItem -File -Recurse src,data; Get-Item CMakeLists.txt,index.html) | Measure-Object -Maximum -Property LastWriteTimeUtc).Maximum.Ticks"') do set "NEWEST=%%T"
for /f %%T in ('powershell -NoProfile -Command "(Get-ChildItem -File -Recurse data | Measure-Object -Maximum -Property LastWriteTimeUtc).Maximum.Ticks"') do set "DATA_NEWEST=%%T"

if not "!NEWEST!"=="!LAST!" (
    set "LAST=!NEWEST!"
    echo.
    echo [nob] change detected, re-nobbing ...
    if not "!DATA_NEWEST!"=="!LAST_DATA!" (
        echo [nob] data\ changed, forcing relink to rebuild wotm.data
        del /q build-em\wotm.html >nul 2>nul
    )
    set "LAST_DATA=!DATA_NEWEST!"
    cmake --build build-em
    if errorlevel 1 (
        echo [nob] FAILED, fix and save to retry
    ) else (
        copy /y index.html build-em\ >nul
        >"build-em\reload-stamp.txt" echo !NEWEST!
        echo [nob] OK, browser will auto-reload
    )
)

timeout /t 1 /nobreak >nul
goto :loop

:fatal
echo [nob] emcmake configure failed
exit /b 1
