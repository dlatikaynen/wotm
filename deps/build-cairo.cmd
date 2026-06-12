@echo off
REM ===========================================================================
REM Build cairo (+ a compatible pixman) as wasm for the wotm project and install
REM into C:\en\Emscripten\Release, next to SDL3/freetype.
REM
REM Why this exists: cairo has no CMake build, so FetchContent in CMakeLists.txt
REM cannot produce a `cairo` target. Cairo's native build system is Meson, and
REM cairo 1.18 needs pixman >= 0.40 (the Release tree shipped 0.33.3), so we build
REM both here against the Emscripten toolchain via deps\emscripten-cross.ini.
REM
REM Prerequisites (one-time):
REM   pip install meson pkgconf      (ninja + emsdk must already be installed)
REM Run from this deps\ directory:  build-cairo.cmd
REM ===========================================================================
setlocal

set REL=C:\en\Emscripten\Release
set DEPS=%~dp0
set PIXMAN_SRC=C:\en\projects\pixman-upstream
set CAIRO_SRC=C:\en\projects\cairo
set CROSS=%DEPS%emscripten-cross.ini

REM meson + pkgconf land in the per-user Python Scripts dir; put it on PATH.
for /f "delims=" %%P in ('python -c "import sysconfig,os;print(os.path.join(sysconfig.get_path('scripts','nt_user'),''))"') do set PYSCRIPTS=%%P
set PATH=%PATH%;%PYSCRIPTS%
set PKG_CONFIG_PATH=%DEPS%pkgconfig

REM --- pixman 0.44.x ---------------------------------------------------------
if not exist "%PIXMAN_SRC%\.git" (
    git clone --depth 1 --branch pixman-0.44.2 https://gitlab.freedesktop.org/pixman/pixman.git "%PIXMAN_SRC%" || goto :err
)
if exist "%PIXMAN_SRC%\build-em" rmdir /s /q "%PIXMAN_SRC%\build-em"
meson setup "%PIXMAN_SRC%\build-em" --cross-file "%CROSS%" --prefix /usr ^
    --default-library static --buildtype release ^
    -Dtests=disabled -Ddemos=disabled -Dgtk=disabled -Dlibpng=disabled || goto :err
meson compile -C "%PIXMAN_SRC%\build-em" || goto :err
copy /y "%PIXMAN_SRC%\build-em\pixman\libpixman-1.a"      "%REL%\lib\libpixman-1.a" || goto :err
copy /y "%PIXMAN_SRC%\pixman\pixman.h"                    "%REL%\include\pixman-1\pixman.h" || goto :err
copy /y "%PIXMAN_SRC%\build-em\pixman\pixman-version.h"   "%REL%\include\pixman-1\pixman-version.h" || goto :err

REM --- cairo 1.18.x ----------------------------------------------------------
if not exist "%CAIRO_SRC%\.git" (
    git clone --depth 1 --branch 1.18.4 https://gitlab.freedesktop.org/cairo/cairo.git "%CAIRO_SRC%" || goto :err
)
if exist "%CAIRO_SRC%\build-em" rmdir /s /q "%CAIRO_SRC%\build-em"
meson setup "%CAIRO_SRC%\build-em" --cross-file "%CROSS%" --prefix /usr ^
    --default-library static --buildtype release ^
    -Dfreetype=enabled -Dfontconfig=disabled -Dpng=disabled -Dzlib=disabled ^
    -Dxlib=disabled -Dxcb=disabled -Dxlib-xcb=disabled ^
    -Dquartz=disabled -Ddwrite=disabled -Dtee=disabled -Dspectre=disabled ^
    -Dlzo=disabled -Dglib=disabled -Dgtk2-utils=disabled ^
    -Dsymbol-lookup=disabled -Dtests=disabled || goto :err
meson compile -C "%CAIRO_SRC%\build-em" || goto :err
if exist "%CAIRO_SRC%\stage" rmdir /s /q "%CAIRO_SRC%\stage"
meson install -C "%CAIRO_SRC%\build-em" --destdir "%CAIRO_SRC%\stage" || goto :err
if not exist "%REL%\include\cairo" mkdir "%REL%\include\cairo"
copy /y "%CAIRO_SRC%\stage\usr\lib\libcairo.a"        "%REL%\lib\libcairo.a" || goto :err
copy /y "%CAIRO_SRC%\stage\usr\include\cairo\*.h"     "%REL%\include\cairo\" || goto :err

echo.
echo === cairo + pixman installed into %REL% ===
exit /b 0

:err
echo BUILD FAILED (errorlevel %errorlevel%)
exit /b 1
