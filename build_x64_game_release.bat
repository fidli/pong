@echo off
ctime -begin local/release.ctime
rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
rem call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\amd64\vcvars64.bat"
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cls

set FILES="%cd%\sources\windows64_main.cpp"
set LIBS="Opengl32.lib" "Kernel32.lib" "Advapi32.lib" "Shell32.lib" "User32.lib" "Gdi32.lib" "Dwmapi.lib" "Dsound.lib" "Dinput8.lib" "dxguid.lib"
set BASELIB=""%cd%\baselib""
set LIBDIR=""%cd%\lib""
set RULESET=""%cd%\ruleset.ruleset""
pushd build
del *.obj 2> NUL
del *.h 2> NUL
for %%f in (..\sources\opengl_shaders\*) do xxd -i %%f >> shaders.h
for %%f in (..\baselib\opengl_shaders\*) do xxd -i %%f >> shaders.h
rem CANNOT USE SOME C++ FEATURES, std lib is ripped off (https://hero.handmade.network/forums/code-discussion/t/94)
rem for release: /w24061 - full switches
call cl.exe /DRELEASE /W4 /nologo /analyze:ruleset %RULESET% /WX /EHa- /GS- /O2 /Oi- /Gs99999999 /GR- /FI"%cd%\shaders.h" /I %BASELIB% /I %LIBDIR% /Fe"game_release.exe" /Zc:threadSafeInit- %FILES%  /link /INCREMENTAL:NO /NODEFAULTLIB /SUBSYSTEM:CONSOLE %LIBS% /STACK:0x1000000,0x1000000
POPD
ctime -end local/release.ctime
