iprint:
	push 	rdx
	push 	rcx
	push 	rbx

	mov 	rcx, 10
	mov 	rdx, 0
	mov 	rbx, 0

	nextdigit:
	xor 	rdx, rdx
	div 	rcx
	add 	rdx, '0'
	push 	rdx
	inc 	rbx
	test 	rax, rax
	jnz 	nextdigit

	printdigit:
	pop 	rdx
	mov 	rax, 4
	mov 	rbx, 1
	int 	0x80
	dec 	rbx
	jnz 	printdigit

	pop 	rbx
	pop 	rcx
	pop 	rdx
	ret

slen:
	push 	rbx
	mov 	rbx, rax

	nextchar:
	cmp 	byte [rax], 0
	jz 	finish
	inc 	rax
	jmp 	nextchar

	finish:
	sub 	rax, rbx
	pop 	rbx
	ret

; asciiz print from rax
sprint:
	push 	rbx
	push 	rdi
	push 	rsi
	push 	rdx

	push 	rax
	call 	slen
	mov 	rdx, rax
	pop 	rax
	mov 	rsi, rax
	mov 	rdi, 1

	mov 	rax, SYS_WRITE
	syscall

	pop 	rdx
	pop 	rsi
	pop 	rdi
	pop 	rbx
	ret

sprintlf:
	push 	rax
	call 	sprint
	pop 	rax
	mov 	rax, lf
	call 	sprint
	ret

send:
	push 	rbx
	push 	rdi
	push 	rsi
	push 	rdx

	push 	rax
	mov 	rax, sent
	call 	sprint
	pop 	rax
	push 	rax
	call 	sprint
	mov 	rax, lf
	call 	sprint
	pop 	rax
	
	push  	rax
	call 	slen
	mov 	rdx, rax
	pop 	rax

	mov 	rdi, rbx
	mov 	rsi, rax

	mov 	rax, SYS_WRITE
	syscall

	pop 	rdx
	pop 	rsi
	pop 	rdi
	pop 	rbx
	ret

sendcrlf:
	call 	send

	mov 	rax, SYS_WRITE
	mov 	rsi, crlf
	mov 	rdi, rbx
	mov 	rdx, 2
	syscall

	ret

clear_buffer:
	; rax is buffer address
	; rdi is buffer length
	; using stosb to clear buffer
	push 	rbx
	push 	rcx
	push 	rdi

	mov 	rcx, rdi
	mov 	rdi, rax
	mov 	al, 0
	rep 	stosb

	pop 	rdi
	pop 	rcx
	pop 	rbx
	ret

find_char_after_space:
	push rdi
	mov rcx, rdi
	add rcx, 1024

search_loop:
	cmp byte [rdi], ' '
	je found_space
	inc rdi
	cmp rdi, rcx
	jb search_loop

	xor rax, rax
	pop rdi
	ret

found_space:
	inc rdi
	mov rax, rdi
	pop rdi
	ret

compare_three_chars:
	mov rax, 0
	mov al, '3'
	cmp al, [rdi]
	jne mismatch
	inc rdi
	mov al, '7'
	cmp al, [rdi]
	jne mismatch
	inc rdi
	mov al, '6'
	cmp al, [rdi]
	jne mismatch

	mov al, 1
mismatch:
	ret
