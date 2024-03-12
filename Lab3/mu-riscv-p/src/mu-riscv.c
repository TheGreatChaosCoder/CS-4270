#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-riscv.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-RISCV Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate RISCV for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {

	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */
/***************************************************************/
void rdump() {
	int i;
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */
/***************************************************************/
void handle_command() {
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-RISCV SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll();
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-RISCV! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value;
			NEXT_STATE.HI = hi_reg_value;
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program();
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;

	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}

	/*load program*/
	load_program();

	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */

	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */
/************************************************************/
void WB()
{
	INSTRUCTION_COUNT++;
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */
/************************************************************/
void MEM()
{
	MEM_WB.IR = EX_MEM.IR;
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */
/************************************************************/
void EX()
{
    EX_MEM.IR = IF_EX.IR;
    int opcode = IF_EX.IR & 0x7F;
    int funct3 = (IF_EX.IR >> 12) & 0x7;
    int funct7 = (IF_EX.IR >> 25);

    if (opcode == 0x33) { // R-type instructions
    switch (funct3) {
        case 0x0:
            if (funct7 == 0x00) {
                // add
                EX_MEM.ALUOutput = IF_EX.A + IF_EX.B;
            } else if (funct7 == 0x20) {
                // sub
                EX_MEM.ALUOutput = IF_EX.A - IF_EX.B;
            }
            break;
        case 0x1:
            // sll
            EX_MEM.ALUOutput = IF_EX.A << IF_EX.B;
            break;
        case 0x4:
            // xor
            EX_MEM.ALUOutput = IF_EX.A ^ IF_EX.B;
            break;
        case 0x6:
            if (funct7 == 0x00) {
                // srl
                EX_MEM.ALUOutput = (unsigned int)IF_EX.A >> IF_EX.B;
            } else if (funct7 == 0x20) {
                // sra
                EX_MEM.ALUOutput = IF_EX.A >> IF_EX.B;
            }
            break;
        case 0x7:
            // and
            EX_MEM.ALUOutput = IF_EX.A & IF_EX.B;
            break;
        case 0x5:
            // or
            EX_MEM.ALUOutput = IF_EX.A | IF_EX.B;
            break;
        case 0x2:
            // slt
            EX_MEM.ALUOutput = (IF_EX.A < IF_EX.B) ? 1 : 0;
            break;
        case 0x3:
            // sltu
            EX_MEM.ALUOutput = ((unsigned int)IF_EX.A < (unsigned int)IF_EX.B) ? 1 : 0;
            break;
    }
} 
else if (opcode == 0x03 || opcode == 0x13 || opcode == 0x1B || opcode == 0x67) { // I-type instructions
    switch (funct3) {
        case 0x0:
            if (opcode == 0x03) {
                // lb
                EX_MEM.ALUOutput = IF_EX.A + IF_EX.imm;
            } else if (opcode == 0x13) {
                // addi
                EX_MEM.ALUOutput = IF_EX.A + IF_EX.imm;
            }
            break;
        case 0x4: // XORI
            EX_MEM.ALUOutput = IF_EX.A ^ IF_EX.imm;
            break;
        case 0x6: // ORI
            EX_MEM.ALUOutput = IF_EX.A | IF_EX.imm;
            break;
        case 0x7: // ANDI
            EX_MEM.ALUOutput = IF_EX.A & IF_EX.imm;
            break;
        case 0x1: // SLLI
            EX_MEM.ALUOutput = IF_EX.A << IF_EX.imm;
            break;
        case 0x5: // SRLI and SRAI
            if (IF_EX.imm & 0x400) { // SRAI
                EX_MEM.ALUOutput = ((int32_t)IF_EX.A) >> IF_EX.imm;
            } else { // SRLI
                EX_MEM.ALUOutput = IF_EX.A >> IF_EX.imm;
            }
            break;
        case 0x2: // SLTI
            EX_MEM.ALUOutput = ((int32_t)IF_EX.A < (int32_t)IF_EX.imm) ? 1 : 0;
            break;
        case 0x3: // SLTIU
            EX_MEM.ALUOutput = (IF_EX.A < IF_EX.imm) ? 1 : 0;
            break;
    }
}
else if (opcode == 0x23) { // S-type instructions
    switch (funct3) {
        case 0x0: // SB
            EX_MEM.ALUOutput = IF_EX.A + IF_EX.imm;
            break;
		case 0x1: // SH
			EX_MEM.ALUOutput = IF_EX.A + IF_EX.imm;
			break;
		case 0x2: // SW
			EX_MEM.ALUOutput = IF_EX.A + IF_EX.imm;
			break;
    }
} else if (opcode == 0x63) { // B-type instructions
    switch (funct3) {
        case 0x0: // BEQ
            if (IF_EX.A == IF_EX.B) {
                EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
            } else {
                EX_MEM.ALUOutput = IF_EX.PC + 4;
            }
            break;
        // ... add other B-type instructions here ...
		case 0x1: // BNE
			if (IF_EX.A != IF_EX.B) {
				EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
			} else {
				EX_MEM.ALUOutput = IF_EX.PC + 4;
			}
			break;
		case 0x4: // BLT
			if ((int32_t)IF_EX.A < (int32_t)IF_EX.B) {
				EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
			} else {
				EX_MEM.ALUOutput = IF_EX.PC + 4;
			}
			break;
		case 0x5: // BGE
			if ((int32_t)IF_EX.A >= (int32_t)IF_EX.B) {
				EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
			} else {
				EX_MEM.ALUOutput = IF_EX.PC + 4;
			}
			break;
		case 0x6: // BLTU
			if (IF_EX.A < IF_EX.B) {
				EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
			} else {
				EX_MEM.ALUOutput = IF_EX.PC + 4;
			}
			break;
		case 0x7: // BGEU
			if (IF_EX.A >= IF_EX.B) {
				EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
			} else {
				EX_MEM.ALUOutput = IF_EX.PC + 4;
			}
			break;
    }
} else if (opcode == 0x37 || opcode == 0x17) { // U-type instructions
    // LUI or AUIPC
    EX_MEM.ALUOutput = IF_EX.imm;
} else if (opcode == 0x6F) { // J-type instructions
    // JAL
    EX_MEM.ALUOutput = IF_EX.PC + IF_EX.imm;
}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */
/************************************************************/
void ID()
{
	IF_EX.IR = ID_IF.IR;
    printf("%d\n", IF_EX.IR);
    // Extract rs and rt from IR
    int rs = (IF_EX.IR >> 15) & 0x1F;
    int rt = (IF_EX.IR >> 20) & 0x1F;

    // Read values from register file
    IF_EX.A = CURRENT_STATE.REGS[rs];
    IF_EX.B = CURRENT_STATE.REGS[rt];

    // Extract opcode from IR
    int opcode = IF_EX.IR & 0x7F;

    // Sign-extend the lower 16 bits of IR only for instructions that use an immediate value
    if (opcode != 0x33) {
        IF_EX.imm = (IF_EX.IR >> 20);
    } else {
        IF_EX.imm = 0;
    }
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */
/************************************************************/
void IF()
{
	// IR <= Mem[PC]
	ID_IF.IR = mem_read_32(CURRENT_STATE.PC);
	ID_IF.PC = CURRENT_STATE.PC;
	////printf("IR: %08x IR_PC: %08x\n", ID_IF.IR, ID_IF.PC);

	// PC <= PC + 4
	CURRENT_STATE.PC += 4;
	NEXT_STATE = CURRENT_STATE;
	////printf("PC: %08x\n", CURRENT_STATE.PC);
}


/************************************************************/
/* Initialize Memory                                                                                                    */
/************************************************************/
void initialize() {
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in RISCV assembly format)    */
/************************************************************/

//Returns 0 if not an all-zero instruction, 1 otherwise
uint8_t print_instruction(const uint32_t instruction, const uint8_t printAddress, const uint32_t addr){
	uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;

	if (opcode == 00000000) { // stops instructions with all zero bits from being printed
		return 1;
	}

	if(printAddress){
    	printf("%08x: %08x ", addr, instruction);
	}

	uint32_t imm = 0;

    switch (opcode) {
        case 0x33: // R-type
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x00) printf("add ");
                    else if (funct7 == 0x20) printf("sub ");
                    break;
                case 0x1: printf("sll "); break;
                case 0x2: printf("slt "); break;
                case 0x3: printf("sltu "); break;
                case 0x4: printf("xor "); break;
                case 0x5:
                    if (funct7 == 0x00) printf("srl ");
                    else if (funct7 == 0x20) printf("sra ");
                    break;
                case 0x6: printf("or "); break;
                case 0x7: printf("and "); break;
                }
                printf("x%d, x%d, x%d\n", rd, rs1, rs2);
                break;
        case 0x03: // I-type (load)
            imm = (instruction >> 20);
            switch (funct3) {
                case 0x0: printf("lb "); break;
                case 0x1: printf("lh "); break;
                case 0x2: printf("lw "); break;
                case 0x4: printf("lbu "); break;
                case 0x5: printf("lhu "); break;
            }
            printf("x%d, %d(x%d)\n", rd, imm, rs1);
            break;
        case 0x13: // I-type (addi, slti, sltiu, xori, ori, andi, slli, srli, srai)
            imm = (instruction >> 20);
            switch (funct3) {
                case 0x0: printf("addi "); break;
                case 0x2: printf("slti "); break;
                case 0x3: printf("sltiu "); break;
                case 0x4: printf("xori "); break;
                case 0x6: printf("ori "); break;
                case 0x7: printf("andi "); break;
                case 0x1: printf("slli "); break;
                case 0x5:
                    if (funct7 == 0x00) printf("srli ");
                    else if (funct7 == 0x20) printf("srai ");                        
					break;
                }
            printf("x%d, x%d, %d\n", rd, rs1, imm);
            break;
        case 0x23: // S-type
            imm = ((instruction >> 25) << 5) | ((instruction >> 7) & 0x1F);
            switch (funct3) {
                case 0x0: printf("sb "); break;
                case 0x1: printf("sh "); break;
                case 0x2: printf("sw "); break;
			}
            printf("x%d, %d(x%d)\n", rs2, imm, rs1);
            break;

        default:
            printf("unknown instruction\n");
    }
	return 0;
}

void print_program(){
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	uint32_t addr;

    for(addr = CURRENT_STATE.PC; addr < MEM_TEXT_END; addr += 4){
        uint32_t instruction = mem_read_32(addr);

        print_instruction(instruction, TRUE, addr);
    }
}

/************************************************************/
/* Print the current pipeline                               */
/************************************************************/
void show_pipeline(){
	const char allZeroInstruction[] = "No Instruction Loaded";

	printf("Curent PC		%i\n", CURRENT_STATE.PC);
	printf("IF/ID.IR		");
	print_instruction(ID_IF.IR, FALSE, 0) == 0 ? printf(" ") : printf("%s\n", allZeroInstruction);
	printf("IF/ID.PC		%i\n", ID_IF.PC);

	printf("\n");

	printf("ID/EX.IR		");
	print_instruction(IF_EX.IR, FALSE, 0) == 0 ? printf(" ") : printf("%s\n", allZeroInstruction);
	printf("ID/EX.A			%i\n", IF_EX.A);
	printf("ID/EX.B			%i\n", IF_EX.B);
	printf("ID/EX.imm		%i\n", IF_EX.imm);

	printf("\n");

	printf("EX/MEM.IR		"); 
	print_instruction(EX_MEM.IR, FALSE, 0) == 0 ? printf(" ") : printf("%s\n", allZeroInstruction);
	printf("EX/MEM.A		%i\n", EX_MEM.A);
	printf("EX/MEM.B		%i\n", EX_MEM.B);
	printf("EX/MEM.ALUOutput	%i\n", EX_MEM.ALUOutput);

	printf("\n");

	printf("MEM/WB.IR		");
	print_instruction(MEM_WB.IR, FALSE, 0) == 0 ? printf(" ") : printf("%s\n", allZeroInstruction);
	printf("MEM/WB.ALUOutput	%i\n", MEM_WB.ALUOutput);
	printf("MEM/WB.LMD		%i\n", MEM_WB.LMD);
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {
	printf("\n**************************\n");
	printf("Welcome to MU-RISCV SIM...\n");
	printf("**************************\n\n");

	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
