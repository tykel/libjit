#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "jit.h"

typedef void (*p_fn)(void);

int main(int argc, char *argv[])
{
    jit_state *s;
    jit_error e = JIT_SUCCESS;
    struct jit_instruction *i;

    void *buffer = NULL;
    void *abuffer = NULL;

    printf("Creating jit state\n");
    e = jit_create(&s, JIT_FLAG_NONE);
    if(FAILURE(e)) {
        fprintf(stderr, "failed to create jit\n");
        goto exit;
    }

    printf("Allocating jit code buffer\n");
    buffer = malloc(4096 * 2);
    if(buffer == NULL) {
        fprintf(stderr, "failed to malloc buffer\n");
        jit_destroy(s);
        goto exit;
    }
    abuffer = (void *)(((uintptr_t)buffer + 4096 - 1) & ~(4096 - 1));

    // Create a "movl $100, %eax" type instruction
    printf("Creating jit v-instruction list\n");
    i = jit_instruction_new(s);
    i->op = JIT_OP_MOVE;
    i->in1_type = JIT_OPERAND_IMM;
    i->in1.imm32 = 100;
    i->out_type = JIT_OPERAND_REG;
    i->out.reg = jit_register_new(s);

    // TODO: Reduce the jit registers to the number in x86_64

    // Emit code
    printf("Emitting code to jit buffer\n");
    jit_begin_block(s, abuffer);
    jit_emit_move(s, i);
    jit_emit_ret(s, i);
    jit_end_block(s);

    printf("Attempting to mprotect buffer %p (page-aligned from %p)\n",
            abuffer, buffer);
    if(mprotect(abuffer, 4096, PROT_READ | PROT_EXEC) != 0) {
        fprintf(stderr, "failed to mprotect() code buffer\n");
        jit_destroy(s);
        goto exit;
    }

    printf("Attempting to call into jit code buffer\n");
    ((p_fn)abuffer)();

    printf("Done. Tearing down\n");
    free(buffer);
    jit_destroy(s);
exit:
    return e;
}
