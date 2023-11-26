/**
 * @file cpu.h
 * @author Mason Daub
 * @brief Exposed (public) elements of the 6502 cpu. Currently only 1 CPU instance is supported. 
 * @version 0.1
 * @date 2023-11-25
 * 
 * @copyright Copyright (c) 2023 Mason Daub
 * 
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>

#define IRQ_ADDRESS 0xfffe
#define RST_ADDRESS 0xfffc
#define NMI_ADDRESS 0xfffa

typedef uint8_t byte;       // redefine to byte to make writing code faster. May change.

extern byte cpu_regA;       // CPU accumulator register
extern byte cpu_regX;       // CPU X index register
extern byte cpu_regY;       // CPU Y index register
extern byte cpu_SP;         // CPU stack pointer register (S)
extern byte cpu_FLAGS;      // CPU flags/status register (P)
extern uint16_t cpu_PC;     // CPU program counter register (16 bit)

/**
 * @brief Reads the memory on the address bus.
 * 
 * @param address Address to read.
 * @return returns the byte at the corresponding address.
 */
byte read_memory(size_t address);

/**
 * @brief Reads a word from the address bus.
 * 
 * @param address The address to read.
 * @return The word located at address. Little Endian.
 */
uint16_t read_memory_word(size_t address);

/**
 * @brief Writes data to the address bus.
 * 
 * @param address 16 bit address to write to.
 * @param data Data to write.
 */
void write_memory(size_t address, byte data);

/**
 * @brief Sets up the CPU in a reset state.
 * 
 */
void cpu_reset();

/**
 * @brief Preform the operation at the current PC address.
 * 
 * @return The number of clock cycles required to preform the operation.
 */
int cpu_do_next_op();

/**
 * @brief Wait a specified number of CPU clock cycles.
 * 
 * @param cycles The number of cycles to wait.
 */
void cpu_delay(int cycles);

/**
 * @brief Dissasembles the instruction at a specified address.
 * 
 * @param adr The address of the instruction to dissasemble.
 * @param buffer Buffer to write output.
 * @param buffsize size of the buffer.
 * @return 0
 */
int dissasemble(uint16_t adr, char* buffer, int buffsize);

/*
struct _cpu_interface
{
    //unsigned int address : 16;
    unsigned int data : 8;          // INPUT/OUTPUT
    unsigned int IRQB : 1;          // INPUT regular interrupt request
    unsigned int NMIB : 1;          // INPUT non-maskable interrupt
    unsigned int ready : 1;         // INPUT Disables the CPU (unless its in a write-cycle)
    unsigned int read_write : 1;    // OUTPUT read active-high, write active-low
    unsigned int overflow : 1;      // INPUT Sets the overflow flag
    unsigned int sync : 1;
} extern cpu_interface;
*/

#endif // CPU_H