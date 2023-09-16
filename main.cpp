//  6502 CPU Emulator in C++
//  Written by Dave Poo, modified by Steve Jackson (2023)
//  https://www.youtube.com/playlist?list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37
//  https://github.com/davepoo/6502Emulator
//  
//  Reference Material
//  http://www.6502.org/users/obelisk/
//  
//  DONT FORGET TO REMOVE ASSERTS!

#include <cassert>
#include <cstdint>
#include <iostream>

// This struct represents a memory block.
struct Mem {
    // This static constexpr member variable represents the maximum size of the Data array.
    // It is initialised to the value 1024 * 64, which represents 64 kilobytes of memory.
    static constexpr std::uint32_t MAX_MEM = 1024 * 64;

    // This member variable represents the actual data stored in the memory block.
    // It is an array of std::uint8_t (unsigned 8-bit integers) with a size of MAX_MEM.
    std::uint8_t Data[MAX_MEM];

    // This function initializes the elements of the Data array to 0.
    void Initialise() {
        // This for loop iterates over each element of the Data array.
        // The loop variable i is initialised to 0, and the loop continues as long as i is less than MAX_MEM.
        // MAX_MEM represents the size of the Data array.
        for (std::uint32_t i = 0; i < MAX_MEM; i++) {
            // This line of code sets the value of the ith element in the Data array to 0.
            Data[i] = 0;
        }
    }

    // Read 1 byte
    // This function overloads the subscript operator [] for a class.
    // It takes an unsigned 32-bit integer Address as input and returns an 8-bit unsigned integer.
    std::uint8_t operator[](std::uint32_t Address) const {
        // This assert statement checks if the Address is within the valid range.
        // If Address is greater than or equal to MAX_MEM, the program will terminate and display an error message.
        assert(Address < MAX_MEM);

        // This line of code returns the value stored in the Data array at the given Address.
        return Data[Address];
    }

    // Write 1 byte
    // This function overloads the subscript operator [] for a class.
    // It takes an unsigned 32-bit integer Address as input and returns a reference to an 8-bit unsigned integer.
    std::uint8_t& operator[](std::uint32_t Address) {
        // This assert statement checks if the Address is within the valid range.
        // If Address is greater than or equal to MAX_MEM, the program will terminate and display an error message.
        assert(Address < MAX_MEM);

        // This line of code returns a reference to the value stored in the Data array at the given Address.
        // By returning a reference, it allows modifying the value at that address directly.
        return Data[Address];
    }

    // Write 2 bytes
    void WriteWord(std::uint16_t Value, std::uint32_t Address, std::uint32_t& Cycles)   {
        Data[Address]     = Value & 0xFF;
        Data[Address + 1] = (Value >> 8);
        Cycles -= 2;
    }
};


// This struct represents a CPU.
struct CPU {
    std::uint16_t PC, SP;       // Program Counter & Stack Pointer
    std::uint8_t A, X, Y;       // Accumulator, Index X & Y registers
    std::uint8_t C : 1;         // Carry Status flag
    std::uint8_t Z : 1;         // Zero flag
    std::uint8_t I : 1;         // Interrupt Disable flag
    std::uint8_t D : 1;         // Decimal Mode flag
    std::uint8_t B : 1;         // Break Command flag
    std::uint8_t V : 1;         // Overflow flag
    std::uint8_t N : 1;         // Negative flag

    // This function resets the CPU state.
    void Reset(Mem& memory) {
        // Reset program counter to 0xFFFC.
        PC = 0xFFFC;
        // Reset stack pointer to 0x0100.
        SP = 0x0100;
        // Reset all flags to 0.
        C = Z = I = D = B = V = N = 0;
        // Reset all registers to 0.
        A = X = Y = 0;
        // Initialize memory.
        memory.Initialise();
    }

    // Fetches a byte from memory at the current program counter (PC) and updates the necessary variables.
    // @param Cycles The number of cycles taken by the operation (updated by reference).
    // @param memory The memory block from which to fetch the byte.
    // @return The fetched byte value.
    std::uint8_t FetchByte(std::uint32_t& Cycles, Mem& memory) {
        // Fetch byte from memory at the current program counter (PC).
        std::uint8_t Data = memory[PC];
        // Increment program counter (PC).
        PC++;
        // Decrement cycle count.
        Cycles--;
        // Return fetched byte.
        return Data;
    }

