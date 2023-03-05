@echo off

if not exist test_output mkdir test_output

REM NOTE(daniel): A for loop over a list in batch?
REM Too rich for my blood.

REM ----------------------------------------------------------------

set listing=listing_0037_single_register_mov

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0038_many_register_mov

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0039_more_movs

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0040_challenge_movs

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------
