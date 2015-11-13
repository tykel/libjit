#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "jit.h"

typedef int (*p_fn)(void);

int main(int argc, char *argv[])
{
    jit_state *s;
    jit_error e = JIT_SUCCESS;
    struct jit_instruction *i0, *i1;

    void *buffer = NULL;
    void *abuffer = NULL;
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
    
    i0 = jit_instruction_new(s);
    i0->op = JIT_OP_MOVE;
    i0->in1_type = JIT_OPERAND_IMM;
    i0->in1.imm32 = 100;
    i0->out_type = JIT_OPERAND_REG;
    i0->out.reg = jit_register_new(s);

    i1 = jit_instruction_new(s);
    i1->op = JIT_OP_RET;

    // TODO: Reduce the jit registers to the number in x86_64

    // Emit code
    printf("Emitting code to jit buffer\n");
    jit_begin_block(s, abuffer);
    jit_emit_move(s, i0);
    jit_emit_ret(s, i1);
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
