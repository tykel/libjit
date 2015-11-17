/*
 * Copyright (c) 2015 Tim Kelsall.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _JIT_H_
#define _JIT_H_

#ifdef __CPLUSPLUS
extern "C" {
#endif

#include <stddef.h>

#ifdef __STDC_VERSION
#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
#else
typedef unsigned int * uintptr_t;
typedef int * intptr_t;
#if __WORDSIZE == 64
#define __INT64_C(c)	c ## L
#define __UINT64_C(c)	c ## UL
#define SIZE_MAX		(18446744073709551615UL)
typedef long int64_t;
typedef unsigned long uint64_t;
#else
#define __INT64_C(c)	c ## LL
#define __UINT64_C(c)	c ## ULL
#ifdef __WORDSIZE32_SIZE_ULONG
#define SIZE_MAX		(4294967295UL)
#else
#define SIZE_MAX		(4294967295U)
#endif
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif
# define INT64_MAX		(__INT64_C(9223372036854775807))
# define UINT64_MAX		(__UINT64_C(18446744073709551615))
typedef int int32_t;
#define INT32_MAX		(2147483647)
typedef unsigned int uint32_t;
# define UINT32_MAX		(4294967295U)
typedef short int16_t;
# define INT16_MAX		(32767)
typedef unsigned short uint16_t;
# define UINT16_MAX		(65535)
typedef char int8_t;
# define INT8_MAX		(127)
typedef unsigned char uint8_t;
# define UINT8_MAX		(255)
#endif
#else
#include <stdint.h>
#endif

typedef uint32_t jit_error;

typedef size_t jit_label;


/* Currently only x86_64 is supported.
 * We only enumerate other targets for potential future usage. */
enum e_jit_targets {
    JIT_TARGET_INVALID = -1,
    JIT_TARGET_X86 = 0,
    JIT_TARGET_X86_64 = 1,
    JIT_TARGET_ARMV7 = 2,
};

enum e_jit_errors {
    JIT_SUCCESS = 0,
    JIT_ERROR_UNKNOWN,
    JIT_ERROR_NO_MORE_VREGS,
    JIT_ERROR_NULL_PTR,
    JIT_ERROR_MALLOC,
    JIT_ERROR_VREG_NOT_FOUND,
    JIT_ERROR_REG_BUSY,
    JIT_MAX,
};

#define SUCCESS(x) ((x) == JIT_SUCCESS)
#define FAILURE(x) (!SUCCESS(x))

enum e_jit_flags {
    JIT_FLAG_NONE = 0,
    JIT_FLAG_MAX = (1 << 31),
};

typedef uint32_t jit_flags;

enum e_jit_operand {
    JIT_OPERAND_INVALID = -1,
    JIT_OPERAND_REG = 0,
    JIT_OPERAND_IMM,
    JIT_OPERAND_REGPTR,
    JIT_OPERAND_IMMPTR,
    JIT_OPERAND_IMMDISP,
};

typedef enum e_jit_operand jit_operand;

/* Virtual registers in the jit are (near) infinite, and monotonically increase.
 *
 * Valid jit register indexes number from 0 to INT32_MAX. */
enum e_jit_reg {
    JIT_REG_INVALID = -1,
};

typedef enum e_jit_reg jit_reg;

enum e_jit_regmap {
    JIT_REGMAP_NONE = 0,
    JIT_REGMAP_CALL_ARG0,
    JIT_REGMAP_CALL_ARG1,
    JIT_REGMAP_CALL_ARG2,
    JIT_REGMAP_CALL_ARG3,
    JIT_REGMAP_CALL_ARG4,
    JIT_REGMAP_CALL_ARG5,
    JIT_REGMAP_CALL_RET,
};

typedef enum e_jit_regmap jit_regmap;

enum e_jit_op {
    JIT_OP_INVALID = -1,
   
