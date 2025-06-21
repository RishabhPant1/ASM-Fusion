// Auto-generated C code from ASM Fusion
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 8085 Register declarations
uint8_t regA = 0, regB = 0, regC = 0, regD = 0;
uint8_t regE = 0, regH = 0, regL = 0;
uint8_t memory[65536] = {0}; // 64KB memory
int carry = 0, zero = 0; // Flags

int main() {
    printf("Starting 8085 program execution...\n");

    // ORG 2000 - Set origin address
label_START:
    // MVI requires 2 operands
    // MVI requires 2 operands
    regA = regA + regB;
    zero = (regA == 0) ? 1 : 0; // Update zero flag
    // STA requires 1 address operand
    // MVI requires 2 operands
    regA = regA - regC;
    zero = (regA == 0) ? 1 : 0; // Update zero flag
    // STA requires 1 address operand
    regA++;
    zero = (regA == 0) ? 1 : 0; // Update zero flag
    // STA requires 1 address operand
    printf("Program halted\n"); exit(0);
    // END - End of program

    printf("Program execution completed.\n");
    return 0;
}
