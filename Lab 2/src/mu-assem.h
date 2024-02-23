#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define MAX_SIZE 255

char prog_file[32];
char memory[MAX_SIZE][MAX_SIZE];

#ifdef __cplusplus
extern 'C'{
#endif
uint32_t instruction_to_machine_code(const char * instruction, const uint32_t program_counter);

#ifdef __cplusplus
}
#endif