#pragma once
#include <stddef.h>
#include <stdbool.h>

#define INSTR_NAME_SIZE 16
#define INSTR_TOTAL_SIZE 128
#define OP_STR_SIZE 16

typedef struct InstrStats InstrStats;
typedef struct Instr Instr;

struct InstrStats {
  size_t issue;
  size_t execute_start;
  size_t execute_end;
  size_t mem_read;
  size_t cdb_write;
  size_t commit;
};

enum op_t {
  STORE = 0, LOAD = 1,
  ADD = 2, SUB = 3,
  FMUL = 4, FDIV = 5,
  FADD = 6, FSUB = 7,
  BRANCH = 8
};

struct Instr {
    char str[INSTR_TOTAL_SIZE];
    enum op_t op_type;

    int fp;
    
    unsigned int op1;
    unsigned int op2;
    unsigned int op3;

    InstrStats stats;

    Instr* next;
    Instr* prev;
};

Instr* instr_new(void);
void   instr_free(Instr* instr);
Instr* instr_parse(const char* instr_str);
Instr* instr_sentinel(void);

//
// DEBUG
//

// prints debug information about 'instr'
// if stats is non-zero, will also print stat information
void instr_print(const Instr* instr, int stats);
