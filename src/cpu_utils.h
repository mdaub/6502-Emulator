/**
 * @file cpu_utils.h
 * @author Mason Daub
 * @brief Utilities and defitions required for the cpu internals
 * @version 0.1
 * @date 2023-11-25
 * 
 * @copyright Copyright (c) 2023 Mason Daub
 * 
 */

#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include "cpu.h"

// Some macros to make writing code faster
#define A cpu_regA
#define P cpu_FLAGS
#define X cpu_regX
#define Y cpu_regY
#define S cpu_SP
#define PC cpu_PC
#define accum_flags update_Zflag(A); update_Nflag(A)

/*   Status Flags   */

#define flag_N 0x80 // Negative Flag
#define flag_V 0x40 // Overflow Flag
#define flag_B 0x10 // Break Flag
#define flag_D 0x08 // BCD flag
#define flag_I 0x04 // Interrupt Disable Flag
#define flag_Z 0x02 // Zero Flag
#define flag_C 0x01 // Carry flag

/*
* 6502 is little endian, that is it stores
* the LSB first and then the MSB after.
*/

/**
 * @brief Enumeratation of the 6502's addressing modes.
 * Usually the address modes are the first 8, which are
 * what is encoded in 3 bits of the instruction. There are
 * however several instructions with "special" address
 * modes that are only used a handful of times.
 */
typedef enum _address_mode
{
    ind_indir_x = 0,    // indexed indirect (X) - (ind, X)
    zpg         = 1,    // zero page
    imm         = 2,    // immediate value
    abs         = 3,    // absolute address 1W
    indir_ind_y = 4,    // indirect indexed: (ind), Y
    ind_zpg_x   = 5,    // indexed zero page (X)
    ind_abs_y   = 6,    // indexed asbolute (Y)
    ind_abs_x   = 7,    // indexed asbolute (X)
    indir_abs,          // indirect absolute, only used for JMP
    rel,                // relative, only used by branching 
    ind_zpg_y,          // indexed zero page (Y), only used by LDX & STX
    ind_abs,            // indirect abs, only used by JMP
    reg_A,              // accumulator, only used by ASL, LSR, ROL & ROR
} address_mode;

/**
 * Instruction Masks:
 * Most instructions for the 6502 have the format 0bxxxlllxx
 * where lll is a 3 bit address mode number. Depending on the 
 * context these modes can change, and there is some overlap
 * between different instructions and these modes (5 bits only
 * supports 32 distince instructions)
 */

#define mode_mask 0x1c  // address mode mask

/*  FULL ADDRESS OPERATIONS  */
#define ADC 0x61  // Add carry mask
#define AND 0x21  // AND mask
#define CMP 0xc1  // compare mask
#define EOR 0x41  // XOR
#define LDA 0xA1  // Load A
#define ORA 0x01  // OR with A
#define SBC 0xe1  // Sub with carry instruction

/*  PARTIAL ADDRESS OPERATIONS  */

#define ASL 0x02  // arithmetic shift left         0a 06 16 0e 1e
#define BIT 0x20  // Bit compare                   24 2c
#define CPX 0xe0  // Compare with X                e0 e4 ec
#define CPY 0xc0  // Compare with Y                c0 c4 cc
#define DEC 0xc2  // Decrement memory              c6 d6 ce de
#define INC 0xe2  // Increment memory              e6 f6 ee fe
#define JMP 0x40  // Jump to abs or indirect abs   4c 6c
#define LDX 0xA2  // Load X with mem               a2 a6 b6 ae be
#define LDY 0xa0  // Load Y with mem               a0 a4 b4 ac bc
#define LSR 0x42  // Logical shift right           4a 46 56 4e 5e
#define ROL 0x22  // Rotate left                   2a 26 36 2e 3e
#define ROR 0x62  // Rotate Right                  6a 66 76 6e 7e
#define STA 0x81  // Store A                       85 95 8d 9d 99 81 91
#define STX 0x82  // Store X                       86 96 8e
#define STY 0x80  // Store Y                       84 94 8c

/*  STATIC ADDRESS OPERATIONS  */

