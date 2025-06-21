#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <cctype>
#include <fstream>
#include <sstream>
#include <set>
#include "asm_fusion.h" 
#include "converter.h"  
using namespace std;

enum class TokenType {
    Mnemonic,
    Register,
    Number,
    Comma,
    Colon,
    OpenBracket,
    CloseBracket,
    Label,
    Directive,
    Identifier,
    EndOfLine,
    Unknown
};

struct Token {
    TokenType type;
    string value;
};

class Lexer {
public:
    explicit Lexer(const string& line) : input(line), pos(0) {}
    
    vector<Token> tokenize() {
        vector<Token> tokens;
        skipWhitespace();
        while (pos < (int) input.length()) {
            char current = input[pos];
            if (isalpha(current) || current == '.' || current == '_') {
                tokens.push_back(readWord());
            } else if (isdigit(current) || (current == '0' && (peekNext() == 'x' || peekNext() == 'X'))) {
                tokens.push_back(readNumber());
            } else if (current == ',') {
                tokens.push_back(Token{TokenType::Comma, ","});
                pos++;
            } else if (current == ':') {
                tokens.push_back(Token{TokenType::Colon, ":"});
                pos++;
            } else if (current == '[') {
                tokens.push_back(Token{TokenType::OpenBracket, "["});
                pos++;
            } else if (current == ']') {
                tokens.push_back(Token{TokenType::CloseBracket, "]"});
                pos++;
            } else {
                tokens.push_back(Token{TokenType::Unknown, string(1, current)});
                pos++;
            }
            skipWhitespace();
        }
        tokens.push_back(Token{TokenType::EndOfLine, ""});
        return tokens;
    }

private:
    string input;
    int pos;
    
    void skipWhitespace() {
        while (pos < (int) input.length() && isspace(input[pos])) pos++;
    }
    
    char peekNext() {
        if (pos + 1 < (int) input.length()) return input[pos + 1];
        return '\0';
    }
    
    bool isHexDigit(char c) {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    }
    
    string toUpper(const string& str) {
        string result;
        for (char c : str) result += toupper(c);
        return result;
    }
    
    bool isRegister(const string &s) {
        static vector<string> regs = {"A","B","C","D","E","H","L","M","SP","PSW","HL","DE","BC"};
        for (const auto& r : regs) if (s == r) return true;
        return false;
    }
    
    bool isMnemonic(const string &s) {
        static vector<string> mnem = {"MOV","MVI","LDA","STA","ADD","SUB","INR","DCR","JMP", "JC","JNC","JZ","JNZ","CALL","RET","NOP","HLT","ANA","ORA","XRA","CMP","PUSH","POP"};
        for (const auto& m : mnem) if (s == m) return true;
        return false;
    }
    
    bool isDirective(const string &s) {
        static vector<string> dir = {"ORG", "DB", "DW", "END"};
        for (const auto& d : dir) if (s == d) return true;
        return false;
    }
    
    Token readWord() {
        int start = pos;
        while (pos < (int) input.length() && (isalnum(input[pos]) || input[pos] == '.' || input[pos] == '_')) pos++;
        string word = input.substr(start, pos - start);
        string wordUpper = toUpper(word);
        
        if (isMnemonic(wordUpper)) return Token{TokenType::Mnemonic, wordUpper};
        if (isRegister(wordUpper)) return Token{TokenType::Register, wordUpper};
        if (isDirective(wordUpper)) return Token{TokenType::Directive, wordUpper};
        
        skipWhitespace();
        if (pos < (int) input.length() && input[pos] == ':') {
            return Token{TokenType::Label, word};
        }
        
        return Token{TokenType::Identifier, word};
    }
    
    Token readNumber() {
        int start = pos;
        if (input[pos] == '0' && (peekNext() == 'x' || peekNext() == 'X')) {
            pos += 2; // skip '0x'
            while (pos < (int) input.length() && isHexDigit(input[pos])) pos++;
        } else {
            while (pos < (int) input.length() && isdigit(input[pos])) pos++;
        }
        string number = input.substr(start, pos - start);
        return Token{TokenType::Number, number};
    }
};

class Parser {
public:
    explicit Parser(const vector<Token>& toks): tokens(toks), pos(0) {}
    
    optional<ParsedInstruction> parseLine() {
        ParsedInstruction instr;
        if (tokens.empty()) return {};
        
        if (pos < (int) tokens.size() && tokens[pos].type == TokenType::Label) {
            instr.label = tokens[pos].value;
            pos++;
            
            if (pos < (int) tokens.size() && tokens[pos].type == TokenType::Colon) {
                pos++;
            }
            
            if (pos >= (int) tokens.size() || tokens[pos].type == TokenType::EndOfLine) {
                return instr;
            }
        }
        
        if (pos < (int) tokens.size()) {
            if (tokens[pos].type == TokenType::Directive) {
                instr.directive = tokens[pos].value;
                pos++;
                parseOperands(instr);
                return instr;
            } else if (tokens[pos].type == TokenType::Mnemonic) {
                instr.mnemonic = tokens[pos].value;
                pos++;
                parseOperands(instr);
                return instr;
            } else {
                cerr << "Error: Expected directive or mnemonic, found '" << tokens[pos].value << "'\n";
                return {};
            }
        }
        
        return instr; 
    }

private:
    const vector<Token>& tokens;
    int pos;
    
    void parseOperands(ParsedInstruction& instr) {
        while (pos < (int) tokens.size()) {
            if (tokens[pos].type == TokenType::EndOfLine) break;
            if (tokens[pos].type == TokenType::Comma) {
                pos++;
                continue;
            }
            if (tokens[pos].type == TokenType::Register || 
                tokens[pos].type == TokenType::Number || 
                tokens[pos].type == TokenType::Identifier) {
                instr.operands.push_back(tokens[pos].value);
                pos++;
            } else {
                cerr << "Warning: Unexpected token '" << tokens[pos].value << "' in operands\n";
                pos++;
            }
        }
    }
};

