@echo off

start "server" server.exe
start "client1" client.exe
@REM timeout /t 1 > NUL
start "client2" client.exe
@REM start "client3" client.exe