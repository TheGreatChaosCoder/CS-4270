#include <unordered_map>
#include <bitset>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

#include "mu-riscv.h"


std::unordered_map<std::string, int> register_map = {
    {"zero", 0}, {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3}, {"x4", 4}, {"x5", 5},
    {"x6", 6}, {"x7", 7}, {"x8", 8}, {"x9", 9}, {"x10", 10}, {"x11", 11},
    {"x12", 12}, {"x13", 13}, {"x14", 14}, {"x15", 15}, {"x16", 16}, {"x17", 17},
    {"x18", 18}, {"x19", 19}, {"x20", 20}, {"x21", 21}, {"x22", 22}, {"x23", 23},
    {"x24", 24}, {"x25", 25}, {"x26", 26}, {"x27", 27}, {"x28", 28}, {"x29", 29},
    {"x30", 30}, {"x31", 31}
};

std::unordered_map<std::string, int> branch_map;

std::string to_binary(int value, int bits) {
    return std::bitset<32>(value).to_string().substr(32 - bits, bits);
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;

    // Iterate through the space-separated tokens
    while (iss >> token) {
        // Ignore tokens starting with #
        if (token[0] == '#') {
            break; // Ignore the rest of the line if a comment is encountered
        }
        token.erase(std::remove(token.begin(), token.end(), ','), token.end());
        token.erase(std::remove(token.begin(), token.end(), ':'), token.end());
        tokens.push_back(token);
    }
    return tokens;
}

inline std::string R_assemble_instruction(std::string instruction, std::string rd, std::string rs1, std::string rs2){
    std::string opcode = "0110011";
    std::string f7, f3;

    //Get funct7 and funct3 for each instruction
    if(instruction == "add"){
        f3 = "000"; f7 = "0000000";
    }
    else if(instruction == "sub"){
        f3 = "000"; f7 = "0100000";
    }
    else if(instruction == "sll"){
        f3 = "001"; f7 = "0000000";
    }
    else{ //invalid instruction
        return NULL;
    }

    //Creating the instruction
    std::string assemble_instruction = f7;
    assemble_instruction += to_binary(register_map[rs2], 5);
    assemble_instruction += to_binary(register_map[rs1], 5);
    assemble_instruction += f3;
    assemble_instruction += to_binary(register_map[rd], 5);
    assemble_instruction += opcode;

    return assemble_instruction;
}

inline std::string B_assemble_instruction(std::string instruction, std::string rs1, std::string rs2, std::string imm){
    std::string opcode = "1100011";
    std::string f3;

    //Get opcode and funct3 for each instruction
    if(instruction == "bge"){
        f3 = "101";
    }
    else{ //invalid instruction
        return NULL;
    }

    uint32_t address = branch_map[imm];

    printf("Branching: %s\n", imm.c_str());
    printf("Branch Address: %i\n", branch_map[imm]);

    //Creating the instruction
    imm = to_binary(address, 13);
    std::string assemble_instruction = imm.substr(13-12-1,1); //[12]
    assemble_instruction += imm.substr(13-10-1,10-5+1); //[10:5]
    assemble_instruction += to_binary(register_map[rs2], 5);
    assemble_instruction += to_binary(register_map[rs1], 5);
    assemble_instruction += f3;
    assemble_instruction += imm.substr(13-4-1,4-1+1); //[4:1]
    assemble_instruction += imm.substr(13-11-1,1); //[11]
    assemble_instruction += opcode;

    return assemble_instruction;
}

inline std::string I_assemble_instruction(std::string instruction, std::string imm, std::string rd, std::string rs1){
    std::string opcode;
    std::string f3;

    //Get opcode and funct3 for each instruction
    if(instruction == "addi"){
        opcode = "0010011"; f3 = "000";
    }
    else if(instruction == "slli"){
        opcode = "0010011"; f3 = "001";
    }
    else if(instruction == "lw"){
        opcode = "0010011"; f3 = "010";
    }
    else{ //invalid instruction
        return NULL;
    }

    //Creating the instruction
    std::string assemble_instruction = to_binary(std::stoi(imm), 12);
    assemble_instruction += to_binary(register_map[rs1], 5);
    assemble_instruction += f3;
    assemble_instruction += to_binary(register_map[rd], 5);
    assemble_instruction += opcode;

    return assemble_instruction;
}

