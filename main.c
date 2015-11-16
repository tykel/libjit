#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "jit.h"

typedef int (*p_fn)(void);

int dummyfn(int a, int b, int c)
{
    return (a + b + c);
}

int main(int argc, char *argv[])
{
    jit_state *s;
    jit_error e = JIT_SUCCESS;
    struct jit_instruction *i[10];
    jit_register r[10];

    void *buffer = NULL;
    void *abuffer = NULL;
    size_t n = 0;
    int res = -1;

    int *dummy = malloc(10*sizeof(int));

    printf("Creating jit state\n");
    e = jit_create(&s, JIT_FLAG_NONE);
    if(FAILURE(e)) {
        fprintf(stderr, "failed to create jit\n");
        goto l_exit;
    }

    printf("Allocating jit code buffer\n");
    // Allocate 1 page + a page for aligning
    buffer = malloc(4096*1 + 4096);
    if(buffer == NULL) {
        fprintf(stderr, "failed to malloc buffer\n");
        jit_destroy(s);
        goto l_exit;
    }
    abuffer = (void *)(((uintptr_t)buffer + 4096 - 1) & ~(4096 - 1));

    // Create a "movl $100, %eax" type instruction
    printf("Creating jit v-instruction list\n");
   
    for(; n < 10; n++) {
        i[n] = jit_instruction_new(s);
    }
    // memset(s.vm, 0, 320*240);
    // s.bgc = 0;
    
    dummy[5] = 666;

    
    r[0] = jit_register_new(s);
    MOVE_ID_R_32(i[0], dummy, r[0]);
    r[1] = jit_register_new_constrained(s, JIT_REGMAP_CALL_RET);
    MOVE_RP_R_32(i[1], r[0], JIT_REGISTER_INVALID, 1, 5*sizeof(int), r[1]); 
    i[2]->op = JIT_OP_RET;
    

    // Emit code
    printf("Emitting code to jit buffer\n");
    jit_begin_block(s, abuffer);
    jit_emit_move(s, i[0]);
    jit_emit_move(s, i[1]);
    //jit_emit_move(s, i[2]);
    //jit_emit_call(s, i[3]);
    //jit_emit_arith(s, i[4]);
    jit_emit_ret(s, i[2]);
    jit_end_block(s);

    printf("Attempting to mprotect buffer %p (page-aligned from %p)\n",
            abuffer, buffer);
    if(mprotect(abuffer, 4096, PROT_READ | PROT_EXEC) != 0) {
        fprintf(stderr, "failed to mprotect() code buffer\n");
        jit_destroy(s);
        goto l_exit;
    }

    printf("Attempting to call into jit code buffer\n");
    res = ((p_fn)abuffer)();
    printf("> jit code returned %d\n", res);

    printf("\nStatistics:\n");
    printf("- Emit buffer size: %zu bytes\n", (size_t)(4096*1)+4096);
    printf("- Emitted: %zu bytes\n- Emitted: %zu instructions\n",
            s->blk_nb, s->__g_nicur);
    printf("- Instruction buffer size: %zu bytes\n\n", s->__g_nipool); 
    printf("Tearing down\n");
    free(dummy);
    free(buffer);
    jit_destroy(s);
l_exit:
    return e;
}
