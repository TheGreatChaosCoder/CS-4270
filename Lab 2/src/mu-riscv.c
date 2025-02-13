#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-riscv.h"
#include "mu-assem.h"

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
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Turn a byte to a word                                                                          */
/***************************************************************/
uint32_t byte_to_word(uint8_t byte)
{
    return (byte & 0x80) ? (byte | 0xffffff80) : byte;
}

/***************************************************************/
/* Turn a halfword to a word                                                                          */
/***************************************************************/
uint32_t half_to_word(uint16_t half)
{
    return (half & 0x8000) ? (half | 0xffff8000) : half;
}

int32_t signExtend_13b(uint32_t number)
{
    // Appending leading zeroes to
    // the 23-bit number
    int32_t ans = number & 0x00001FFF;
 
    // Checking if sign-bit of number is 0 or 1
    if (number & 0x00001000) {
 
        // If number is negative, append
        // leading 1's to the 21-bit sequence
        ans = ans | 0xFFFFE000;
    }
 
    return ans;
}

int32_t signExtend_21b(uint32_t number)
{
    // Appending leading zeroes to
    // the 21-bit number
    int32_t ans = number & 0x001FFFFF;
 
    // Checking if sign-bit of number is 0 or 1
    if (number & 0x00100000) {
 
        // If number is negative, append
        // leading 1's to the 21-bit sequence
        ans = ans | 0xFFE00000;
    }
 
    return ans;
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
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
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

/**************************************************************rdump*/
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
	for (i = 0; i < RISCV_REGS; i++){
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
			runAll(); 
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
	for (i = 0; i < RISCV_REGS; i++){
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
	//char str_buffer[100];
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

void R_Processing(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t f7) {
	switch(f3){
		case 0:
			switch(f7){
				case 0:		//add
					NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] + NEXT_STATE.REGS[rs2];
					break;
				case 32:	//sub
					NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] - NEXT_STATE.REGS[rs2];
					break;
				default:
					RUN_FLAG = FALSE;
					break;
				}	
			break;
		case 6: 			//or
			NEXT_STATE.REGS[rd] = (NEXT_STATE.REGS[rs1] | NEXT_STATE.REGS[rs2]);
			break;
		case 7:				//and
			NEXT_STATE.REGS[rd] = (NEXT_STATE.REGS[rs1] & NEXT_STATE.REGS[rs2]);
			break;
		default:
			RUN_FLAG = FALSE;
			break;
	} 			
}

void ILoad_Processing(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t imm) {
	switch (f3)
	{
	case 0: //lb
		NEXT_STATE.REGS[rd] = byte_to_word((mem_read_32(NEXT_STATE.REGS[rs1] + imm)) & 0xFF);
		break;

	case 1: //lh
		NEXT_STATE.REGS[rd] = half_to_word((mem_read_32(NEXT_STATE.REGS[rs1] + imm)) & 0xFFFF);
		break;

	case 2: //lw
		NEXT_STATE.REGS[rd] = mem_read_32(NEXT_STATE.REGS[rs1] + imm);
		break;
	
	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void Iimm_Processing(uint32_t rd, uint32_t f3, uint32_t rs1, uint32_t imm) {
	uint32_t imm0_4 = (imm << 7) >> 7;
	uint32_t imm5_11 = imm >> 5;

	switch (f3)
	{
	case 0: //addi
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] + imm;
		break;

	case 4: //xori
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] ^ imm;
		break;
	
	case 6: //ori
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] | imm;
		break;
	
	case 7: //andi
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] & imm;
		break;
	
	case 1: //slli
		NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] << imm0_4;
		break;
	
	case 5: //srli and srai
		switch (imm5_11)
		{
		case 0: //srli
			NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] >> imm0_4;
			break;

		case 32: //srai
			//NEXT_STATE.REGS[rd] = NEXT_STATE.REGS[rs1] >> imm0_4;
			break;
		
		default:
			RUN_FLAG = FALSE;
			break;
		}
		break;
	
	case 2:
		break;

	case 3:
		break;

	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void S_Processing(uint32_t imm4, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t imm11) {
	// Recombine immediate
	uint32_t imm = (imm11 << 5) + imm4;

	switch (f3)
	{
	case 0: //sb
		mem_write_32((NEXT_STATE.REGS[rs1] + imm), NEXT_STATE.REGS[rs2]);
		break;
	
	case 1: //sh
		mem_write_32((NEXT_STATE.REGS[rs1] + imm), NEXT_STATE.REGS[rs2]);
		break;

	case 2: //sw
		mem_write_32((NEXT_STATE.REGS[rs1] + imm), NEXT_STATE.REGS[rs2]);
		break;

	default:
		printf("Invalid instruction");
		RUN_FLAG = FALSE;
		break;
	}
}

