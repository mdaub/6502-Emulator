/**
 * @file main.c
 * @author Mason Daub
 * @brief Runs an emulation of the MOS 6502 processor. 
 * It is not cycle accurate, or even timing accurate at the moment.
 * Only one CPU instance is allowed at the moment.
 * 
 * Currently only has one IO device mapped to $4000-$40ff -- the terminal.
 * This allows the 6502 CPU to write to the terminal and request the
 * emulation be terminated.
 * 
 * @version 0.1
 * @date 2023-11-25
 * 
 * @copyright Copyright (c) 2023 Mason Daub
 * 
 */

#include "cpu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define RAM_SIZE 0x4000
#define IO_SIZE 0x4000
#define ROM_SIZE 0x8000

/**
 * @brief Read the contents of a file into ROM
 * 
 * @param filename The name of the file
 * @return 0 on success
 */
int read_file(const char* filename);

/**
 * @brief Load the 'Hello World!' program into ROM
 * 
 */
void load_hello_world();

/**
 * @brief Run the CPU (and terminal) normally
 * 
 */
void run_mode();

/**
 * @brief Run the CPU in Debug Mode.
 * This allows single stepping, reading addresses and registers.
 * @todo Add breakpoints.
 */
void debug_mode();

/**
 * @brief Runs the terminal IO device.
 * Only supports printing to terminal and stopping the emulation.
 * @return true: keep running the emulation.
 * @return false: Terminate the emulation.
 */
bool run_terminal_interface();

/**
 * @brief returns a pointer to the memory mapped to the address bus.
 * 
 * For now there is only a single memory map defined for reads and writes.
 * Systems like the NES have a map that changes according to reads and writes.
 * Each IO device will likely want it's own read/write functions so it can
 * make changes internally on read/write.
 * 
 * @param address the value of the address bus
 * @return Pointer to the mapped data.
 */
byte* memory_map(size_t address);

byte RAM_DATA[RAM_SIZE];    // RAM 
byte IO_MEM[IO_SIZE];       // IO memory
byte ROM_DATA[ROM_SIZE];    // ROM



// Machine Code for the Hello World program
const char hello_world[] =
{
    'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o','r', 'l', 'd', '!','\0', // string
    0xa2, 0xff,         // 800c: LDX ff
    0x9a,               // 800d: TXS
    0xe8,               // 800e: INX  -> PRINT
    0xbd, 0x00, 0x80,   // 800f: LDA $8000, X
    0x9d, 0x00, 0x40,   // 8012: STA $4000, X
    0xd0, 0xf7,         // 8015: BNE PRINT
    0xa9, 0xaa,         // LDA $aa
    0x8d, 0xff, 0x40,    // STA $40ff
    0xa9, 0xbb,         // LDA $bb
    0x8d, 0xff, 0x40    // STA $40ff
};



int main(int argc, char* argv[])
{
    puts("*** 6502 EMULATOR ***");
    
    // Load the program options
    bool has_input = false;
    bool debug = false;
    for(int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];
        // file input
        if(strcmp(arg, "-f") == 0 && (i + 1) < argc)
        {
            printf("Reading binary from file '%s'...\n", argv[i+1]);
            read_file(argv[++i]);
            has_input = true;
        }
        else if(strcmp(arg, "-d") == 0)
        {
            debug = true;
        }
        else
        {
            printf("Argument %d: '%s'\n", i, argv[i]);
        }
    }

    // Load the 'Hello World!' binary if no input is specified.
    if(!has_input)
    {
        puts("No input binary: Loading Hello World...");
        load_hello_world();
    }
    
    write_memory(0x40ff, 0);    // init terminal by setting its command to 0
    cpu_reset();                // reset the cpu

    // Run the CPU normally
    if(!debug)
    {
        run_mode();
    }

    // Start the debug (single step) mode
    else if(debug) 
    {
        debug_mode();
    }


    return EXIT_SUCCESS;
}

