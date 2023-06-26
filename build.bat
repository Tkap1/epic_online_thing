
@echo off

cls

if NOT defined VSCMD_ARG_TGT_ARCH (
	@REM Replace with your path
	call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

if not exist build\NUL mkdir build

set comp=-nologo -std:c++20 -Zc:strictStrings- -W4 -FC -Gm- -GR- -EHa- -wd 4324 -wd 4127 -wd 4505 -D_CRT_SECURE_NO_WARNINGS
set linker=user32.lib Shell32.lib -INCREMENTAL:NO

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

@REM taskkill /IM "client.exe" > NUL 2> NUL
@REM taskkill /IM "server.exe" > NUL 2> NUL

SETLOCAL ENABLEDELAYEDEXPANSION
pushd build
	..\stamp_timer.exe start

	cl ..\src\client.cpp -LD -Feclient.dll %comp% -Dm_app -link %linker% -PDB:client.pdb opengl32.lib > temp_compiler_output.txt
	if NOT !ErrorLevel! == 0 (
		type temp_compiler_output.txt
		popd
		goto fail
	)
	type temp_compiler_output.txt

	cl ..\src\server.cpp -LD -Feserver.dll %comp% -Dm_app -link %linker% -PDB:server.pdb > temp_compiler_output.txt
	if NOT !ErrorLevel! == 0 (
		type temp_compiler_output.txt
		popd
		goto fail
	)
	type temp_compiler_output.txt

	tasklist /fi "ImageName eq client.exe" /fo csv 2>NUL | find /I "client.exe">NUL
	if NOT !ERRORLEVEL!==0 (
		cl ..\src\win32_platform_client.cpp -Feclient.exe %comp% -Dm_app -link %linker% -PDB:platform_client.pdb gdi32.lib opengl32.lib Xinput.lib Ole32.lib Winmm.lib "..\enet64.lib" Ws2_32.lib > temp_compiler_output.txt
		if NOT !ErrorLevel! == 0 (
			type temp_compiler_output.txt
			popd
			goto fail
		)
		type temp_compiler_output.txt
	)

	tasklist /fi "ImageName eq server.exe" /fo csv 2>NUL | find /I "server.exe">NUL
	if NOT !ERRORLEVEL!==0 (
		cl ..\src\win32_platform_server.cpp -Feserver.exe %comp% -Dm_app -link %linker% "..\enet64.lib" Ws2_32.lib Winmm.lib -PDB:platform_server.pdb > temp_compiler_output.txt
		if NOT !ErrorLevel! == 0 (
			type temp_compiler_output.txt
			popd
			goto fail
		)
		type temp_compiler_output.txt
	)

	..\stamp_timer.exe end
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

