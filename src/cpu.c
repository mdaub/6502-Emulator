/**
 * @file cpu.c
 * @author Mason Daub
 * @brief Contains the core functionality required to run the 6502 emulation.
 * @version 0.1
 * @date 2023-11-25
 * 
 * @copyright Copyright (c) 2023 Mason Daub
 * 
 */


#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "cpu_utils.h"
#include "cpu.h"

/* CPU register declarations */

byte cpu_SP = 0xff;
uint16_t cpu_PC = 0;
byte cpu_regA, cpu_regX, cpu_regY, cpu_SP, cpu_FLAGS;

/* Lookup tables related to number of instruction operands */

const int c_conv[] = {2, 1, 5, 3, 7, 6, 0, 4}; // LUT for timing array index
const int count_full_imm[]  = {1, 1, 1, 2, 1, 1, 2, 2};
const int count_full_a[]    = {1, 1, 0, 2, 1, 1, 2, 2};

// Wait for a specified number of clock cycles
void cpu_delay(int num_cycles)
{
    ;
}
void cpu_reset()
{
    PC = read_memory_word(RST_ADDRESS);
}

void cpu_stack_push(byte data)
{
    assert(cpu_SP != 0);
    size_t address = 0x0100 | cpu_SP; // computer effective address of the stack
    write_memory(address, data); // load data
    cpu_SP--; // decrement stack pointer 
}

byte cpu_stack_pop()
{
    assert(cpu_SP != 0xff);
    cpu_SP++;
    size_t address = 0x0100 | cpu_SP;
    return read_memory(address); // return data at original memory
}

void IRQ()
{

}

byte read_address(address_mode mode, byte arg1, byte arg2)
{
    return read_memory(get_effective_address(mode, arg1, arg2));
}

size_t get_effective_address(address_mode mode, byte arg1, byte arg2)
{
    size_t eff_address = 0;
    const uint16_t abs_arg = (arg2 << 8) | arg1;
    size_t address1;
    switch(mode)
    {
        case zpg:
            eff_address = arg1;
            break;
        case ind_zpg_x:
            eff_address = (X + arg1) & 0xff;
            break;
        case ind_zpg_y:
            eff_address = (Y + arg1) & 0xff;
            break;
        case abs:
            eff_address = abs_arg;
            break;
        case ind_abs_x:
            eff_address = (X + abs_arg)& 0xffff;
            break;
        case ind_abs_y:
            eff_address = (Y + abs_arg) & 0xffff;
            break;
        case rel:
            eff_address = PC + *(int8_t*)(&arg1); // signed offset
            break;
        case indir_abs:
            eff_address = read_memory(abs_arg);
            break;
        case ind_indir_x:
            address1 = (X + arg1) & 0xff;
            eff_address = (read_memory(address1) << 8) | read_memory(address1 + 1);
            break;
        case indir_ind_y:
            eff_address = read_memory(arg1) + Y;
            break;
        default:
            return 0x0000;
    }
    return eff_address;
}

int address_delay(address_mode mode, byte arg1, byte arg2)
{
    size_t warg = arg1 | (arg2 << 8);
    unsigned int add;
    switch(mode)
    {
        case ind_abs_x:
            return (warg & 0xff) + cpu_regX > 0xff;
        case ind_indir_x:
            return (cpu_regX + (size_t) arg1) > 0xff;
        case indir_ind_y:
            add = read_memory(arg1);
            add |= read_memory(arg1+1) << 8; // read the word at zpg arg1
            return (add & 0xff) + cpu_regY > 0xff;
        default:
        return 0;
    }
}


