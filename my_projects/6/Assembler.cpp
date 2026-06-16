#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

// Predefined symbols
unordered_map<string, int> symbolTable = {
    {"SP", 0}, {"LCL", 1}, {"ARG", 2}, {"THIS", 3}, {"THAT", 4},
    {"R0", 0}, {"R1", 1}, {"R2", 2}, {"R3", 3}, {"R4", 4}, {"R5", 5},
    {"R6", 6}, {"R7", 7}, {"R8", 8}, {"R9", 9}, {"R10", 10}, {"R11", 11},
    {"R12", 12}, {"R13", 13}, {"R14", 14}, {"R15", 15},
    {"SCREEN", 16384}, {"KBD", 24576}
};

unordered_map<string, string> compTable = {
    {"0",   "0101010"}, {"1",   "0111111"}, {"-1",  "0111010"},
    {"D",   "0001100"}, {"A",   "0110000"}, {"!D",  "0001101"},
    {"!A",  "0110001"}, {"-D",  "0001111"}, {"-A",  "0110011"},
    {"D+1", "0011111"}, {"A+1", "0110111"}, {"D-1", "0001110"},
    {"A-1", "0110010"}, {"D+A", "0000010"}, {"D-A", "0010011"},
    {"A-D", "0000111"}, {"D&A", "0000000"}, {"D|A", "0010101"},
    {"M",   "1110000"}, {"!M",  "1110001"}, {"-M",  "1110011"},
    {"M+1", "1110111"}, {"M-1", "1110010"}, {"D+M", "1000010"},
    {"D-M", "1010011"}, {"M-D", "1000111"}, {"D&M", "1000000"},
    {"D|M", "1010101"}
};

unordered_map<string, string> destTable = {
		 {"",    "000"}, {"M",   "001"}, {"D",   "010"}, {"MD",  "011"},
		{"A",   "100"}, {"AM",  "101"}, {"AD",  "110"}, {"AMD", "111"}
};

unordered_map<string, string> jumpTable = {
    {"",    "000"}, {"JGT", "001"}, {"JEQ", "010"}, {"JGE", "011"},
    {"JLT", "100"}, {"JNE", "101"}, {"JLE", "110"}, {"JMP", "111"}
};

string trim(const string& s) {
    int start = s.find_first_not_of(" \t\r\n");
    int end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

string removeSpaces(const string& s) {
    string result;
    for (char c : s) if (!isspace(c)) result.push_back(c);
    return result;
}

string toBinary(int n) {
    string res = "";
    for (int i = 15; i >= 0; --i)
        res += ((n >> i) & 1) ? '1' : '0';
    return res;
}	

void label_init(const vector<string>& lines) {
    int rom = 0;
    for (const auto& line : lines) {
        string code = trim(line);
        if (code.empty()) continue;
        if (code.front() == '(' && code.back() == ')') {
            string label = code.substr(1, code.size() - 2);
            symbolTable[label] = rom;
        } else {
            ++rom;
        }
    }
}

vector<string> comment_remove(const string& filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Error: could not open file " << filename << endl;
        exit(1);
    }
    vector<string> lines;
    string line;
    while (getline(fin, line)) {
        // Remove inline comments
        int comment = line.find("//");
        if (comment != string::npos) line = line.substr(0, comment);
        lines.push_back(line);
    }
    return lines;
}

void assemble(const vector<string>& lines, const string& outFile) {
    ofstream fout(outFile);
    if (!fout.is_open()) {
        cerr << "Error: could not write to file " << outFile << endl;
        exit(1);
    }

    int ram = 16;
    for (int lineno = 0; lineno < lines.size(); ++lineno) {
        string code = trim(lines[lineno]);
        if (code.empty()) continue;
        if (code.front() == '(' && code.back() == ')') continue; // skip labels

        if (code[0] == '@') {
            string symbol = trim(code.substr(1));
            int value;
            bool isNumber = !symbol.empty() &&
                            all_of(symbol.begin(), symbol.end(), ::isdigit);
            if (isNumber) {
                value = stoi(symbol);
            } else {
                if (symbolTable.find(symbol) == symbolTable.end()) {
                    symbolTable[symbol] = ram++;
                }
                value = symbolTable[symbol];
            }
            fout << toBinary(value) << "\n";
        } else {
            // C-instruction
            string dest, comp, jump;
            int eq = code.find('=');
            int sc = code.find(';');

            if (eq != string::npos) {
                dest = code.substr(0, eq);
                if (sc != string::npos) {
                    comp = code.substr(eq+1, sc-eq-1);
                    jump = code.substr(sc+1);
                } else {
                    comp = code.substr(eq+1);
                }
            } else {
                dest = "";
                if (sc != string::npos) {
                    comp = code.substr(0, sc);
                    jump = code.substr(sc+1);
                } else {
                    comp = code;
                }
            }

            comp = removeSpaces(trim(comp));
            dest = removeSpaces(trim(dest));
            jump = removeSpaces(trim(jump));

            if (compTable.find(comp) == compTable.end()) {
                cerr << "Error on line " << lineno+1 << ": unknown comp '" << comp << "'\n";
                exit(1);
            }
            if (destTable.find(dest) == destTable.end()) {
                cerr << "Error on line " << lineno+1 << ": unknown dest '" << dest << "'\n";
                exit(1);
            }
            if (jumpTable.find(jump) == jumpTable.end()) {
                cerr << "Error on line " << lineno+1 << ": unknown jump '" << jump << "'\n";
                exit(1);
            }

            fout << "111"
                 << compTable[comp]
                 << destTable[dest]
                 << jumpTable[jump]
                 << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: assembler <file.asm>" << endl;
        return 1;
    }
    string inFile = argv[1];
    string outFile = inFile.substr(0, inFile.find_last_of('.')) + ".hack";
    vector<string> lines = comment_remove(inFile);
    label_init(lines);
    assemble(lines, outFile);
    cout << "Assembly complete: " << outFile << endl;
    return 0;
}



