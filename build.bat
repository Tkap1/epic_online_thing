
@echo off

cls

if NOT defined VSCMD_ARG_TGT_ARCH (
	@REM Replace with your path
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

if not exist build\NUL mkdir build

set comp=-nologo -std:c11 -W4 -wd4505 -FC -I ../../my_libs -Gm- -GR- -EHa- -wd 4324 -wd 4127 -D_CRT_SECURE_NO_WARNINGS
set linker=user32.lib Shell32.lib -INCREMENTAL:NO "..\..\enet\enet64.lib" Ws2_32.lib Winmm.lib

set debug=2
if %debug%==0 (
	set comp=%comp% -O2 -MT
)
if %debug%==1 (
	set comp=%comp% -O2 -Dm_debug -MTd
)
if %debug%==2 (
	set comp=%comp% -Od -Dm_debug -Zi -MTd
)

taskkill /IM "client.exe" > NUL 2> NUL
taskkill /IM "server.exe" > NUL 2> NUL

pushd build
	stamp_timer.exe start
	cl ..\src\client.c %comp% -Dm_app -link %linker% gdi32.lib opengl32.lib Xinput.lib
	cl ..\src\server.c %comp% -link %linker%
	stamp_timer.exe end
popd
if %errorlevel%==0 goto success
goto fail

:success
copy build\server.exe server.exe > NUL
copy build\client.exe client.exe > NUL
goto end

:fail

:end
copy build\temp_compiler_output.txt compiler_output.txt > NUL
type build\temp_compiler_output.txt