#define BCC 0x90  // Branch carry clear
#define BCS 0xb0  // Branch carry set
#define BEQ 0xf0  // Branch zero set
#define BMI 0x30  // Branch negative set
#define BNE 0xd0  // Branch zero clear
#define BPL 0x10  // Branch negative clear
#define BRK 0x00  // Break (Software Interrupt)
#define BVC 0x50  // Branch overflow clear
#define BVS 0x70  // Branch overflow set
#define CLC 0x18  // Clear carry
#define CLD 0xd8  // Clear BCD
#define CLI 0x58  // Allow IRQ
#define CLV 0xb8  // Clear overflow
#define DEX 0xca  // Decrement X
#define DEY 0x88  // Decrement Y
#define INX 0xe8  // Increment X
#define INY 0xc8  // Increment Y
#define JSR 0x20  // Call Subroutine
#define NOP 0xEA  // No Operation
#define PHA 0x48  // PusH A to stack
#define PHP 0x08  // PusH P to stack
#define PLA 0x68  // PulL stack to A
#define PLP 0x28  // PulL stack to P
#define RTI 0x40  // Ret from interrupt
#define RTS 0x60  // Ret from subroutine
#define SEC 0x38  // Set carry
#define SED 0xf8  // Enable BCD
#define SEI 0x78  // Disable maskable interrupts
#define TAX 0xaa  // Transfer A -> X
#define TAY 0xa8  // Transfer A -> Y
#define TSX 0xba  // Transfer S -> X
#define TXA 0x8a  // Transfer X -> A
#define TXS 0x9a  // Transfer X -> S
#define TYA 0x98  // Transfer Y -> A



/**
 * @brief Push's 1 byte to the CPU's stack.
 * 
 * @param data Data to push to the stack.
 */
void cpu_stack_push(byte data);

/**
 * @brief Pop's data from the CPU's stack.
 * 
 * @return returns the byte at the top of the stack.
 */
byte cpu_stack_pop();

/**
 * @brief Fetches the data at the location of the PC and increments it.
 * 
 * @return byte at the current PC address.
 */
byte cpu_fetch();

/**
 * @brief Get the effective address given the addressing mode and arguments.
 * 
 * @param mode the addressing mode.
 * @param arg1 first argument of the mode.
 * @param arg2 second argument of the mode.
 * @return size_t: the effective address.
 */
size_t get_effective_address(address_mode mode, byte arg1, byte arg2);

/**
 * @brief Reads the data stored with a specific addressing mode.
 * 
 * @param mode Address mode -- does not support immediate mode or accumulator mode.
 * @param arg1 first argument
 * @param arg2 second argument
 * @return returns the byte stored at the effective address
 */
byte read_address(address_mode mode, byte arg1, byte arg2);

/**
 * @brief Determines additional delay from page changes in addressing.
 * 
 * @param mode address mode.
 * @param arg1 first opcode argument.
 * @param arg2 second opcode argument.
 * @return returns the number of additional cycles, if there are any.
 */
int address_delay(address_mode mode, byte arg1, byte arg2); // returns the cycle delay from page changes.

/**
 * @brief Updates the zero flag given some data.
 * 
 * @param res The data to update the zero flag with.
 */
void update_Zflag(byte res);

/**
 * @brief Sets the appropriate value of the negative flag given some data.
 * 
 * @param res The data to update the flag with.
 */
void update_Nflag(byte res);

/**
 * @brief Set carry flag if argument cannot be stored in 1 byte
 * 
 * @param intermed Intermediate value to test.
 */
void update_Cflag(int intermed);

/**
 * @brief Retrieves the specified number of arguments from the current PC address.
 * 
 * @param count The number of arguments for the operand.
 * @param arg1 Pointer to the first argument
 * @param arg2 Pointer to the second argument
 */
void get_args(int count, byte* arg1, byte* arg2); // Get the operator arguments

/**
 * @brief Get the data with the given address mode and accounts for immediate addressing.
 * 
 * @param mode addressing mode.
 * @param arg1 opcode first argument.
 * @param arg2 opcode second argument.
 * @return byte corresponding to the address mode and arguments.
 */
byte get_data_full_imm(address_mode mode, byte arg1, byte arg2);

/**
 * @brief Get the data with given address mode. Supports accumulator mode.
 * 
 * @param mode addressing mode.
 * @param arg1 first opcode argument.
 * @param arg2 second opcode argument.
 * @return byte for corresponding data.
 */
byte get_data_accum(address_mode mode, byte arg1, byte arg2);

/**
 * @brief Set the data with the given address mode. Supports accumulator mode.
 * 
 * @param mode the address mode.
 * @param arg1 opcode first argument.
 * @param arg2 opcode second argument.
 * @param data data to be written.
 */
void set_data_accum(address_mode mode, byte arg1, byte arg2, byte data);

/**
 * @brief Returns the dissasembled string for the address mode.
 * 
 * @param mode address mode.
 * @param arg1 opcode first argument.
 * @param arg2 opcode second argument.
 * @param buffer buffer to print to. Should be at least 16 characters wide.
 * @return 0
 */
int address_mode_str(address_mode mode, byte arg1, byte arg2, char* buffer);

/**
 * @brief Does a branch if the condition is met.
 * 
 * @param condition if condition is set, the branch will be preformed.
 * @param arg1 the relative addres to branch to.
 * @return the number of additional cycles.
 */
int branch_instruction(uint8_t condition, byte arg1);

#endif