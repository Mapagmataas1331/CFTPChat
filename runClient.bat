@echo off
title CFTPChat
mode 80, 24

cd CLIENT/build
ninja
start cmd /c main.exe
echo Starting the CLIENT . . .

timeout 2 > NUL