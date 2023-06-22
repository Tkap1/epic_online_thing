@echo off

@REM start "server" server.exe
start "client1" client.exe
@REM timeout /t 1 > NUL
start "client2" client.exe
start "client3" client.exe