#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "instr.h"
#include "string.h"

#define _STR(x)  #x
#define STR(X) _STR(x)

// DEBUG PRINTING FOR INSRUCTIONS
void instr_print(const Instr* instr, int stats) {
  printf("instr:  %s\n", instr->str); 
  printf("\top_type: %d\n", instr->op_type);
  printf("\tfp: %d\n", instr->fp);
  printf("\top1: %d\t\nop2:%d\t\nop3:%d\n", instr->op1, instr->op2, instr->op3);
  
  if (!stats) return;

  printf("\tissues: %lu\n", instr->stats.issues);
  printf("\texecutes: %lu\n", instr->stats.executes);
  printf("\tmem_read: %lu\n", instr->stats.mem_read);
  printf("\tcdb_write: %lu\n", instr->stats.cdb_write);
  printf("\tcommits: %lu\n", instr->stats.commits);
}

void instr_free(Instr* instr) {
  free(instr);
}

Instr* instr_parse(const char* instr_str) {
  Instr* instr = calloc(1, sizeof(*instr));
  if (!instr) {
    fprintf(stderr, "Failed to allocate instruction\n");
    return NULL;
  }
  
  // copy in instruction
  strncpy(instr->str, instr_str, INSTR_TOTAL_SIZE - 1);

  char instr_name[INSTR_NAME_SIZE];
  
  // int/float load/stores
  if (sscanf(instr_str, "%s %*c%u,%*u(%*c%u):%u", instr_name, &instr->op1, &instr->op2, &instr->op3) == 4) {
    int len = strlen(instr_name);
    int pos = 0;
    
    // determine where to look for op type (floating vs int)
    if (len < 2) {
      fprintf(stderr, "load/store is shorter than expected, treating as store\n");
      goto parse_instr_fail;
    }

    // deciding character is right before the 'w'. i.e. lw, sw, flw, fsw
    pos = len - 2;
    
    // determine op type
    switch (instr_name[pos]) {
      case 's':
        instr->op_type = STORE;
        break;
      case 'l':
        instr->op_type = LOAD;
        break;
      default:
        fprintf(stderr, "unexpected instruction parsing load/store\n");
        goto parse_instr_fail;
    }

    fprintf(stderr, "load store: %s\n", instr_name); 
  }
  // arithmetic
  else if (sscanf(instr_str, "%s %*c%u,%*c%u,%*c%u", instr_name, &instr->op1, &instr->op2, &instr->op3) == 4) {
    instr->op_type = ARITHMETIC;
    fprintf(stderr, "arithmetic\n");
  }
  // branch
  else if (sscanf(instr_str, "%s x%u,x%u,%*s", instr_name, &instr->op1, &instr->op2) == 3) {
    instr->op_type = BRANCH;
    fprintf(stderr, "branch\n");
  }
  // unrecognized
  else {
    fprintf(stderr, "instr not recognized\n");
    goto parse_instr_fail;
  }

  // determine whether the instruction is floating point
  instr->fp = (instr_name[0] == 'f');

  instr_print(instr, 0);
  
  goto parse_instr_clean;

parse_instr_fail:
  if (instr) instr_free(instr);
parse_instr_clean:
  return instr;
}