    std::uint16_t FetchWord(std::uint32_t& Cycles, Mem& memory) {
        // Fetch word from memory at the current program counter (PC)
        // 6502 is little endian
        std::uint16_t Data = memory[PC];
        // Increment program counter (PC).
        PC++;

        Data |= (memory[PC] << 8);
        // Increment program counter (PC).
        PC++;
        // Decrement cycle count.
        Cycles -= 2;
        // Return fetched byte.

        return Data;
    }

    // Fetches a byte from memory at the current Address and updates the necessary variables.
    // @param Cycles The number of cycles taken by the operation (updated by reference).
    // @param memory The memory block from which to fetch the byte.
    // @return The fetched byte value.
    std::uint8_t ReadByte(std::uint32_t& Cycles, std::uint8_t Address, Mem& memory) {
        // Check if PC is within the valid memory range.
        if (Address >= Mem::MAX_MEM) {
            throw std::out_of_range("Invalid memory address");
        }    
        // Fetch byte from memory at the current Address
        std::uint8_t Data = memory[Address];
        // Decrement cycle count.
        Cycles--;
        // Return fetched byte.
        return Data;
    }

    // *** OPCODES ***
    // LDA
    static constexpr std::uint8_t
        INS_LDA_IM = 0xA9,  // Immediate  
        INS_LDA_ZP = 0xA5,  // Zero Page
        INS_LDA_ZPX = 0xB5, // Zero Page X
        INS_JSR = 0x20;     // JSR

    void LDASetStatus() {
        // Set zero flag (Z) if accumulator is 0.
        Z = (A == 0);
        // Set negative flag (N) if most significant bit of accumulator is set.
        N = (A & 0b10000000) > 0;
    }

    // This function executes the CPU instructions for the given number of cycles.
    void Execute(std::uint32_t Cycles, Mem& memory) {
        while (Cycles > 0) {
            // Fetch next instruction from memory.
            std::uint8_t Instruction = FetchByte(Cycles, memory);
            switch (Instruction) {
                case INS_LDA_IM: {
                    // Load value from immediate into the accumulator (A).
                    std::uint8_t Value = FetchByte(Cycles, memory);
                    A = Value;
                    LDASetStatus();
                } break;
                case INS_LDA_ZP:    {
                    // Load value from immediate into the accumulator (A).
                    std::uint8_t ZeroPageAddress = FetchByte(Cycles, memory);
                    A = ReadByte(Cycles, ZeroPageAddress, memory);
                    LDASetStatus();
                } break;
                case INS_LDA_ZPX:    {
                    // Load value from immediate into the accumulator (A).
                    std::uint8_t ZeroPageAddress = FetchByte(Cycles, memory);
                    ZeroPageAddress += X;
                    Cycles--;
                    A = ReadByte(Cycles, ZeroPageAddress, memory);
                    LDASetStatus();
                } break;
                case INS_JSR:   {
                    std::uint16_t SubAddr = FetchWord(Cycles, memory);
                    memory.WriteWord(PC - 1, SP, Cycles);
                    PC = SubAddr;
                    Cycles --;
                    SP ++;
                } break;
                default: {
                    // Handle unknown instruction.
                    std::cout << "Instruction not handled " << static_cast<int>(Instruction) << std::endl;
                } break;
            }
        }
    }
};



// This is the main function of the program.
int main(int argc, const char * argv[]) {
    // Create an instance of the Mem class to represent memory.
    Mem mem;
    // Create an instance of the CPU class to represent the CPU.
    CPU cpu;
    // Reset the CPU state by initialising the member variables and calling the `Initialise` function of the `Mem` object.
    cpu.Reset(mem);

    // START - Inline a little program
    // Load the "Load Accumulator Immediate" opcode into memory at address 0xFFFC.
    mem[0xFFFC] = CPU::INS_JSR;
    // Load the value 0x42 into memory at address 0xFFFD.
    mem[0xFFFD] = 0x42;
    mem[0xFFFE] = 0x42;
    mem[0x4242] = CPU::INS_LDA_IM;
    mem[0x4243] = 0x84;

    // END - Inline a little program

    // Execute the CPU instructions for 2 cycles using the provided memory.
    cpu.Execute(9, mem);

    // Return 0 to indicate successful program execution.
    return 0;
}
