@echo off

start "server" server.exe
start "client1" client.exe
timeout /t 1 > NUL
start "client2" client.exe