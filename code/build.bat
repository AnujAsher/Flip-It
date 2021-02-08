@echo off

set result=0

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl /nologo /GR- /GS- /EHa- /WX /W4 /wd4204 /wd4244 /MD /O2 /FC /Fe:flip_it.exe ../code/flip_it.c /link /subsystem:windows,5.2 /ENTRY:mainCRTStartup ../raylib/lib/raylib.lib user32.lib gdi32.lib winmm.lib shell32.lib
set /a result=%result%+%errorlevel%

popd
exit /b %result%
