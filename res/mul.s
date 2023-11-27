var1 = $00
var2 = $01
answer = $02

var1_low = $03
var1_high = $04
answer_low = $05
answer_high = $06

buffer = $4000
command = $40ff

	.org $8000
RESET:
	ldx #$ff			; Initialize stack
	txs					; ...
	cld					; clear decimal flag
	lda #-100			; push arg1 to stack
	pha					; ...
	lda #-16			; push arg2 to stack
	pha					; ...
	jsr mul_signed  	; call multiply 8x3
	pla					; flush arg2
	sta buffer + 1
	pla					; flush arg1, (our answer)
	sta buffer			; store answer in io memory
	lda #$ce			; print number to terminal
	sta command			; ...
	lda #$bb			; stop emulation
	sta command			; ...

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

multiply_word: 		; multiplies 2 8-bit numbers for a 16-bit result
	tsx				; move stack pointer into A register
	txa 			; ...
	clc				; clear carry bit
	adc #3			; 
	tax				; index of the second argument
	lda $100, X		; copy second argument
	sta var2		; ...
	inx				; increment x
	lda $100, X		; copy first argument
	sta var1_low	; ...
	lda #0
	sta var1_high
	sta answer_low	; store answer in the zero page
	sta answer_high
mul2_loop:
	lda #1			; test bit one of second arg
	bit var2		; ...
	beq mul2_loop2	; if bit 1 is set, branch
	lda answer_low	; move answer into A
	clc
	adc var1_low	; add shifted arg1 to answer
	sta answer_low	; store the answer
	lda answer_high	;
	adc var1_high	;
	sta answer_high
mul2_loop2:
	asl var1_low	; bit shift arg 1 left
	rol var1_high	; bit shift high byte, with carry!
	lsr var2 		; bit shift arg 2 right
	lda var2		; set zero flag on arg2
	bne mul2_loop	; continue loop if arg2 isn't zero
	lda answer_low 	; put answer on the stack
	sta $100, X		; put asnwer 1 into arg1's place
	lda answer_high
	dex
	sta $100, X
	rts				; return from mutliply subroutine


; same algorithm as unsigned, except it must first convert the signed numbers
; into unsigned ones, and then multiply the result by negative 1 if
mul_signed:
	; set answer and var1 to 0
	lda #0
	sta answer_low
	sta answer_high
	sta var1_high
	tsx
	lda $104, X 	; first argument
	bpl smul_positive_1 ; if arg1 is negative, make it positive
	eor #$ff
	clc
	adc #1
smul_positive_1:
	sta var1_low
	lda $103, X 	; second argument
	bpl smul_positive_2
	eor #$ff
	clc
	adc #1
smul_positive_2:
	sta var2
smul_loop:
	lda #1			; test bit one of second arg
	bit var2		; ...
	beq smul_loop2	; if bit 1 is set, branch
	lda answer_low	; move answer into A
	clc
	adc var1_low	; add shifted arg1 to answer
	sta answer_low	; store the answer
	lda answer_high	;
	adc var1_high	;
	sta answer_high
smul_loop2:
	asl var1_low	; bit shift arg 1 left, without carry
	rol var1_high	; bit shift high byte, with carry!
	lsr var2 		; bit shift arg 2 right
	lda var2		; set zero flag on arg2
	bne smul_loop	; continue loop if arg2 isn't zero
	lda $104, X		; get first arg
	eor $103, X		; get second arg
	bmi smul_answer_neg
	lda answer_low 	; put answer on the stack
	sta $104, X		; put asnwer 1 into arg1's place
	lda answer_high
	sta $103, X
	rts				; return from mutliply subroutine
smul_answer_neg:
	lda #0
	sec
	sbc answer_low
	sta $104, x
	lda #0
	sbc answer_high
	sta $103, X
	rts

IRQ:
NMI:
	
	.org $fffa
	.word IRQ		; IRQ vector
	.word RESET	; Reset vector
	.word NMI		; NMIRQ vector