    JIT_OP_NOP  = 0,
    JIT_OP_MOVE = 1,
    JIT_OP_ADD  = 2,
    JIT_OP_SUB  = 3,
    JIT_OP_MUL  = 4,
    JIT_OP_DIV  = 5,
    JIT_OP_SHL  = 6,
    JIT_OP_SHR  = 7, 
    JIT_OP_AND  = 8,
    JIT_OP_OR   = 9,
    JIT_OP_XOR  = 10,
    JIT_OP_CALL = 11,
    JIT_OP_JUMP = 12,
    JIT_OP_JUMP_IF = 13,
    JIT_OP_RET  = 14,
    
    JIT_NUM_OPS,
};

typedef enum e_jit_op jit_op;

struct jit_ptr {
    jit_reg base;
    jit_reg index;
    int32_t scale;
    int32_t offset;
};

union u_jit_operand_union {
    int32_t reg;

    int64_t imm64;
    int32_t imm32;
    int16_t imm16;
    int8_t imm8;

    struct jit_ptr regptr;
    int32_t *m32ptr;
    int32_t *m16ptr;
    int32_t *m8ptr;
    
    void *ptr;
};

typedef union u_jit_operand_union jit_operand_union;

struct jit_instr;
struct jit_instr {
    jit_op op;

    jit_operand in1_type;
    jit_operand in2_type;
    jit_operand out_type;

    jit_operand_union in1;
    jit_operand_union in2;
    jit_operand_union out;

    size_t opsz;

    struct jit_instr *next;
};

enum e_jit_opsz {
    JIT_8BIT = 1,
    JIT_16BIT = 2,
    JIT_32BIT = 4,
    JIT_64BIT = 8,
};