int read_file(const char* filename)
{
    //printf("File input string: '%s'\n", filename);
    FILE* file = fopen(filename, "r");
    assert(file != NULL);
    ROM_DATA[0] = fgetc(file);
    int i = 1;
    while(i < ROM_SIZE && (ROM_DATA[i++] = fgetc(file)) != EOF); // load contents into memory
    fclose(file);
    return 0;
}

byte* memory_map(size_t address)
{
    address &= 0xffff;
    if(address < 0x4000) // ram
    {
        return RAM_DATA + address;
    }
    else if(address < 0x8000) // IO
    {
        return IO_MEM + address - 0x4000;
    }
    else return ROM_DATA + address -0x8000; // ROM
}


byte read_memory(size_t address)
{
    return *memory_map(address);
}

uint16_t read_memory_word(size_t address)
{
    return read_memory(address) | (read_memory(address + 1) << 8);
}

void write_memory(size_t address, byte data)
{
    *memory_map(address) = data; // currently this will still allow writing to ROM
}

void write_memory_word(size_t address, uint16_t word)
{
    write_memory(address, word & 0xff); // write l
    write_memory(address + 1, (word >> 8) & 0xff); // write h
}

void load_hello_world()
{
    int len = sizeof(hello_world)/ sizeof(char);
    for(int i = 0; i < len; i++)
    {
        ROM_DATA[i] = hello_world[i];
    }
    ROM_DATA[RST_ADDRESS-0x8000] = 0x0d;
    ROM_DATA[RST_ADDRESS-0x8000 + 1] = 0x80;
}

void run_mode()
{
    bool running = true;
    while(running){
        cpu_do_next_op();
        running = run_terminal_interface();
    }
}

bool run_terminal_interface()
{
    bool running = true;
    // read the command and set it to zero.
    byte command = read_memory(0x40ff);
    write_memory(0x40ff, 0);
    
    // if terminal command is 0xaa, write contents of buffer
    if(command == 0xaa)
    {
        puts(IO_MEM);
    }
    // 6502 emulator stop command.
    else if (command == 0xbb)
    {
        puts("Emulator recieved halt command...");
        running = false;
    }
    return running;
}

void debug_mode()
{
    bool running = true;
    int read_start, read_stop;
    char buffer[256];
    
    while(running)
    {
        dissasemble(cpu_PC, buffer, sizeof(buffer) / sizeof(char));

        // Print the contents of the registers and the dissasembled instruction
        printf("\nPC: %4x A: %2x X: %2x Y: %2x P: %2x S: %2x\n", cpu_PC, cpu_regA, cpu_regX, cpu_regY, cpu_FLAGS, cpu_SP);
        printf("\nCurrent Instruction: '%s'\n", buffer);

        fgets(buffer, 256, stdin); // get user input

        // next or n (single step)
        if(strncmp(buffer, "next", 4) == 0 || strcmp(buffer, "n") == 0)
        {
            cpu_do_next_op();
            running = run_terminal_interface();
        }

        // Read range of memory. Format: 'read start:stop' (in hex)
        else if(sscanf(buffer, "read %x:%x", &read_start, &read_stop) == 2)
        {
            if(read_start >= read_stop)
            {
                printf("Bad Read: (%04x:%04x)", read_start, read_stop);
            }
            else
            {
                //printf("(%4x:%4x):\n", read_start, read_stop);
                int num_reads = read_stop - read_start;
                int n_rows = num_reads >> 3; // 8 columns
                for(int j = 0; j <= n_rows; j++)
                {
                    printf("(%04x): ", read_start + j * 8);
                    for(int i = 0; i <= (j == n_rows ? num_reads & 7 : 7); i++)
                    {
                        printf("%02x ", read_memory(read_start + i + j * 8));
                    }
                    puts("");
                }
            }
            
        }

        // Read single byte in memory (address in hex)
        else if(sscanf(buffer, "read %x", &read_start) == 1)
        {
            printf("(%04x): %02x\n", read_start, read_memory(read_start));
        }

        // Terminate the emulation
        else if(strncmp(buffer, "stop", 4) == 0)
        {
            running = false;
        }
    }
}