void B_Processing(uint32_t imm11, uint32_t imm4, uint32_t f3, uint32_t rs1, uint32_t rs2, uint32_t imm10, uint32_t imm12) {
	//recombine immediates
	uint32_t imm = (imm12 << 12) + (imm11 << 11) + (imm10 << 5) + (imm4 << 1);
	int32_t offset = signExtend_13b(imm); //convert to signed

	switch (f3)
	{
		case 5: //bge
			if(NEXT_STATE.REGS[rs1] >= NEXT_STATE.REGS[rs2]){
				NEXT_STATE.PC += offset;
			}
			break;
		case 4: //blt:
			if(NEXT_STATE.REGS[rs1] < NEXT_STATE.REGS[rs2]){
				NEXT_STATE.PC += offset;
			}
			break;
		case 0: // beq
			if(NEXT_STATE.REGS[rs1] == NEXT_STATE.REGS[rs2]){
				NEXT_STATE.PC += offset;
			}
			break;
		default:
			printf("Invalid instruction");
			RUN_FLAG = FALSE;
			break;
	}
	printf("\n b offset = %x", NEXT_STATE.PC);
}

void J_Processing(uint32_t imm20, uint32_t imm10, uint32_t imm11, uint32_t imm19, uint32_t rd) {
	//recombine immediate
	uint32_t imm = (imm20 << 20) + (imm19 << 12) + (imm11 << 11) + (imm10 << 1);
	int32_t offset = signExtend_21b(imm); //convert to signed
	printf("\n%x, %x", imm, offset);

	//jal/j is the only J-type instruction needed
	NEXT_STATE.REGS[rd] = NEXT_STATE.PC + 4;
	NEXT_STATE.PC += offset;

}

void U_Processing() {
	// hi
}

