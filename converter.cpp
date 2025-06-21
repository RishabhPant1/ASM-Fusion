#include "converter.h"
#include <sstream>
#include <iomanip>

// Helper to map register 8085 names to C variable names
static string regToC(const string& reg) {
    if (reg == "A") return "regA";
    else if (reg == "B") return "regB";
    else if (reg == "C") return "regC";
    else if (reg == "D") return "regD";
    else if (reg == "E") return "regE";
    else if (reg == "H") return "regH";
    else if (reg == "L") return "regL";
    else if (reg == "M") return "memory[(regH << 8) | regL]";
    else return reg;
}

// Helper to format immediate values
static string formatImmediate(const string& value) {
    if (value.empty()) return "0";
    
    // Handle hex values
    if (value.size() > 2 && value.substr(0, 2) == "0x") {
        return value;
    }
    
    // Handle hex values without 0x prefix
    bool isHex = true;
    for (char c : value) {
        if (!isdigit(c) && !(c >= 'A' && c <= 'F') && !(c >= 'a' && c <= 'f')) {
            isHex = false;
            break;
        }
    }
    
    if (isHex && value.size() <= 4) {
        return "0x" + value;
    }
    
    return value;
}

// MOV R1, R2
static string convertMOV(const ParsedInstruction& instr) {
    if (instr.operands.size() != 2) return "// MOV requires 2 operands";
    return regToC(instr.operands[0]) + " = " + regToC(instr.operands[1]) + ";";
}

// MVI R, data
static string convertMVI(const ParsedInstruction& instr) {
    if (instr.operands.size() != 2) return "// MVI requires 2 operands";
    return regToC(instr.operands[0]) + " = " + formatImmediate(instr.operands[1]) + ";";
}

// LDA addr
static string convertLDA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// LDA requires 1 address operand";
    return "regA = memory[" + formatImmediate(instr.operands[0]) + "];";
}

// STA addr
static string convertSTA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// STA requires 1 address operand";
    return "memory[" + formatImmediate(instr.operands[0]) + "] = regA;";
}

// ADD R
static string convertADD(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// ADD requires 1 operand";
    return "regA = regA + " + regToC(instr.operands[0]) + ";";
}

// SUB R
static string convertSUB(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// SUB requires 1 operand";
    return "regA = regA - " + regToC(instr.operands[0]) + ";";
}

// INR R
static string convertINR(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// INR requires 1 operand";
    return regToC(instr.operands[0]) + "++;";
}

// DCR R
static string convertDCR(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// DCR requires 1 operand";
    return regToC(instr.operands[0]) + "--;";
}

// JMP addr
static string convertJMP(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JMP requires 1 operand";
    return "goto label_" + instr.operands[0] + ";";
}

// JC addr
static string convertJC(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JC requires 1 operand";
    return "if (carry) goto label_" + instr.operands[0] + ";";
}

// JNC addr
static string convertJNC(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JNC requires 1 operand";
    return "if (!carry) goto label_" + instr.operands[0] + ";";
}

// JZ addr
static string convertJZ(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JZ requires 1 operand";
    return "if (zero) goto label_" + instr.operands[0] + ";";
}

// JNZ addr
static string convertJNZ(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JNZ requires 1 operand";
    return "if (!zero) goto label_" + instr.operands[0] + ";";
}

// CALL addr
static string convertCALL(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// CALL requires 1 operand";
    return "label_" + instr.operands[0] + "(); // Call subroutine";
}

// RET
static string convertRET(const ParsedInstruction&) {
    return "return; // Return from subroutine";
}

// ANA R
static string convertANA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// ANA requires 1 operand";
    return "regA = regA & " + regToC(instr.operands[0]) + ";";
}

// ORA R
static string convertORA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// ORA requires 1 operand";
    return "regA = regA | " + regToC(instr.operands[0]) + ";";
}

// XRA R
static string convertXRA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// XRA requires 1 operand";
    return "regA = regA ^ " + regToC(instr.operands[0]) + ";";
}

// CMP R
static string convertCMP(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// CMP requires 1 operand";
    return "zero = (regA == " + regToC(instr.operands[0]) + ") ? 1 : 0; " +
           "carry = (regA < " + regToC(instr.operands[0]) + ") ? 1 : 0; // Compare";
}

// PUSH rp
static string convertPUSH(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// PUSH requires 1 operand";
    return "// PUSH " + instr.operands[0] + " - Stack operation (simplified)";
}

// POP rp
static string convertPOP(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// POP requires 1 operand";
    return "// POP " + instr.operands[0] + " - Stack operation (simplified)";
}

// HLT
static string convertHLT(const ParsedInstruction&) {
    return "printf(\"Program halted\\n\"); exit(0);";
}

// NOP
static string convertNOP(const ParsedInstruction&) {
    return "; // No operation";
}

// Dispatcher
string convertToC(const ParsedInstruction& instr) {
    if (!instr.mnemonic) return "// No mnemonic";
    
    string m = *instr.mnemonic;
    
    if (m == "MOV") return convertMOV(instr);
    if (m == "MVI") return convertMVI(instr);
    if (m == "LDA") return convertLDA(instr);
    if (m == "STA") return convertSTA(instr);
    if (m == "ADD") return convertADD(instr);
    if (m == "SUB") return convertSUB(instr);
    if (m == "INR") return convertINR(instr);
    if (m == "DCR") return convertDCR(instr);
    if (m == "JMP") return convertJMP(instr);
    if (m == "JC") return convertJC(instr);
    if (m == "JNC") return convertJNC(instr);
    if (m == "JZ") return convertJZ(instr);
    if (m == "JNZ") return convertJNZ(instr);
    if (m == "CALL") return convertCALL(instr);
    if (m == "RET") return convertRET(instr);
    if (m == "ANA") return convertANA(instr);
    if (m == "ORA") return convertORA(instr);
    if (m == "XRA") return convertXRA(instr);
    if (m == "CMP") return convertCMP(instr);
    if (m == "PUSH") return convertPUSH(instr);
    if (m == "POP") return convertPOP(instr);
    if (m == "NOP") return convertNOP(instr);
    if (m == "HLT") return convertHLT(instr);
    
    return "// Unsupported instruction: " + m;
}