@echo off
title CFTPChat
mode 80, 24

cd SERVER/build
ninja
start cmd /c main.exe
echo Starting the SERVER . . .

timeout 2 > NUL