@echo off

start "server" server.exe
start "client1" client.exe
@REM timeout /t 1 > NUL
@REM start "client2" client.exe