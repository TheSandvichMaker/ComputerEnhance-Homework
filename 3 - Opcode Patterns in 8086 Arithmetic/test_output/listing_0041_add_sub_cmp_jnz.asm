; disassembly for listings\listing_0041_add_sub_cmp_jnz
bits 16
add bx, [bx + si]
add bx, [bp]
add si, word 2
add bp, word 2
add cx, word 8
add bx, [bp]
add cx, [bx + 2]
add bh, [bp + si + 4]
add di, [bp + di + 6]
add [bx + si], bx
add [bp], bx
add [bp], bx
add [bx + 2], cx
add [bp + si + 4], bh
add [bp + di + 6], di
add [bx], byte 34
add [bp + si + 1000], word 29
add ax, [bp]
add al, [bx + si]
add ax, bx
add al, ah
add ax, word 1000
add al, byte -30
add al, byte 9
sub bx, [bx + si]
sub bx, [bp]
sub si, word 2
sub bp, word 2
sub cx, word 8
sub bx, [bp]
sub cx, [bx + 2]
sub bh, [bp + si + 4]
sub di, [bp + di + 6]
sub [bx + si], bx
sub [bp], bx
sub [bp], bx
sub [bx + 2], cx
sub [bp + si + 4], bh
sub [bp + di + 6], di
sub [bx], byte 34
sub [bx + di], word 29
sub ax, [bp]
sub al, [bx + si]
sub ax, bx
sub al, ah
sub ax, word 1000
sub al, byte -30
sub al, byte 9
cmp bx, [bx + si]
cmp bx, [bp]
cmp si, word 2
cmp bp, word 2
cmp cx, word 8
cmp bx, [bp]
cmp cx, [bx + 2]
cmp bh, [bp + si + 4]
cmp di, [bp + di + 6]
cmp [bx + si], bx
cmp [bp], bx
cmp [bp], bx
cmp [bx + 2], cx
cmp [bp + si + 4], bh
cmp [bp + di + 6], di
cmp [bx], byte 34
cmp [4834], word 29
cmp ax, [bp]
cmp al, [bx + si]
cmp ax, bx
cmp al, ah
cmp ax, word 1000
cmp al, byte -30
cmp al, byte 9
jne $+4
jne $-2
jne $-4
jne $-2
je $+0
jl $-2
jle $-4
jb $-6
jbe $-8
jp $-10
jo $-12
js $-14
jne $-16
jge $-18
jg $-20
jae $-22
ja $-24
jpo $-26
jno $-28
jns $-30
loop $-32
loope $-34
loopne $-36
jcxz $-38