// Returns number of clock cycles it would have taken to execute
// Yes I could have made this using if statements and seperate functions.
// I tried to keep branching and function calls to a minimum to minimize
// emulator overhead.
int cpu_do_next_op()
{
    #ifdef DEBUG
    char buffer[16];
    dissasemble(PC, buffer, 16);
    #endif
    byte opcode = cpu_fetch();
    address_mode mode = (opcode & mode_mask) >> 2;
    byte arg1, arg2;    // stores the (up to) 2 operands of the op
    byte data;          // storing operation data
    int intermediate;   // 9 bit result of math op

    const int ADC_cycles[]  = {2, 3, 4, 4, 4, 4, 6, 5};
    const int AND_cycles[]  = {2, 2, 3, 4, 4, 4, 6, 5};
    const int CMP_cycles[]  = {2, 3, 4, 4, 4, 4, 6, 5};
    const int EOR_cycles[]  = {2, 3, 4, 4, 4, 4, 6, 5};
    const int LDA_cycles[]  = {2, 3, 4, 4, 4, 4, 6, 5};
    const int OR_cycles[]   = {2, 2, 3, 4, 4, 4, 6, 5};
    const int SBC_cycles[]  = {2, 3, 4, 4, 4, 4, 6, 5};
    // Full address instructions;
    switch(opcode & ~mode_mask)
    {
        case ADC:
            get_args(count_full_imm[mode], &arg1, &arg2);
            data = get_data_full_imm(mode, arg1, arg2);
            intermediate = (P & flag_C) + data + A; // add carry to data + A
            P = ((A & 0x80) != (intermediate & 0x80)) ? P | flag_V : P & ~flag_V; // set V if A.7 != res.7
            P = (A & 0x80) | (P & 0x7f); // set neg flag to A neg
            update_Zflag(intermediate);
            
            /*  TODO: BCD ADD  */
            P = (P & ~flag_C) | (intermediate > 0xff) * flag_C; 
            A = intermediate & 0xff;
            return ADC_cycles[c_conv[mode]] + address_delay(mode, arg1, arg2);
        
        case AND:
            get_args(count_full_imm[mode], &arg1, &arg2);
            data = get_data_full_imm(mode, arg1, arg2);
            A &= data;
            accum_flags;
            return AND_cycles[c_conv[mode]] + address_delay(mode, arg1, arg2);

        case CMP:
            get_args(count_full_imm[mode], &arg1, &arg2);
            data = get_data_full_imm(mode, arg1, arg2);
            intermediate = A + ~data + 1;
            P = (intermediate & flag_N) | (P & ~flag_N); // set negative flag to bit 7 of sub
            P = (intermediate == 0) ? P | flag_Z : P & ~flag_Z; // set zero flag
            P = (A >= data) ? P | flag_C : P & ~flag_C; // set carry flag if A >= data
            return CMP_cycles[c_conv[mode]];

        case EOR:
            get_args(count_full_imm[mode], &arg1, &arg2);
            data = get_data_full_imm(mode, arg1, arg2);
            A ^= data;
            accum_flags;
            return EOR_cycles[c_conv[mode]] + address_delay(mode, arg1, arg2); 
        
        case LDA:
            get_args(count_full_imm[mode], &arg1, &arg2);
            A = get_data_full_imm(mode, arg1, arg2);
            accum_flags;
            return LDA_cycles[c_conv[mode]] + address_delay(mode, arg1, arg2);
            
        case ORA:
            get_args(count_full_imm[mode], &arg1, &arg2);
            data = get_data_full_imm(mode, arg1, arg2);
            A |= data;
            accum_flags;
            return AND_cycles[mode] + address_delay(mode, arg1, arg2);

        case SBC:
            get_args(count_full_imm[mode], &arg1, &arg2);
            data = get_data_full_imm(mode, arg1, arg2);
            if(P & flag_D) // TODO BCD
            {
                
            }
            else
            {
                intermediate = A + ~data + (P & flag_C);
                P = (intermediate > 127 || intermediate < -128) ? P | flag_V : P & ~flag_V; // Overflow cond
            }
            P = (intermediate >= 0) ? P | flag_C : P & ~flag_C;
            P = (intermediate & flag_N) | (P & ~flag_N);
            update_Zflag(intermediate);
            A = intermediate & 0xff;
            return SBC_cycles[c_conv[mode]] + address_delay(mode, arg1, arg2);
    }
   
    /* Fixed Address Modes */
    uint8_t COMP_reg;
    switch(opcode)
    {
        case BCC:
            arg1 = cpu_fetch();
            return branch_instruction(~P & flag_C, arg1);

        case BCS:
            arg1 = cpu_fetch();
            return branch_instruction(P & flag_C, arg1);

        case BEQ:
            arg1 = cpu_fetch();
            return branch_instruction(P & flag_Z, arg1);

        case BMI:
            arg1 = cpu_fetch();
            return branch_instruction(P & flag_N, arg1);

        case BNE:
            arg1 = cpu_fetch();
            return branch_instruction(~P & flag_Z, arg1);

        case BPL:
            arg1 = cpu_fetch();
            return branch_instruction(~P & flag_N, arg1);
            
        case BRK:
            PC++; // this is a bug, but we'll keep it
            cpu_stack_push((PC >> 8) & 0xff);
            cpu_stack_push(PC & 0xff);
            cpu_stack_push(P | flag_B);
            PC = read_memory_word(IRQ_ADDRESS);
            return 7;

        case BVC:
            arg1 = cpu_fetch();
            return branch_instruction(P & flag_C, arg1);

        case BVS:
            arg1 = cpu_fetch();
            return branch_instruction(P & flag_C, arg1);

        case CLC:
            P &= ~flag_C;
            return 2;

        case CLD:
            P &= ~flag_D;
            return 2;

        case CLI:
            P &= ~flag_I;
            return 2;

        case CLV:
            P &= ~flag_V;
            return 2;

        case DEX:
            X--;
            update_Zflag(X);
            update_Nflag(X);
            return 2;

        case DEY:
            Y--;
            update_Zflag(Y);
            update_Nflag(Y);
            return 2;

        case INX:
            X++;
            update_Zflag(X);
            update_Nflag(X);
            return 2;

        case INY:
            X++;
            update_Zflag(Y);
            update_Nflag(Y);
            return 2;

        case JSR:
            arg1 = cpu_fetch(); arg2 = cpu_fetch();
            PC--;
            cpu_stack_push((PC >> 8) & 0xff);
            cpu_stack_push(PC & 0xff);
            PC = arg1 | (arg2 << 8);
            return 6;

        case NOP:
            return 2;
        case PHA:
            cpu_stack_push(A);
            return 3;
            
        case PHP:
            cpu_stack_push(P);
            return 3;

        case PLA:
            A = cpu_stack_pop();
            accum_flags;
            return 4;

        case PLP:
            P = cpu_stack_pop();
            return 4;

        case RTI:
            P = cpu_stack_pop();
            PC = cpu_stack_pop();
            PC |= cpu_stack_pop() << 8;
            return 6;

        case RTS:
            PC = cpu_stack_pop();
            PC |= cpu_stack_pop() << 8;
            PC++;
            return 6;

        case SEC:
            P |= flag_C;
            return 2;

        case SED:
            P |= flag_D;
            return 2;

        case SEI:
            P |= flag_I;
            return 2;

        case TAX:
            X = A;
            update_Nflag(X);
            update_Zflag(X);
            return 2;

        case TAY:
            Y = A;
            update_Nflag(Y);
            update_Zflag(Y);
            return 2;

        case TSX:
            X = S;
            update_Zflag(X);
            update_Nflag(X);
            return 2;

        case TXA:
            A = X;
            accum_flags;
            return 2;

        case TXS:
            S = X;
            update_Nflag(S);
            update_Zflag(S);
            return 2;
        case TYA:
            A = Y;
            accum_flags;
            return 2;

        case 0x4c: // JMP abs
            get_args(2, &arg1, &arg2);
            PC = arg1 | (arg2 << 8);
            return 3;
        case 0x6c: // JMP (abs)
            get_args(2, &arg1, &arg2);
            intermediate = arg1 | (arg2 << 8);
            PC = read_memory(intermediate) | (read_memory((intermediate + 1) & 0xffff) << 8);
            return 5;

    }

    /* Variable Address Modes */
    switch (opcode & ~mode_mask)
    {
    case ASL: // A zpg zpg,X abs abs,X
        assert(mode == imm || mode == zpg || mode == ind_zpg_x || mode == abs || mode == ind_abs_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        data = get_data_accum(mode, arg1, arg2);
        P = (P & ~flag_C) | flag_C * ((data & 0x80) == 0x80); // set carry flag if bit 7 of data is set
        data = (data << 1) & 0xfe;
        update_Zflag(data);
        set_data_accum(mode, arg1, arg2, data);
        return address_delay(mode, arg1, arg2) + mode == imm ? 2 : mode == zpg ? 5 :
        mode == ind_zpg_x ? 6 : mode == abs ? 6 : mode == ind_abs_x ? 7 : -1;

    case BIT:
        assert(mode == zpg || mode == abs);
        get_args(mode == zpg ? 1 : 2, &arg1, &arg2);
        data = A & read_address(mode, arg1, arg2);        
        P = (P & ~0xc0) | (data & 0xC0);
        update_Zflag(data);
        return mode == zpg ? 3 : 4;
    
    case CPY:
        COMP_reg = Y;
        goto COMP;
    case CPX:
        COMP_reg = X;
        COMP:
        assert(mode == imm || mode == zpg || mode == abs);
        get_args(count_full_imm[mode], &arg1, &arg2);
        data = get_data_full_imm(mode, arg1, arg2);
        intermediate = COMP_reg - data;
        P = (P & ~flag_N) | (flag_N * ((intermediate & 0x80) == 0x80));
        P = (P & ~flag_C) | (flag_C * (intermediate >= data));
        P = (P & ~flag_Z) | (flag_Z * (intermediate == 0));
        return mode == imm ? 2 : mode == zpg ? 3 : 4;

    case DEC:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs || mode == ind_abs_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        data = (read_address(mode, arg1, arg2) - 1) & 0xff;
        update_Nflag(data);
        update_Zflag(data);
        set_data_accum(mode, arg1, arg2, data);
        return address_delay(mode, arg1, arg2) + mode == zpg ? 5 : mode == ind_zpg_x ? 6 : mode == abs ? 6 : 7;

    case INC:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs || mode == ind_abs_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        data = (read_address(mode, arg1, arg2) + 1) & 0xff;
        update_Nflag(data);
        update_Zflag(data);
        set_data_accum(mode, arg1, arg2, data);
        return mode == zpg ? 5 : mode == ind_zpg_x ? 6 : mode == abs ? 6 : 7;

    // JMP handeled in static addressing

    case LDX:
        assert(mode == ind_indir_x || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        mode = mode == ind_indir_x ? imm : mode;
        get_args(count_full_imm[mode], &arg1, & arg2);
        mode = mode == ind_zpg_x ? ind_zpg_y : mode;
        X = get_data_full_imm(mode, arg1, arg2);
        update_Nflag(X);
        update_Zflag(X);
        return address_delay(mode, arg1, arg2) + mode == imm ? 2 : mode == zpg ? 3 : mode == ind_zpg_y ? 4 : mode == abs ? 4 : 4;
    case LDY:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_imm[mode], &arg1, & arg2);
        Y = get_data_full_imm(mode, arg1, arg2);
        update_Nflag(Y);
        update_Zflag(Y);
        return address_delay(mode, arg1, arg2) + mode == imm ? 2 : mode == zpg ? 3 : mode == ind_zpg_x ? 4 : mode == abs ? 4 : 4;
    case LSR:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        P &= ~(flag_N & flag_C); // clear negative and carry flags
        data = get_data_accum(mode, arg1, arg2);
        data = (data >> 1) & 0x7f;
        set_data_accum(mode, arg1, arg2, data);
        update_Zflag(data);
        return address_delay(mode, arg1, arg2) + mode == imm ? 2 : mode == zpg ? 5 : mode == ind_zpg_x ? 6 : mode == abs ? 6 : 7;
    case ROL:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        data = get_data_accum(mode, arg1, arg2);
        intermediate = ((data & 0x80) == 0x80) * flag_C;
        data = (data << 1) & 0xfe;
        data |= P & flag_C;
        P = (P & ~flag_C) | intermediate; // set carry to bit 7 of original data
        update_Zflag(data);
        update_Nflag(data);
        set_data_accum(mode, arg1, arg2, data);
        return address_delay(mode, arg1, arg2) + mode == imm ? 2 : mode == zpg ? 5 : mode == ind_zpg_x ? 6 : mode == abs ? 6 : 7;
    case ROR:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        data = get_data_accum(mode, arg1, arg2);
        intermediate = ((data & 0x01) == 0x01) * flag_C;
        data = (data >> 1) & 0x7f;
        data |= (P & flag_C != 0) * 0x80; // set bit 7 if carry bit is high
        P = (P & ~flag_C) | intermediate; // set carry to bit 0 of original data
        update_Zflag(data);
        update_Nflag(data);
        set_data_accum(mode, arg1, arg2, data);
        return address_delay(mode, arg1, arg2) + mode == imm ? 2 : mode == zpg ? 5 : mode == ind_zpg_x ? 6 : mode == abs ? 6 : 7;
    case STA:
        assert(mode != imm); // Supports all modes except immediate / accum
        get_args(count_full_imm[mode], &arg1, &arg2);
        write_memory(get_effective_address(mode, arg1, arg2), A);
        return address_delay(mode, arg1, arg2) + mode == zpg ? 3 : mode == ind_zpg_x ? 4 : mode == abs ? 4 : mode == ind_abs_x ? 5 : mode == ind_abs_y ? 5 : 6;

    case STX:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs);
        get_args(mode, &arg1, &arg2);
        mode = mode == ind_zpg_x ? ind_zpg_y : mode; 
        set_data_accum(mode, arg1, arg2, X);
        return mode == zpg ? 3 : 4;

    case STY:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs);
        get_args(mode, &arg1, &arg2);
        set_data_accum(mode, arg1, arg2, Y);
        return mode == zpg ? 3 : 4;

    default:
    break;
    }

    assert(0); // unkown instruction if the program makes it here
    return 0;
}

