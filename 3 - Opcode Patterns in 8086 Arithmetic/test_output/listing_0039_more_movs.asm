; disassembly for listings\listing_0039_more_movs
bits 16
mov si, bx
mov dh, al
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
