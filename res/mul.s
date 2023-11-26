var1 = $00
var2 = $01
answer = $02

buffer = $4000
command = $40ff

	.org $8000
RESET:
	ldx #$ff		; Initialize stack
	txs				; ...
	cld				; clear decimal flag
	lda #23			; push arg1 (23) to stack
	pha				; ...
	lda #7			; push arg2 (7) to stack
	pha				; ...
	jsr multiply 	; call multiply 8x3
	pla				; flush arg2
	pla				; flush arg1, (our answer)
	sta buffer		; store answer in io memory
	lda #$cc		; print number to terminal
	sta command		; ...
	lda #$bb		; stop emulation
	sta command		; ...

multiply:			; multiply two unsigned numbers
	tsx				; move stack pointer into A register
	txa 			; ...
	clc				; clear carry bit
	adc #3			; 
	tax				; index of the second argument
	lda $100, X		; copy second argument
	sta var2		; ...
	inx				; increment x
	lda $100, X		; copy first argument
	sta var1		; ...
	lda #0
	sta answer		; store asnwer in the zero page
mul_loop:
	lda #1			; test bit one of second arg
	bit var2		; ...
	beq mul_loop2	; if bit 1 is set, branch
	lda answer		; move answer into A
	clc
	adc var1		; add shifted arg1 to answer
	sta answer		; store the answer
mul_loop2:
	asl var1		; bit shift arg 1 left
	lsr var2 		; bit shift arg 2 right
	lda var2		; set zero flag on arg2
	bne mul_loop	; continue loop if arg2 isn't zero
	lda answer 		; put asnwer on the stack
	sta $100, X		; put asnwer 1 into arg1's place
	rts				; return from mutliply subroutine
	
IRQ:
NMI:
	
	.org $fffa
	.word IRQ		; IRQ vector
	.word RESET	; Reset vector
	.word NMI		; NMIRQ vector