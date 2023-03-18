; disassembly for .*s
bits 16
mov si, bx
mov dh, al
mov cl, byte 12
mov ch, byte -12
mov cx, word 12
mov cx, word -12
mov dx, word 3948
mov dx, word -3948
mov al, [bx + si]
mov bx, [bp + di]
mov dx, [bp]
mov ah, [bx + si + 4]
mov al, [bx + si + 4999]
mov [bx + di], cx
mov [bp + si], cl
mov [bp], ch
mov ax, [bx + di - 37]
mov [si - 300], cx
mov dx, [bx - 32]
mov [bp + di], byte 7
mov [di + 901], word 347
mov bp, [5]
mov bx, [3458]
mov ax, [2555]
mov ax, [16]
mov [2554], ax
mov [15], ax
jcxz $+52
jcxz $+56
mov ax, word -245
jno $-28
