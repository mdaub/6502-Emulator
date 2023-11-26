command = $40ff
buffer = $4000

    
    .org $8000

string: .asciiz "Hello World!"

reset:
    LDX #$ff        ; init SP
    TXS     
print:              ; X is $ff, when we increment, it will be $00
    INX             ; Increment X
    LDA string, X   ; Load the Xth character to A
    STA buffer, X   ; Move the character to the buffer
    BNE print       ; loop if character is not null
    LDA #$aa        ; write command
    STA command
    LDA #$bb        ; Stop command
    STA command

    .org $fffc
    .word reset     ; Set the Reset Vector
    .word $0000
