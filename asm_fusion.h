#ifndef ASM_FUSION_H
#define ASM_FUSION_H
#include <string>
#include <vector>
#include <optional>
using namespace std;
struct ParsedInstruction {
    optional<string> label;
    optional<string> directive;
    optional<string> mnemonic;
    vector<string> operands;
};
#endif 