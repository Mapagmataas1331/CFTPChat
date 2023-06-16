@echo off
title CFTPChat build

cd ../SERVER/build
ninja

cd ../../CLIENT/build
ninja

cd ../../_build

echo f | xcopy ..\SERVER\build\main.exe server.exe /y/q
echo f | xcopy ..\CLIENT\build\main.exe client.exe /y/q

start cmd /c server.exe
echo Starting the SERVER . . .

timeout 2 > NUL

start cmd /c client.exe
echo Starting the CLIENT . . .


timeout 4 > NUL