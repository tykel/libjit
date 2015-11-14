#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "jit.h"

typedef int (*p_fn)(void);

int main(int argc, char *argv[])
{
    jit_state *s;
    jit_error e = JIT_SUCCESS;
    struct jit_instruction *i[10];
    size_t rlive_start = INT32_MAX, rlive_end = INT32_MAX;

    void *buffer = NULL;
    void *abuffer = NULL;
    size_t n = 0;
    int res = -1;

    printf("Creating jit state\n");
    e = jit_create(&s, JIT_FLAG_NONE);
    if(FAILURE(e)) {
        fprintf(stderr, "failed to create jit\n");
        goto l_exit;
    }

    printf("Allocating jit code buffer\n");
    buffer = malloc(4096 * 2);
    if(buffer == NULL) {
        fprintf(stderr, "failed to malloc buffer\n");
        jit_destroy(s);
        goto l_exit;
    }
    abuffer = (void *)(((uintptr_t)buffer + 4096 - 1) & ~(4096 - 1));

    // Create a "movl $100, %eax" type instruction
    printf("Creating jit v-instruction list\n");
   
    for(; n < 5; n++)
        i[n] = jit_instruction_new(s);
    MOVE_I_R_32(i[0], 100, jit_register_new(s))
    MOVE_I_R_32(i[1], 200, jit_register_new(s))
    ADD_R_R_R_32(i[2], i[0]->out.reg, i[1]->out.reg, i[0]->out.reg)
    ADD_I_R_R_32(i[3], 300, i[1]->out.reg, jit_register_new(s))
    i[4]->op = JIT_OP_RET;

    // TODO: Reduce the jit registers to the number in x86_64
    e = jit_register_life(s, i[0]->out.reg, &rlive_start, &rlive_end);
    if(FAILURE(e)) {
        fprintf(stderr, "Could not get register %d's liveness: %d\n",
                i[0]->out.reg, e);
    }
    e = jit_register_life(s, i[1]->out.reg, &rlive_start, &rlive_end);
    if(FAILURE(e)) {
        fprintf(stderr, "Could not get register %d's liveness: %d\n",
                i[1]->out.reg, e);
    }

    // Temporary while testing register allocation
    //jit_destroy(s);
    //goto l_exit;

    // Emit code
    printf("Emitting code to jit buffer\n");
    jit_begin_block(s, abuffer);
    jit_emit_move(s, i[0]);
    jit_emit_move(s, i[1]);
    jit_emit_add(s, i[2]);
    jit_emit_add(s, i[3]);
    jit_emit_ret(s, i[4]);
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

    printf("Tearing down\n");
    free(buffer);
    jit_destroy(s);
l_exit:
    return e;
}
