#include <stdio.h>
#include <stddef.h>

#define INSTR_NAME_SIZE 16
#define INSTR_TOTAL_SIZE 128
#define OP_STR_SIZE 16

typedef struct InstrStats {
  size_t issues;
  size_t executes;
  size_t mem_read;
  size_t cdb_write;
  size_t commits;
} InstrStats;

typedef enum op_t {
  STORE, LOAD, ARITHMETIC, BRANCH
} op_t;

typedef struct Instr {
    char str[INSTR_TOTAL_SIZE];
    op_t op_type;

    int fp;
    
    unsigned int op1;
    unsigned int op2;
    unsigned int op3;

    InstrStats stats;
} Instr;

Instr* instr_parse(const char* instr_str);

//
// DEBUG
//

// prints debug information about 'instr'
// if stats is non-zero, will also print stat information
void instr_print(const Instr* instr, int stats);