/************************************************************/
/* decode and execute instruction                           */ 
/************************************************************/
void handle_instruction()
{
	// Read the instruction at the address in the program counter
	uint32_t instruction = mem_read_32(CURRENT_STATE.PC);

	// Read opcode
	uint8_t opcode_bitmask = 0x7f; //0111 1111 to low quarter word
	uint8_t opcode = instruction & opcode_bitmask;

	switch(opcode)
	{
	case 0x33: //R-type Instruction
		R_Processing(
			(instruction & 0xF80) >> 7, //rd
			(instruction & 0x7000) >> 12, //f3
			(instruction & 0xF80000) >> 15, //rs1
			(instruction & 0x1F00000) >> 20, //rs2
			(instruction & 0xFE000000) >> 25 //f7
		);
		break;

	case 0x13://I-type Immediate Instruction
		Iimm_Processing(
			(instruction & 0xF80) >> 7, //rd
			(instruction & 0x7000) >> 12, //f3
			(instruction & 0xF80000) >> 15, //rs1
			(instruction & 0xFFF00000) >> 20 //imm
		);
		break;

	case 0x03: //I-type Load Instruction:
		ILoad_Processing(
			(instruction & 0xF80) >> 7, //rd
			(instruction & 0x7000) >> 12, //f3
			(instruction & 0xF80000) >> 15, //rs1
			(instruction & 0xFFF00000) >> 20 //imm
		);
		break;

	case 0x23: //S-type Store Instruction
		S_Processing(
			(instruction & 0xF80) >> 7, // imm[4:0]
			(instruction & 0x7000) >> 12, //f3
			(instruction & 0xF80000) >> 15, //rs1
			(instruction & 0x1F00000) >> 20, //rs2
			(instruction & 0xFE000000) >> 25 // imm[11:5]
		);
		break;
	case 0x63: //B-type Branch Instruction
		B_Processing(
			(instruction & 0x80) >> 7, // imm[11]
			(instruction & 0xF00) >> 8, // imm[4:1]
			(instruction & 0x7000) >> 12, // f3
			(instruction & 0xF80000) >> 15, //rs1
			(instruction & 0x1F00000) >> 20, //rs2
			(instruction & 0x7E000000) >> 25, // imm[10:5]
			(instruction & 0x80000000) >> 31  // imm[12]
		);
		break;
	case 0x6F: //J-type Jump Instruction
		J_Processing(
			(instruction & 0xF80) >> 7, //rd
			(instruction & 0xFF000) >> 12, //imm[19:12]
			(instruction & 0x00100000) >> 20, //imm[11]
			(instruction & 0x7FE00000) >> 21, //imm[10:1]
			(instruction & 0x80000000) >> 31 //imm[20]
		);
		break;

	default: // Unrecognized opcode or zero instruction
		RUN_FLAG = FALSE;
		break;
	}

	//Increment to next instruction
	NEXT_STATE.PC += 4;
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	CURRENT_STATE.REGS[2] = MEM_STACK_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in RISCV assembly format)    */ 
/************************************************************/
void print_program(){
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	uint32_t addr;

	uint32_t MAX_BRANCHES = 10;
	uint32_t branches[MAX_BRANCHES];
	uint32_t numBranches = 0;

	//Have to check for branching beforehand so labels can be generated
	for(addr = CURRENT_STATE.PC; addr < MEM_TEXT_END; addr += 4){
        uint32_t instruction = mem_read_32(addr);

        uint32_t opcode = instruction & 0x7F;
		uint32_t imm = 0;
		int32_t offset;

		if(opcode == 0x63){ //B-type
			imm += ((instruction & 0x80) >> 7) << 11; // imm[11]
			imm += ((instruction & 0xF00) >> 8) << 1; // imm[4:1]
			imm += ((instruction & 0x7E000000) >> 25) << 5; // imm[10:5]
			imm += ((instruction & 0x80000000) >> 31) << 12;  // imm[12]

			offset = signExtend_13b(imm);
		}
		else if(opcode == 0x6F){ //J-type
			imm += ((instruction & 0xFF000) >> 12) << 12; //imm[19:12]
			imm += ((instruction & 0x00100000) >> 20) << 11; //imm[11]
			imm += ((instruction & 0x7FE00000) >> 21) << 1; //imm[10:1]
			imm += ((instruction & 0x80000000) >> 31) << 20; //imm[20]

			offset = signExtend_21b(imm);
		}
		else{
			continue;
		}

		branches[numBranches] = addr + offset; //imm is the offset
		numBranches++;

		if(numBranches == MAX_BRANCHES){
			printf("Too many branches for print_program() to handle \n");
			return;
		}
	}

    for(addr = CURRENT_STATE.PC; addr < MEM_TEXT_END; addr += 4){
        uint32_t instruction = mem_read_32(addr);

        uint32_t opcode = instruction & 0x7F;
        uint32_t rd = (instruction >> 7) & 0x1F;
        uint32_t funct3 = (instruction >> 12) & 0x7;
        uint32_t rs1 = (instruction >> 15) & 0x1F;
        uint32_t rs2 = (instruction >> 20) & 0x1F;
        uint32_t funct7 = (instruction >> 25) & 0x7F;

        uint32_t imm=0; // immediate value
		int32_t offset; //offset for jumps
		uint32_t branchAddress;

		char branchHeader[] = "Br-";

		if (opcode == 00000000) { // kind of hard coded? not sure what to do here but this is a band-aid for now
			break;
		}

        printf("%08x: %08x ", addr, instruction);

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
			case 0x63: // B-type branching
				imm += ((instruction & 0x80) >> 7) << 11; // imm[11]
				imm += ((instruction & 0xF00) >> 8) << 1; // imm[4:1]
				imm += ((instruction & 0x7E000000) >> 25) << 5; // imm[10:5]
				imm += ((instruction & 0x80000000) >> 31) << 12;  // imm[12]

				switch (funct3){
					case 0x0: printf("beq "); break;
					case 0x1: printf("bne "); break;
					case 0x4: printf("blt "); break;
					case 0x5: printf("bge "); break;
					case 0x6: printf("bltu "); break;
					case 0x7: printf("bgeu "); break;
				}

				offset = signExtend_13b(imm);
				
				//convert from unsigned to signed
				branchAddress = addr + offset + 4;

				printf("x%d, x%d, %s%x\n", rs1, rs2, branchHeader, branchAddress);
				break;

			case 0x6F: // J-type instruction (only jal)
				imm += ((instruction & 0xFF000) >> 12) << 12; //imm[19:12]
				imm += ((instruction & 0x00100000) >> 20) << 11; //imm[11]
				imm += ((instruction & 0x7FE00000) >> 21) << 1; //imm[10:1]
				imm += ((instruction & 0x80000000) >> 31) << 20; //imm[20]

				offset = signExtend_21b(imm);
				
				//convert from unsigned to signed
				branchAddress = addr + offset + 4;

				printf("jal x%d, %s%x\n", rd, branchHeader, branchAddress);
				break;

            default:
                printf("unknown instruction\n");
        }

		//Check to see if a branch is for this value of the PC
		for(int i = 0; i<numBranches; i++){
			// plus 4 to consider pc incrementing after j/b instruction ran
			if(branches[i] + 4 == addr){ 
				printf("%s%x\n", branchHeader, addr);
				break; // don't need to print out multiple branches
			}
		}
    }
}




/************************************************************/
/* Print the instruction at given memory address (in RISCV assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){

	uint32_t instruction = mem_read_32(addr);
		uint32_t maskopcode = 0x7F;
		uint32_t opcode = instruction & maskopcode;
		if(opcode == 51) { //R-type
			uint32_t maskrd = 0xF80;
			uint32_t rd = instruction & maskrd;
			rd = rd >> 7;
			uint32_t maskf3 = 0x7000;
			uint32_t f3 = instruction & maskf3;
			f3 = f3 >> 12;
			uint32_t maskrs1 = 0xF8000;
			uint32_t rs1 = instruction & maskrs1;
			rs1 = rs1 >> 15;
			uint32_t maskrs2 = 0x1F00000;
			uint32_t rs2 = instruction & maskrs2;
			rs2 = rs2 >> 20;
			uint32_t maskf7 = 0xFE000000;
			uint32_t f7 = instruction & maskf7;
			f7 = f7 >> 25;
			//R_Print(rd,f3,rs1,rs2,f7);
		} else {
			printf("instruction print not yet created\n");
		}
		CURRENT_STATE = NEXT_STATE;
	return;
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