#define MOVE_R_R(i,a,b,s) (i)->op=JIT_OP_MOVE; \
    (i)->in1_type=(i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.reg=a; (i)->out.reg=b; (i)->opsz=s
#define MOVE_I_R(i,a,b,s) (i)->op=JIT_OP_MOVE; \
    (i)->in1_type=JIT_OPERAND_IMM; (i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.imm32=a; (i)->out.reg=b; (i)->opsz=s
#define MOVE_ID_R(i,a,b,s) (i)->op=JIT_OP_MOVE; \
    (i)->in1_type=JIT_OPERAND_IMMDISP; (i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.ptr=a; (i)->out.reg=b; (i)->opsz=s
#define MOVE_M_R(i,a,b,s) (i)->op=JIT_OP_MOVE; \
    (i)->in1_type=JIT_OPERAND_IMMPTR; (i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.m32ptr=a; (i)->out.reg=b; (i)->opsz=s
#define MOVE_R_M(i,a,b,s) (i)->op=JIT_OP_MOVE; \
    (i)->in1_type=JIT_OPERAND_REG; (i)->out_type=JIT_OPERAND_IMMPTR; \
    (i)->in1.reg=a; (i)->out.m32ptr=b; (i)->opsz=s
#define MOVE_RP_R(i,b,n,s,o,r,z) (i)->op=JIT_OP_MOVE; \
    (i)->in1_type=JIT_OPERAND_REGPTR; (i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.regptr.base=b; (i)->in1.regptr.index=n; \
    (i)->in1.regptr.scale=s; (i)->in1.regptr.offset=o;\
    (i)->out.reg=r; (i)->opsz=z

#define CALL_M(i,a,s) (i)->op=JIT_OP_CALL; \
    (i)->in1_type=JIT_OPERAND_IMMPTR; (i)->in1.m32ptr=a; (i)->opsz=s

#define OP_R_R_R(i,o,a,b,c,s) (i)->op=(o); \
    (i)->in1_type=(i)->in2_type=(i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.reg=a; (i)->in2.reg=b; (i)->out.reg=c; (i)->opsz=s
#define OP_I_R_R(i,o,a,b,c,s) (i)->op=(o); \
    (i)->in1_type=JIT_OPERAND_IMM; \
    (i)->in2_type=(i)->out_type=JIT_OPERAND_REG; \
    (i)->in1.imm32=a; (i)->in2.reg=b; (i)->out.reg=c; (i)->opsz=s

#define ADD_R_R_R(i,a,b,c,s) OP_R_R_R((i),JIT_OP_ADD,(a),(b),(c),(s))
#define ADD_I_R_R(i,a,b,c,s) OP_I_R_R((i),JIT_OP_ADD,(a),(b),(c),(s))
#define SUB_R_R_R(i,a,b,c,s) OP_R_R_R((i),JIT_OP_SUB,(a),(b),(c),(s))
#define SUB_I_R_R(i,a,b,c,s) OP_I_R_R((i),JIT_OP_SUB,(a),(b),(c),(s))
#define AND_R_R_R(i,a,b,c,s) OP_R_R_R((i),JIT_OP_AND,(a),(b),(c),(s))
#define AND_I_R_R(i,a,b,c,s) OP_I_R_R((i),JIT_OP_AND,(a),(b),(c),(s))
#define XOR_R_R_R(i,a,b,c,s) OP_R_R_R((i),JIT_OP_XOR,(a),(b),(c),(s))
#define XOR_I_R_R(i,a,b,c,s) OP_I_R_R((i),JIT_OP_XOR,(a),(b),(c),(s))


struct jit_emitter;

struct jit_state {
    /* Number of instructions in the basic block.*/
    size_t blk_ni;
    /* Number of bytes in the basic block. */
    size_t blk_nb;
    struct jit_instr *blk_is;

    /* Pointer to start of block code buffer. */
    uint8_t *p_bufstart;
    /* Pointer to current location in code buffer. */
    uint8_t *p_bufcur;

    /* Current highest jit register index. */
    int32_t regcur;
    /* Last instruction added. */
    struct jit_instr *p_icur;

    /* Internal book-keeping. */
    struct jit_instr *__g_pipool;
    size_t __g_nipool;
    size_t __g_nicur;

    struct jit_emitter *p_emitter;
};

/* Provide an alternative typedef for those who don't like typing struct. */
typedef struct jit_state jit_state;


/* Allocate and initialize a new jit state in pointer s, with options defined
 * in flags. */

jit_error jit_create(struct jit_state **s, jit_flags flags);


/* Deallocate the jit state in pointer s. */

jit_error jit_destroy(struct jit_state *s);


/* Set the jit state to start a block and provide the output code buffer. */

jit_error jit_begin_block(struct jit_state *s, void *buf);

jit_error jit_end_block(struct jit_state *s);


/* Allocate a new reg and return its index. */

jit_reg jit_reg_new(struct jit_state *s);

jit_reg jit_reg_new_fixed(struct jit_state *s,
        int32_t map);
jit_error jit_set_reg_mapping(struct jit_state *s,
        jit_reg r, int32_t map);


/* Append a given instruction to the state's instrcution sequence. */

struct jit_instr* jit_instr_new(struct jit_state *s);

jit_label jit_label_here(struct jit_state *s);

jit_error jit_create_emitter(struct jit_state *s);

jit_error jit_destroy_emitter(struct jit_state *s);

/* Return start and end of a register's liveness (or life). */
jit_error jit_reg_life(struct jit_state *s, jit_reg reg, size_t *start, size_t *end);

jit_error jit_emit_instr(struct jit_state *s, struct jit_instr *i);

jit_error jit_emit_move(struct jit_state *s, struct jit_instr *i);
jit_error jit_emit_arith(struct jit_state *s, struct jit_instr *i);
jit_error jit_emit_call(struct jit_state *s, struct jit_instr *i);
jit_error jit_emit_jump(struct jit_state *s, struct jit_instr *i);
jit_error jit_emit_jump_if(struct jit_state *s, struct jit_instr *i);
jit_error jit_emit_ret(struct jit_state *s, struct jit_instr *i);

#ifdef __CPLUSPLUS
}
#endif

#endif
