@echo off

echo Add MinGW to path
set PATH=%PATH%;C:\Program Files\CodeBlocks\MinGW\bin

echo Build main object
gcc -c -o main.o main.c

echo Build resource object
windres -o main.rc.o main.rc

echo Link objects
gcc -o toggle-turbo.exe main.o main.rc.o
