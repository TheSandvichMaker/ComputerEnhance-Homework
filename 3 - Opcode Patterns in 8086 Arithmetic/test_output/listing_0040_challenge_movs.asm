; disassembly for listings\listing_0040_challenge_movs
bits 16
mov ax, [bx + di - 37]           ; 10001011 01000001 11011011
mov [si - 300], cx               ; 10001001 10001100 11010100 11111110
mov dx, [bx - 32]                ; 10001011 01010111 11100000
mov [bp + di], byte 7            ; 11000110 00000011 00000111
mov [di + 901], word 347         ; 11000111 10000101 10000101 00000011 01011011 00000001
mov bp, [5]                      ; 10001011 00101110 00000101 00000000
mov bx, [3458]                   ; 10001011 00011110 10000010 00001101
mov ax, [2555]                   ; 10100001 11111011 00001001
mov ax, [16]                     ; 10100001 00010000 00000000
mov [2554], ax                   ; 10100011 11111010 00001001
mov [15], ax                     ; 10100011 00001111 00000000
