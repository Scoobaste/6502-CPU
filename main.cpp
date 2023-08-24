//  6502 CPU Emulator in C++
//  https://www.youtube.com/playlist?list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37
//  
//  Reference Material
//  http://www.6502.org/users/obelisk/
//  

#include <stdio.h>
#include <stdlib.h>

using Byte = unsigned char;     // Signifies 8 Bit types
using Word = unsigned short;    // Signifies 16 bit types

using u32 = unsigned int;

struct Mem  {
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialise()   {
        for (u32 i = 0; i < MAX_MEM; i++)
        {
            Data[i] = 0;
        }
    }

    //  Read 1 byte
    Byte operator[](u32 Address) const  {
        //  Assert here Address is < MAX_MEM
        return Data[Address];
    }
};


struct CPU  {
    Word PC, SP;        // Program Counter & Stack Pointer
    
    Byte A, X, Y;       // Accumulator, Index X & Y registers

    Byte C : 1;         // Carry Status flag
    Byte Z : 1;         // Zero flag
    Byte I : 1;         // Interupt Disable flag
    Byte D : 1;         // Decimal Mode flag
    Byte B : 1;         // Break Command flag
    Byte V : 1;         // Overflow flag
    Byte N : 1;         // Negative flag

    void Reset(Mem& memory)    {
        PC = 0xFFFC;
        SP = 0x0100;
        C = Z = I = D = B = V = N = 0;  // Reset all flags
        A = X = Y = 0;                  // Reset all registers
        memory.Initialise();
    }

    Byte FetchByte(u32& Cycles, Mem& memory)    {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

//  *** OPCODES ***
//  LDA - Immediate
static constexpr Byte
        INS_LDA_IM = 0xA9;


    void Execute(u32 Cycles, Mem& memory)    {
        while (Cycles > 0)
        {
            Byte Instruction = FetchByte(Cycles, memory);
            switch (Instruction)
            {
                case INS_LDA_IM:
                {
                    Byte Value = FetchByte(Cycles, memory);
                    A = Value;
                    Z = (A == 0);
                    N = (A & 0b10000000) > 0  ;
                }
                break;
            }
        }
    }
};


int main(int argc, const char * argv[]) {
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);
    cpu.Execute(2, mem);

    return 0;
}
