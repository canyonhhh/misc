global _start

%include "syscalls.inc"
%include "functions.inc"

%define BUFFER_SIZE 	1024
%define NICKNAME 	"irctestassembler"
%define NAME 		"Tester McTestFace"
%define CHANNEL 	"#myworld"
%define PASSWORD 	"doesnthaveone"

SECTION .data
	lf 		db 	0x0a, 0
	crlf 		db 	0x0d, 0x0a, 0
	NICK_MSG 	db 	"NICK ", NICKNAME, 0
	USER_MSG	db 	"USER ", NICKNAME, " 0 * :", NAME, 0
	JOIN_MSG 	db 	"JOIN ", CHANNEL, 0
	PRIVMSG_MSG 	db 	"PRIVMSG ", CHANNEL, " :", 0
	NICKSRV_MSG 	db 	"PRIVMSG NickServ :IDENTIFY ", NICKNAME, " ", PASSWORD, 0
	PING_CHECK 	db 	"PING", 0
	QUIT_CHECK 	db 	"QUIT", 0
	JOIN_CHECK 	db 	"376", 0
	sent 		db 	"SENT: ", 0
	received 	db 	"RECEIVED: ", 0
	buffer 		db 	BUFFER_SIZE dup(0)
	readfds 	db 	128 dup(0)

SECTION .text

_start:
	push 	rbp
	mov 	rbp, rsp

	mov 	rax, [rbp + 0x04]

	xor 	rax, rax
	xor 	rbx, rbx
	xor 	rdi, rdi
	xor 	rsi, rsi

_socket:
	mov 	rax, SYS_SOCKET
	mov 	rdi, 2
	mov 	rsi, 1
	xor 	edx, edx
	syscall

	mov 	rbx, rax

_connect:
	mov 	dword [rsp], 0x0b1a0002
	mov 	rsi, rsp
	mov 	rdx, 16
	mov 	rdi, rbx
	mov 	dword [rsp+0x4], 0x7ee8b982
	mov 	rax, SYS_CONNECT
	syscall

_write:
	
	mov 	rax, NICK_MSG
	call 	sendcrlf
	mov 	rax, USER_MSG
	call 	sendcrlf
	mov 	rax, JOIN_MSG
	call 	sendcrlf

_loop:
	mov 	rax, readfds
	mov 	rdi, 128
	call 	clear_buffer

	;; SET STDIN
	mov 	rdi, readfds
	mov 	rax, 1
	or 	[rdi], rax

	;; SET sockfd
	mov 	rdi, readfds
	mov 	rcx, rbx
	mov 	rax, 1
	shl 	rax, cl
	or 	[rdi], rax

	;;; Prepare select() call
	mov 	rdi, rbx
	inc 	rdi
	mov 	rsi, readfds
	xor 	rdx, rdx
	xor 	rcx, rcx
	xor 	r10, r10
	xor 	r8, r8
	mov 	rax, SYS_SELECT
	syscall

	;;;; IF sockfd is set
	mov 	rax, [readfds]
	mov 	rcx, rbx
	mov 	rdx, 1
	shl 	rdx, cl
	and 	rax, rdx
	jz 	_if_stdin_set

	; Clean buffer
	mov 	rax, buffer
	mov 	rdi, 1024
	call 	clear_buffer

	; Read from socket
	mov 	rax, SYS_READ
	mov 	rdi, rbx
	mov 	rsi, buffer
	mov 	rdx, 1024
	syscall
	jz 	_close

	; Print buffer
	mov 	rax, received
	call 	sprint
	mov 	rax, buffer
	call 	sprint

	; Respond to PING
	mov 	rsi, buffer
	mov 	rdi, PING_CHECK
	mov 	rcx, 4
	rep 	cmpsb
	jnz 	continue

	mov 	dword [buffer + 1], 'O'
	mov 	rax, buffer
	call 	sendcrlf
	
_if_stdin_set:
	mov 	rax, [readfds]
	mov 	rcx, 1
	and 	rax, rcx
	jz 	continue

	;;;;;;; process input
	mov 	rax, buffer
	mov 	rdi, 1024
	call 	clear_buffer

	mov 	rax, SYS_READ
	mov 	rdi, 0
	mov 	rsi, buffer
	mov 	rdx, 1024
	syscall

	; if first 4 chars are "QUIT" then jump to _close
	mov 	rdi, buffer
	mov 	rsi, QUIT_CHECK
	mov 	rcx, 4
	rep 	cmpsb
	jz 	_close

	mov 	rax, PRIVMSG_MSG
	call 	send
	mov 	rax, buffer
	call 	sendcrlf

	continue:
	jmp 	_loop

_close:
	mov 	rsp, rbp
	pop 	rbp

	mov 	rax, SYS_CLOSE
	mov 	rdi, rbx
	syscall

_exit:	
	mov     rax, SYS_EXIT
	mov 	rdi, rbx
	syscall