// Yes this does modify the program counter while it's running.
// Yes that is stupid and dangerous.
// It restores it before exiting, but it remains to be seen if it's untrustworthy...
int dissasemble(uint16_t adr, char* buffer, int buffsize)
{
    buffer[0] = 0; // set to detect if op was found;
    char add_str[32];
    uint16_t _PC = PC;
    PC = adr;
    byte opcode = cpu_fetch();;
    address_mode mode = (opcode & mode_mask) >> 2;
    byte arg1, arg2;
    switch(opcode & ~mode_mask)
    {
        case ADC:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "ADC %s", add_str);
            break;

        case AND:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "AND %s", add_str);
            break;

        case CMP:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "CMP %s", add_str);
            break;

        case EOR:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "EOR %s", add_str);
            break; 
        
        case LDA:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "LDA %s", add_str);
            break;

        case ORA:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "ORA %s", add_str);
            break;

        case SBC:
            get_args(count_full_imm[mode], &arg1, &arg2);
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "SBC %s", add_str);
            break;
    }
   
    /* Fixed Address Modes */
    switch(opcode)
    {
        case BCC:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BCC %s", add_str);
            break;

        case BCS:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BCS %s", add_str);
            break;

        case BEQ:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BEQ %s", add_str);
            break;

        case BMI:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BMI %s", add_str);
            break;

        case BNE:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BNE %s", add_str);
            break;

        case BPL:
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BPL %s", add_str);
            break;
            
        case BRK:
            sprintf(buffer, "BRK");
            break;

        case BVC:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BVC %s", add_str);
            break;

        case BVS:
            arg1 = cpu_fetch();
            mode = rel;
            address_mode_str(mode, arg1, arg2, add_str);
            sprintf(buffer, "BVS %s", add_str);
            break;

        case CLC:
            sprintf(buffer, "CLC");
            break;

        case CLD:
            sprintf(buffer, "CLD");
            break;

        case CLI:
            sprintf(buffer, "CLI");
            break;

        case CLV:
            sprintf(buffer, "CLV");
            break;

        case DEX:
            sprintf(buffer, "DEX");
            break;

        case DEY:
            sprintf(buffer, "DEY");
            break;

        case INX:
            sprintf(buffer, "INX");
            break;

        case INY:
            sprintf(buffer, "INY");
            break;

        case JSR:
            arg1 = cpu_fetch(); arg2 = cpu_fetch();
            address_mode_str(abs, arg1, arg2, add_str);
            sprintf(buffer, "JSR %s", add_str);
            break;

        case NOP:
            sprintf(buffer, "NOP");
            break;
        case PHA:
            sprintf(buffer, "PHA");
            break;
            
        case PHP:
            sprintf(buffer, "PHP");
            break;

        case PLA:
            sprintf(buffer, "PLA");
            break;

        case PLP:
            sprintf(buffer, "PLP");
            break;

        case RTI:
            sprintf(buffer, "RTI");
            break;

        case RTS:
            sprintf(buffer, "RTS");
            break;

        case SEC:
            sprintf(buffer, "SEC");
            break;

        case SED:
            sprintf(buffer, "SED");
            break;

        case SEI:
            sprintf(buffer, "SEI");
            break;

        case TAX:
            sprintf(buffer, "TAX");
            break;

        case TAY:
            sprintf(buffer, "TAY");
            break;

        case TSX:
            sprintf(buffer, "TSX");
            break;

        case TXA:
            sprintf(buffer, "TXA");
            break;

        case TXS:
            sprintf(buffer, "TXS");
            break;
        case TYA:
            sprintf(buffer, "TYA");
            break;

        case 0x4c: // JMP abs
            get_args(2, &arg1, &arg2);
            address_mode_str(abs, arg1, arg2, add_str);
            sprintf(buffer, "JMP %s", add_str);
            break;
        case 0x6c: // JMP (abs)
            get_args(2, &arg1, &arg2);
            address_mode_str(ind_abs, arg1, arg2, add_str);
            sprintf(buffer, "JMP %s", add_str);
            break;

    }
    if(buffer[0] != 0) 
    {
        PC = _PC;
        return 0;
    }
    /* Variable Address Modes */
    switch (opcode & ~mode_mask)
    {
    case ASL: // A zpg zpg,X abs abs,X
        assert(mode == imm || mode == zpg || mode == ind_zpg_x || mode == abs || mode == ind_abs_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        mode = mode == imm ? reg_A : mode;
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "ASL %s", add_str);
        break;

    case BIT:
        assert(mode == zpg || mode == abs);
        get_args(mode == zpg ? 1 : 2, &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "BIT %s", add_str);
        break;
    
    case CPY:
        assert(mode == imm || mode == zpg || mode == abs);
        get_args(count_full_imm[mode], &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "CPY %s", add_str);
        break;
    case CPX:
        assert(mode == imm || mode == zpg || mode == abs);
        get_args(count_full_imm[mode], &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "CPX %s", add_str);
        break;

    case DEC:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs || mode == ind_abs_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "DEC %s", add_str);
        break;
    case INC:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs || mode == ind_abs_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "INC %s", add_str);
        break;

    // JMP handeled in static addressing

    case LDX:
        assert(mode == ind_indir_x || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        mode = mode == ind_indir_x ? imm : mode;
        get_args(count_full_imm[mode], &arg1, & arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "LDX %s", add_str);
        break;
    case LDY:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_imm[mode], &arg1, & arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "LDY %s", add_str);
        break;
    case LSR:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        mode = mode == imm ? reg_A : mode;
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "LSR %s", add_str);
        break;
    case ROL:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        mode = mode == imm ? reg_A : mode;
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "ROL %s", add_str);
        break;
    case ROR:
        assert(mode == imm || mode == zpg || mode == abs || mode == ind_abs_x || mode == ind_zpg_x);
        get_args(count_full_a[mode], &arg1, &arg2);
        mode = mode == imm ? reg_A : mode;
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "ROR %s", add_str);
        break;
    case STA:
        assert(mode != imm); // Supports all modes except immediate / accum
        get_args(count_full_imm[mode], &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "STA %s", add_str);
        break;

    case STX:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs);
        get_args(mode, &arg1, &arg2);
        mode = mode == ind_zpg_x ? ind_zpg_y : mode;
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "STX %s", add_str);
        break;

    case STY:
        assert(mode == zpg || mode == ind_zpg_x || mode == abs);
        get_args(mode, &arg1, &arg2);
        address_mode_str(mode, arg1, arg2, add_str);
        sprintf(buffer, "STY %s", add_str);
        break;
    break;
    }

    //assert(0); // unkown instruction if the program makes it here
    if(buffer[0] == 0)
        sprintf(buffer, "<%02x>", opcode);
    PC = _PC; // restore program counter;
    return 0;
}

