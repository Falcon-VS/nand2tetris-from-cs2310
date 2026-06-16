#include <bits/stdc++.h>
#include <cstdlib>
#include <unordered_map>
using namespace std;

#define DEBUG 0 
#define dcout if(DEBUG) cout

string to_binary(int x){
	string res = "";
	while(x){
		res = to_string(x%2)+res;
		x=x/2;
	}
	while(res.size()<16)
		res = "0"+res;
	return res;
}
unordered_map<string,int> s_table = {
	{"SP", 0},
	{"LCL", 1},
	{"ARG", 2},
	{"THIS", 3},
	{"THAT", 4},
    {"R0", 0}, {"R1", 1}, {"R2", 2}, {"R3", 3},
	{"R4", 4}, {"R5", 5},{"R6", 6},{"R7", 7},
	{"R8", 8},{"R9", 9},{"R10", 10},{"R11", 11},
    {"R12", 12},{"R13", 13},{"R14", 14},{"R15", 15},
    {"SCREEN", 16384}, {"KBD", 24576}
};

unordered_map<string,string> comp_table = {
	{"0",   "0101010"},
	{"1",   "0111111"},
	{"-1",  "0111010"},
    {"D",   "0001100"}, {"A",   "0110000"},
	{"!D",  "0001101"}, {"!A",  "0110001"}, 
	{"-D",  "0001111"}, {"-A",  "0110011"},
    {"D+1", "0011111"}, {"1+D", "0011111"}, 
	{"A+1", "0110111"}, {"1+A", "0110111"}, 
	{"D-1", "0001110"}, {"-1+D","0001110"},
    {"A-1", "0110010"}, {"-1+A","0110010"},
	{"D+A", "0000010"}, {"A+D", "0000010"},
	{"D-A", "0010011"}, {"-A+D","0010011"},
    {"A-D", "0000111"}, {"-D+A","0000111"},
	{"D&A", "0000000"}, {"A&D", "0000000"},
	{"D|A", "0010101"}, {"A|D", "0010101"},
    {"M",   "1110000"}, {"!M",  "1110001"}, {"-M",  "1110011"},
    {"M+1", "1110111"}, {"1+M", "1110111"},
	{"M-1", "1110010"}, {"-1+M","1110010"},
	{"D+M", "1000010"}, {"M+D", "1000010"},
    {"D-M", "1010011"}, {"-M+D","1010011"},
	{"M-D", "1000111"}, {"-D+M","1000111"},
	{"D&M", "1000000"}, {"M&D", "1000000"},
	{"D|M", "1010101"}, {"M|D", "1010101"},
};

unordered_map<string,string> dest_table = {
	{"",    "000"}, 
	{"M",   "001"}, 
	{"D",   "010"}, 
	{"A",   "100"},
	{"MD",  "011"},{"DM",  "011"},
    {"AM",  "101"},{"MA",  "101"}, 
	{"AD",  "110"},{"DA",  "110"},  
	{"AMD", "111"},{"ADM", "111"},{"MAD", "111"},{"MDA", "111"},{"DAM", "111"},{"DMA", "111"},
};

unordered_map<string,string> jump_table = {
	{"",    "000"}, 
	{"JGT", "001"}, 
	{"JEQ", "010"}, 
	{"JGE", "011"},
    {"JLT", "100"}, 
	{"JNE", "101"},
	{"JLE", "110"}, 
	{"JMP", "111"}
};

string trim(const string& s){
	int start = s.find_first_not_of("\t\r\n");
	int end = s.find_last_not_of("\t\r\n");
	return(start==string::npos) ? "" : s.substr(start,end-start+1);
}

string strip(const string& s){
	string result = "";
	for(char a:s) if(a!=' ') result.push_back(a);
	return result;
}

void first_pass(const vector<string>& lines){
	int rom = 0;
	for(const auto& code : lines){
		if(code.empty()) continue;
		if(code.front()=='(' && code.back()==')'){
			string label = code.substr(1,code.size()-2);
			s_table[label] = rom;
		} else { 

		}
	}
}

int main(){
	
	return(EXIT_SUCCESS);
}
