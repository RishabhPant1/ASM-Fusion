#include "converter.h"
#include <sstream>
#include <iomanip>

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

static string formatImmediate(const string& value) {
    if (value.empty()) return "0";
    
    if (value.size() > 2 && value.substr(0, 2) == "0x") {
        return value;
    }
    
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

static string convertMOV(const ParsedInstruction& instr) {
    if (instr.operands.size() != 2) return "// MOV requires 2 operands";
    return regToC(instr.operands[0]) + " = " + regToC(instr.operands[1]) + ";";
}

static string convertMVI(const ParsedInstruction& instr) {
    if (instr.operands.size() != 2) return "// MVI requires 2 operands";
    return regToC(instr.operands[0]) + " = " + formatImmediate(instr.operands[1]) + ";";
}

static string convertLDA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// LDA requires 1 address operand";
    return "regA = memory[" + formatImmediate(instr.operands[0]) + "];";
}

static string convertSTA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// STA requires 1 address operand";
    return "memory[" + formatImmediate(instr.operands[0]) + "] = regA;";
}

static string convertADD(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// ADD requires 1 operand";
    return "regA = regA + " + regToC(instr.operands[0]) + ";";
}

static string convertSUB(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// SUB requires 1 operand";
    return "regA = regA - " + regToC(instr.operands[0]) + ";";
}

static string convertINR(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// INR requires 1 operand";
    return regToC(instr.operands[0]) + "++;";
}

static string convertDCR(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// DCR requires 1 operand";
    return regToC(instr.operands[0]) + "--;";
}

static string convertJMP(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JMP requires 1 operand";
    return "goto label_" + instr.operands[0] + ";";
}

static string convertJC(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JC requires 1 operand";
    return "if (carry) goto label_" + instr.operands[0] + ";";
}

static string convertJNC(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JNC requires 1 operand";
    return "if (!carry) goto label_" + instr.operands[0] + ";";
}

static string convertJZ(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JZ requires 1 operand";
    return "if (zero) goto label_" + instr.operands[0] + ";";
}

static string convertJNZ(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// JNZ requires 1 operand";
    return "if (!zero) goto label_" + instr.operands[0] + ";";
}

static string convertCALL(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// CALL requires 1 operand";
    return "label_" + instr.operands[0] + "(); // Call subroutine";
}

static string convertRET(const ParsedInstruction&) {
    return "return; // Return from subroutine";
}

static string convertANA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// ANA requires 1 operand";
    return "regA = regA & " + regToC(instr.operands[0]) + ";";
}

static string convertORA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// ORA requires 1 operand";
    return "regA = regA | " + regToC(instr.operands[0]) + ";";
}

static string convertXRA(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// XRA requires 1 operand";
    return "regA = regA ^ " + regToC(instr.operands[0]) + ";";
}

static string convertCMP(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// CMP requires 1 operand";
    return "zero = (regA == " + regToC(instr.operands[0]) + ") ? 1 : 0; " +
           "carry = (regA < " + regToC(instr.operands[0]) + ") ? 1 : 0; // Compare";
}

static string convertPUSH(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// PUSH requires 1 operand";
    return "// PUSH " + instr.operands[0] + " - Stack operation (simplified)";
}

static string convertPOP(const ParsedInstruction& instr) {
    if (instr.operands.size() != 1) return "// POP requires 1 operand";
    return "// POP " + instr.operands[0] + " - Stack operation (simplified)";
}

static string convertHLT(const ParsedInstruction&) {
    return "printf(\"Program halted\\n\"); exit(0);";
}

static string convertNOP(const ParsedInstruction&) {
    return "; // No operation";
}

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