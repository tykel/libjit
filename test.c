#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "libjit.h"

typedef int (*p_fn)(void);

int dummyfn0(void) { return 1; }
int dummyfn1(int a) { return a + 1; }
int dummyfn2(int a, int b) { return a + b; }
int dummyfn3(int a, int b, int c) { return (a + b + c); }

jit_error test_call(void);
jit_error test_move(void);

#define NUM_INSTRS 5

int main(int argc, char *argv[])
{
    jit_error e = JIT_SUCCESS;
    e = (test_call());
    printf("---- test_call() %s\n\n", SUCCESS(e) ? "pass!" : "fail");
    e = (test_move());
    printf("---- test_move() %s\n\n", SUCCESS(e) ? "pass!" : "fail");
l_exit:
    return e;
}

jit_error test_call(void)
{
#undef NUM_INSTRS
#define NUM_INSTRS 4
    jit_state *s;
    struct jit_instr *i[NUM_INSTRS];
    jit_reg r_sp; 
    jit_error e = JIT_SUCCESS;
    int res = 0;

    void *buffer = NULL;
    void *abuffer = NULL;
    size_t n;
    
    printf("-- test_call\n");
    e = jit_create(&s, JIT_FLAG_NONE);
    buffer = calloc(4096*1 + 4096, 1);
    abuffer = (void *)(((uintptr_t)buffer + 4096 - 1) & ~(4096 - 1));
    for(n = 0; n < NUM_INSTRS; n++) {
        i[n] = jit_instr_new(s);
    }
    
    r_sp = jit_reg_new_fixed(s, JIT_REGMAP_SP);
    PUSH_R(i[0], r_sp, JIT_32BIT);
    CALL_M(i[1], (int32_t*)dummyfn0, JIT_32BIT);
    POP_R(i[2], r_sp, JIT_32BIT);
    RET(i[3]);

    jit_begin_block(s, abuffer);
    for(n = 0; n < NUM_INSTRS; n++) {
        jit_emit_instr(s, i[n]);
    }
    jit_end_block(s);

    mprotect(abuffer, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
    printf("executing code at %p\n", abuffer);
    res = ((p_fn)abuffer)();
    printf("@ expected return %d\n", dummyfn0());
    e = (res == dummyfn0()) ? JIT_SUCCESS : JIT_ERROR_UNKNOWN;
    printf("@ jit code returned %d\n", res);

    free(buffer);
    jit_destroy(s);

    return e;
}

jit_error test_move(void)
{
#undef NUM_INSTRS
#define NUM_INSTRS 5
    jit_state *s;
    jit_error e = JIT_SUCCESS;
    struct jit_instr *i[NUM_INSTRS];
    jit_reg r[10];

    void *buffer = NULL;
    void *abuffer = NULL;
    size_t n = 0;
    int res = -1;
    
    printf("-- test_move\n");
    e = jit_create(&s, JIT_FLAG_NONE);
    buffer = malloc(8192* sizeof(uint8_t));
    abuffer = (void *)(((uintptr_t)buffer + 4096 - 1) & ~(4096 - 1));
    for(n = 0; n < NUM_INSTRS; n++) {
        i[n] = jit_instr_new(s);
    }
    
    r[0] = jit_reg_new_fixed(s, JIT_REGMAP_CALL_ARG0);
    MOVE_I_R(i[0], 1, r[0], JIT_32BIT);
    r[1] = jit_reg_new_fixed(s, JIT_REGMAP_CALL_ARG1);
    MOVE_I_R(i[1], 10, r[1], JIT_32BIT);
    r[2] = jit_reg_new_fixed(s, JIT_REGMAP_CALL_ARG2);
    MOVE_I_R(i[2], 100, r[2], JIT_32BIT);
    CALL_M(i[3], (int32_t*)dummyfn3, JIT_32BIT);
    RET(i[4]);

    jit_begin_block(s, abuffer);
    for(n = 0; n < NUM_INSTRS; n++) {
        jit_emit_instr(s, i[n]);
    }
    jit_end_block(s);

    mprotect(abuffer, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
    printf("executing code at %p\n", abuffer);
    res = ((p_fn)abuffer)();
    printf("@ expected return %d\n", dummyfn3(1, 10, 100));
    e = (res == dummyfn3(1, 10, 100)) ? JIT_SUCCESS : JIT_ERROR_UNKNOWN;
    printf("@ jit code returned %d\n", res);

    free(buffer);
    jit_destroy(s);

    return e;
}