// Yes I could have used a lookup table instead, but oh well.
int address_mode_str(address_mode mode, byte arg1, byte arg2, char* buffer)
{
    const char* format;
    size_t arg = arg1;
    const size_t word = arg1 | (arg2 << 8);
    switch(mode)
    {
        case reg_A:
            format = "A";
            break;
        case ind_indir_x:
            arg = word;
            format = "($%04x, X)";
        case zpg:
            format = "$%02x";
            break;
        case rel:
            sprintf(buffer, "$%02x ; $%04x", arg1, 0xffff & (PC + (arg1 & 0x80 ? (0xff00 | arg1) : arg1)));
            return 0;
        case imm:
            format = "#%02x";
            break;
        case abs:
            arg = word;
            format = "$%04x";
            break;
        case ind_abs:
            arg = word;
            format = "($%04x)";
            break;
        case indir_ind_y:
            arg = word;
            format = "($%04x), Y";
            break;
        case ind_zpg_x:
            format = "$%02x, X";
            break;
        case ind_zpg_y:
            format = "$%02x, Y";
            break;
        case ind_abs_x:
            arg = word;
            format = "$%04x, X";
            break;
        case ind_abs_y:
            arg = word;
            format = "$%04x, Y";
            break;
        case indir_abs:
            arg = word;
            format = "($%04x)";
            break;
        //case indir_zpg:
        //    format = "($%02x)";
        //    break;
        default:
            format = "<??>";
    }
    sprintf(buffer, format, arg);
    return 0;
}