class ProgramProcessor {
private:
    vector<ParsedInstruction> instructions;
    set<string> labels;
    set<string> jumpTargets;
    
public:
    bool loadProgram(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << endl;
            return false;
        }
        
        string line;
        int lineNum = 0;
        while (getline(file, line)) {
            lineNum++;
            
            if (line.empty() || line[0] == ';') continue;
            
            size_t commentPos = line.find(';');
            if (commentPos != string::npos) {
                line = line.substr(0, commentPos);
            }
            
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty()) continue;
            
            Lexer lexer(line);
            vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            
            auto parsed = parser.parseLine();
            if (parsed) {
                instructions.push_back(*parsed);
                
                if (parsed->label) {
                    labels.insert(*parsed->label);
                }
                
                if (parsed->mnemonic) {
                    string mnemonic = *parsed->mnemonic;
                    if ((mnemonic == "JMP" || mnemonic == "JC" || mnemonic == "JZ" || mnemonic == "CALL") 
                        && !parsed->operands.empty()) {
                        jumpTargets.insert(parsed->operands[0]);
                    }
                }
            } else {
                cerr << "Warning: Failed to parse line " << lineNum << ": " << line << endl;
            }
        }
        
        file.close();
        return true;
    }
    
    void generateCCode(const string& outputFilename) {
        ofstream outFile(outputFilename);
        if (!outFile.is_open()) {
            cerr << "Error: Cannot create output file " << outputFilename << endl;
            return;
        }
        
        outFile << "// Auto-generated C code from ASM Fusion\n";
        outFile << "#include <stdio.h>\n";
        outFile << "#include <stdlib.h>\n";
        outFile << "#include <stdint.h>\n\n";
        
        outFile << "// 8085 Register declarations\n";
        outFile << "uint8_t regA = 0, regB = 0, regC = 0, regD = 0;\n";
        outFile << "uint8_t regE = 0, regH = 0, regL = 0;\n";
        outFile << "uint8_t memory[65536] = {0}; // 64KB memory\n";
        outFile << "int carry = 0, zero = 0; // Flags\n\n";
        
        if (!jumpTargets.empty()) {
            outFile << "// Function declarations\n";
            for (const string& target : jumpTargets) {
                if (labels.count(target)) {
                    outFile << "void label_" << target << "();\n";
                }
            }
            outFile << "\n";
        }
        
        outFile << "int main() {\n";
        outFile << "    printf(\"Starting 8085 program execution...\\n\");\n\n";
        
        for (const auto& instr : instructions) {
            
            if (instr.label) {
                outFile << "label_" << *instr.label << ":\n";
            }
            
            if (instr.directive) {
                string directive = *instr.directive;
                if (directive == "ORG" && !instr.operands.empty()) {
                    outFile << "    // ORG " << instr.operands[0] << " - Set origin address\n";
                } else if (directive == "DB" && !instr.operands.empty()) {
                    outFile << "    // DB " << instr.operands[0] << " - Define byte\n";
                } else if (directive == "DW" && !instr.operands.empty()) {
                    outFile << "    // DW " << instr.operands[0] << " - Define word\n";
                } else if (directive == "END") {
                    outFile << "    // END - End of program\n";
                    break;
                }
                continue;
            }
            
            if (instr.mnemonic) {
                string cCode = convertToC(instr);
                outFile << "    " << cCode << "\n";
                
                string mnemonic = *instr.mnemonic;
                if (mnemonic == "ADD" || mnemonic == "SUB" || mnemonic == "INR" || mnemonic == "DCR") {
                    outFile << "    zero = (regA == 0) ? 1 : 0; // Update zero flag\n";
                }
            }
        }
        
        outFile << "\n    printf(\"Program execution completed.\\n\");\n";
        outFile << "    return 0;\n";
        outFile << "}\n";
        
        for (const string& target : jumpTargets) {
            if (labels.count(target)) {
                outFile << "\nvoid label_" << target << "() {\n";
                outFile << "    // Function implementation for label " << target << "\n";
                outFile << "    // Note: This is a placeholder - you may need to implement\n";
                outFile << "    // the actual function body based on your program logic\n";
                outFile << "}\n";
            }
        }
        
        outFile.close();
        cout << "C code generated successfully in " << outputFilename << endl;
    }
    
    void printStatistics() {
        cout << "\nProgram Statistics:\n";
        cout << "Total instructions: " << instructions.size() << endl;
        cout << "Labels found: " << labels.size() << endl;
        cout << "Jump targets: " << jumpTargets.size() << endl;
        
        if (!labels.empty()) {
            cout << "Labels: ";
            for (const string& label : labels) {
                cout << label << " ";
            }
            cout << endl;
        }
    }
};

int main() {
    cout << "ASM Fusion - Assembly to C Converter (Full Program Mode)\n";
    cout << "=========================================================\n\n";
    
    string inputFile = "input.asm";
    string outputFile = "output.c";
    
    cout << "Reading assembly program from: " << inputFile << endl;
    
    ProgramProcessor processor;
    
    if (!processor.loadProgram(inputFile)) {
        cerr << "Failed to load program. Make sure " << inputFile << " exists." << endl;
        return 1;
    }
    
    cout << "Program loaded successfully!" << endl;
    processor.printStatistics();
    
    cout << "\nGenerating C code..." << endl;
    processor.generateCCode(outputFile);
    
    cout << "\nConversion completed! Check " << outputFile << " for the generated C code." << endl;
    
    return 0;
}