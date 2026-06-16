#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
namespace fs = std::filesystem;
bool isPunctuation(char ch) {
    static const std::string symbolChars = "{}()[].,;+-*/&|<>=~";
    return symbolChars.find(ch) != std::string::npos;
}
bool isReservedWord(const std::string &word) {
    static const std::set<std::string> keywordSet = {
        "class","constructor","function","method","field","static","var",
        "int","char","boolean","void","true","false","null","this",
        "let","do","if","else","while","return"
    };
    return keywordSet.count(word) > 0;
}
std::string formatForXML(const std::string &inputStr) {
    if (inputStr == "<") return "&lt;";
    if (inputStr == ">") return "&gt;";
    if (inputStr == "&") return "&amp;";
    return inputStr;
}
std::string classifyToken(const std::string &tokenStr) {
    if (isReservedWord(tokenStr)) return "keyword";
    if (tokenStr.size() == 1 && isPunctuation(tokenStr[0])) return "symbol";
    if (isdigit(tokenStr[0])) return "integerConstant";
    if (tokenStr[0] == '"') return "stringConstant";
    return "identifier";
}
void processJackFile(const std::string &inputPath) {
    std::ifstream inputFile(inputPath);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file " << inputPath << "\n";
        return;
    }
    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string fileContent = buffer.str();
    inputFile.close();
    std::vector<std::string> tokenList;
    std::string currentToken;
    bool inStringLiteral = false;
    bool inBlockComment = false;
    for (size_t idx = 0; idx < fileContent.size(); ++idx) {
        char currentChar = fileContent[idx];
        if (!inBlockComment && idx + 1 < fileContent.size() && fileContent[idx] == '/' && fileContent[idx + 1] == '*') {
            inBlockComment = true;
            idx++;
            continue;
        }
        if (inBlockComment && idx + 1 < fileContent.size() && fileContent[idx] == '*' && fileContent[idx + 1] == '/') {
            inBlockComment = false;
            idx++;
            continue;
        }
        if (inBlockComment) {
            continue;
        }
        if (!inStringLiteral && idx + 1 < fileContent.size() && fileContent[idx] == '/' && fileContent[idx + 1] == '/') {
            while (idx < fileContent.size() && fileContent[idx] != '\n') {
                idx++;
            }
            continue;
        }
        if (inStringLiteral) {
            currentToken += currentChar;
            if (currentChar == '"') {
                tokenList.push_back(currentToken);
                currentToken.clear();
                inStringLiteral = false;
            }
            continue;
        }
        if (isspace(currentChar)) {
            if (!currentToken.empty()) {
                tokenList.push_back(currentToken);
                currentToken.clear();
            }
            continue;
        }
        if (currentChar == '"') {
            if (!currentToken.empty()) {
                tokenList.push_back(currentToken);
                currentToken.clear();
            }
            currentToken = "\"";
            inStringLiteral = true;
            continue;
        }
        if (isPunctuation(currentChar)) {
            if (!currentToken.empty()) {
                tokenList.push_back(currentToken);
                currentToken.clear();
            }
            tokenList.push_back(std::string(1, currentChar));
            continue;
        }
        currentToken += currentChar;
    }
    if (!currentToken.empty()) {
        tokenList.push_back(currentToken);
    }
    fs::path filePath(inputPath);
    std::string outputFileName = "My_" + filePath.stem().string() + "T.xml";
    fs::path outputFilePath = filePath.parent_path() / outputFileName;
    std::ofstream outputFile(outputFilePath.string());
    if (!outputFile.is_open()) {
         std::cerr << "Error: Could not write to file " << outputFilePath << "\n";
        return;
    }
    outputFile << "<tokens>\n";
    for (const std::string &token : tokenList) {
        std::string tokenCategory = classifyToken(token);
        std::string tokenValue = token;
        if (tokenCategory == "stringConstant") {
            tokenValue = tokenValue.substr(1, tokenValue.size() - 2);
        }
        outputFile << "<" << tokenCategory << "> "
                   << formatForXML(tokenValue) << " </" << tokenCategory << ">\n";
    }
    outputFile << "</tokens>\n";
    outputFile.close();
    std::cout << "Generated: " << outputFilePath.string() << "\n";
}
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: tokenizer <directory_path>\n";
        return 1;
    }
    std::string directoryPath = argv[1];
    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        if (entry.path().extension() == ".jack") {
            processJackFile(entry.path().string());
        }
    }
    return 0;
}