inline std::string J_assemble_instruction(std::string instruction, std::string imm, std::string rd){
    std::string opcode;

    //Get opcode for each instruction
    if(instruction=="jal" || instruction =="j"){
        opcode = "1101111";
    }
    else if(instruction == "jalr"){
        opcode = "1100111";
    }
    else{ //invalid instruction
        return NULL;
    }

    imm = to_binary(std::stoi(imm), 21);
    std::string assemble_instruction = imm.substr(21-20-1,1); //[20]
    assemble_instruction +=  imm.substr(21-10-1,10-1+1); //[10:1]
    assemble_instruction += imm[21-11-1]; //[11]
    assemble_instruction += imm.substr(21-19-1,19-12+1); //[19:12]
    assemble_instruction += to_binary(register_map[rd], 5);
    assemble_instruction += opcode;

    return assemble_instruction;
}

std::string convert_to_machine_code(const std::string instruction, std::string rd, std::string rs1, std::string rs2_or_imm) {
    std::string machine_code_bin;

        //select instruction type
    if(instruction == "add" || instruction == "sub" || instruction == "sll"){
        machine_code_bin = R_assemble_instruction(instruction, rd, rs1, rs2_or_imm);
    }
    else if(instruction == "addi" || instruction == "slli" || instruction == "lw"){
        machine_code_bin = I_assemble_instruction(instruction, rs2_or_imm, rd, rs1);
    } 
    else if(instruction == "jal" || instruction == "j" || instruction == "jalr"){
        machine_code_bin = J_assemble_instruction(instruction, std::to_string(branch_map[rs2_or_imm]), rd);
    }
    else if(instruction == "bge"){
        machine_code_bin = B_assemble_instruction(instruction, rd, rs1, rs2_or_imm); //rd=rs1, rs1=rs2
    }
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << std::stoul(machine_code_bin, nullptr, 2);
    return ss.str();
}

void print_machine_code(const std::vector<std::string>& tokens) {
    //const std::string instruction, std::string rd, std::string rs1, std::string rs2_or_imm
    if(tokens.size() == 4) //Instruction - convert to machine code
    {
        std::cout << convert_to_machine_code(tokens[0], tokens[1], tokens[2], tokens[3]) << std::endl;
    }
    else if(tokens.size() == 3) //immediate - parse to get immediate and rs1
    {
        std::string rs1 = tokens[2].substr(tokens[2].find("(")+1,tokens[2].find(")"));
        std::string imm = tokens[2].substr(0, tokens[2].find("(x"));

        std::cout << convert_to_machine_code(tokens[0], tokens[1], rs1, imm) << std::endl;
    }
    else if(tokens.size() == 2) //j instruction - set rd=x0
    {
        std::cout << convert_to_machine_code(tokens[0], "zero", "None", tokens[1]) << std::endl;
    }
}

void set_branch_label(const std::string& line, const uint32_t program_counter){
    std::vector<std::string> tokens = tokenize(line);
    if(tokens.size() == 1) //Branch label
    {
        branch_map.insert({tokens[0] , program_counter});
    }
}

uint32_t instruction_to_machine_code(const char * instruction, const uint32_t program_counter){
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Please provide an input file." << std::endl;
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file) {
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    std::string line;
    std::map<size_t, std::vector<std::string>>instruction_file;
    uint32_t program_counter = MEM_TEXT_BEGIN;

    while(std::getline(input_file, line)){
        printf("%s\n", line.c_str());
        std::vector<std::string> tokens = tokenize(line);

        if(tokens.size()==1){ //record label for branching
            set_branch_label(line, program_counter);
        }
        else if(tokens.size()>1){ //record tokens for instruction
            instruction_file[program_counter] = tokenize(line);
            program_counter += 4;
        }
    }

    for (auto i = instruction_file.begin(); i != instruction_file.end(); i++)
    {
        print_machine_code(i->second);
    }

    return 0;
}