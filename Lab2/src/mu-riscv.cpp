#include <iostream>
#include <string>
#include <cstdint>
#include <bit>
#include <map>

#include "mu-riscv.h"

inline uint32_t R_assemble_instruction(std::string instruction, uint32_t rd, uint32_t rs1, uint32_t rs2){
    uint32_t opcode = 0x33;
    uint32_t f7;
    uint32_t f3;

    //Get funct7 and funct3 for each instruction
    switch (instruction)
    {
    case "add":
        f3 = 0x0; f7 = 0x00;
        break;
    case "sub":
        f3 = 0x0; f7 = 0x20;
    case "sll":
        f3 = 0x1; f7 = 0x00;
    default: //invalid instruction
        return 0;
    }

    //Creating the instruction
    uint32_t assembled_instruction = opcode;
    assemble_instruction |=  (rd << 7);
    assemble_instruction |=  (f3 << 12);
    assemble_instruction |=  (rs1 << 15);
    assemble_instruction |=  (rs2 << 20);
    assemble_instruction |=  (f7 << 25);

    return assemble_instruction;
}

inline uint32_t I_assemble_instruction(std::string instruction, uint32_t imm, uint32_t rd, uint32_t rs1){
    uint32_t opcode;
    uint32_t f3;

    //Get opcode and funct3 for each instruction
    switch (instruction)
    {
    case "addi":
        f3 = 0x0; opcode = 0x13;
        break;
    case "slli":
        f3 = 0x1; opcode = 0x13;
    case "lw":
        f3 = 0x1; opcode = 0x13;
    default: //invalid instruction
        return 0;
    }

    //Creating the instruction
    uint32_t assembled_instruction = opcode;
    assemble_instruction |=  (rd << 7);
    assemble_instruction |=  (f3 << 12);
    assemble_instruction |=  (rs1 << 15);
    assemble_instruction |=  (imm << 20);

    return assemble_instruction;
}

inline uint32_t J_assemble_instruction(std::string instruction, uint32_t imm, uint32_t rd){
    uint32_t opcode;

    //Get opcode for each instruction
    switch (instruction)
    {
    case "jal":
        opcode = 0x6F;
        break;
    case "jalr":
        opcode = 0x67;
    default: //invalid instruction
        return 0;
    }

    uint32_t assembled_instruction = opcode;
    assemble_instruction |=  (rd << 7);
    assemble_instruction |=  ((imm&7F800) << 12); //[19:12] = 0111 1111 1000 0000 0000
    assemble_instruction |=  ((imm&400) << 19); //[11] = 0100 0000 0000
    assemble_instruction |=  ((imm&3FE)<< 20); //[10:1] = 0011 1111 1110
    assemble_instruction |=  ((imm&80000)<< 31); //[20] = 1000 0000 0000 0000 0000

    return assemble_instruction;
}

inline uint32_t parse_number(std::string parameter){
    if(parameter == "zero,"){
        return 0;
    }
    else{
        //this requires c++ 20
        std::erase(parameter, "x")
        std::erase(parameter, ",")
        return std::stoi(parameter);
    }
}


extern "C" uint32_t assemble_instruction(char* instruction, uint_32 address){
    // Map for jump instructions
    static std::map<std::string,int> jump_address_map;

    // Parsing through string and putting the instructions in a vector
    std::stringstream ss(str);
    vector<string> instruction_vector;
 
    while (getline(ss, (std::string) instruction, ' ')) {
        // store token string in the vector
        instruction_vector.push_back(s);
    }

    // Check if it is a comment first
    if(instruction_vector[0][0] == '#'){
        return -1;
    }

    //TODO: add to instruction list

    uint8_t opcode;
    uint8_t f3;
    uint8_t f7;
    uint32_t imm;
    // Check first instruction
    switch(instruction_vector[0]){
        //R-type instructions
        case "add": case "sub": case "sll":
            //read register names
            uint32_t registers[3]; //rd, rs1, rs2
            for(int i = 1; i<=3; i++){
                registers[i-1] = parse_number(instruction_vector[i]);
            }
            return R_assemble_instruction(instruction, registers[0], registers[1], registers[2]);
            break;

    }
}

