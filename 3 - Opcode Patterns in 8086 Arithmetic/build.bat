@echo off

echo[
echo -----------------------------------------------------
echo Building Disassembler
echo -----------------------------------------------------
echo[

cl.exe /nologo /Zi /W4 /WX /wd4201 /D_CRT_SECURE_NO_WARNINGS sim8086.c
