@echo off

echo Add MinGW to path
rem set PATH=%PATH%;C:\Dev-Cpp\bin

echo Build main object
g++ -Wall -Wextra -c -o main.o -Os -ggdb src/main.cpp -Iinclude/

echo Build resource object
windres -o main.rc.o resources/main.rc

echo Link objects
g++ -o toggle-turbo.exe main.o main.rc.o
