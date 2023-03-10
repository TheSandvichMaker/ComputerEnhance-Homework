@echo off

if not exist test_output mkdir test_output

REM make sure the listings are all up to date
call build_listings.bat

REM NOTE(daniel): A for loop over a list in batch?
REM Too rich for my blood.

REM ----------------------------------------------------------------

set listing=listing_0037_single_register_mov

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc /B listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0038_many_register_mov

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc /B listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0039_more_movs

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc /B listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0040_challenge_movs

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc /B listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0041_add_sub_cmp_jnz

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc /B listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------

set listing=listing_0042_completionist_decode

sim8086 listings\%listing% > test_output\%listing%.asm
nasm test_output\%listing%.asm -o test_output\%listing%
fc /B listings\%listing% test_output\%listing%

REM ----------------------------------------------------------------