int branch_instruction(uint8_t condition, uint8_t arg1)
{
    if(condition)
    {
        size_t rel = arg1 & 0x80 ? (arg1 | 0xff00) : arg1; // set high byte to ff if arg1 is neg
        size_t new_PC = PC + rel;
        int retval = (new_PC & 0xff00) == (PC & 0xff00) ? 3 : 4; // add 1 C for page change
        PC = new_PC & 0xffff;
        return retval;
    }
    return 2;
}

byte get_data_full_imm(address_mode mode, byte arg1, byte arg2)
{
    return mode == imm ? arg1 : read_address(mode, arg1, arg2);
}

byte get_data_accum(address_mode mode, byte arg1, byte arg2)
{
    return mode == imm ? A : read_address(mode, arg1, arg2);
}

void set_data_accum(address_mode mode, byte arg1, byte arg2, byte data)
{
    if(mode == imm)
        A = data;
    else{
        write_memory(get_effective_address(mode, arg1, arg2), data);
    }
}

void get_args(int count, byte* arg1, byte* arg2)
{
    int increment = 0;
    switch(count)
    {
        case 2:
        *arg1 = cpu_fetch();
        *arg2 = cpu_fetch();
        break;
        case 1:
        *arg1 = cpu_fetch();
    }
}

byte cpu_fetch()
{
    return read_memory(cpu_PC++);
}

void update_Zflag(byte res)
{
    cpu_FLAGS = (P & ~flag_Z) | (res == 0) * flag_Z;
}

void update_Nflag(byte res)
{
    cpu_FLAGS = (P & ~flag_N) | ((res & 0x80) > 0) * flag_N;
}

void update_Cflag(int res)
{
    cpu_FLAGS = (P & ~flag_C) | ((res & 0x100) > 0) * flag_C;
}

/*

byte to_bcd_format(byte in)
{
    int ones = in % 10;
    int tens = in - ones;
    return ones | (tens & 0xf << 4);
}

unsigned int add_bcd(byte in1, byte in2)
{
    byte ones1 = in1 & 0xf;
    byte ones2 = in2 & 0xf;
    ones1 += ones2;
    return in1 + in2 + (ones1 >= 10) * 6; // add 6 if the ones are larger than 1 digit
}

*/