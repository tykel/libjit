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

#include <stddef.h>

#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
#else
typedef uintptr_t unsigned int *;
typedef intptr_t int *;
#if defined(__x86_64__)
typedef int64_t long;
typedef uint64_t unsigned long;
#else
typedef int64_t long long;
typedef uint64_t unsigned long long;
#endif
typedef int32_t int;
typedef uint32_t unsigned int;
typedef int16_t short;
typedef uint16_t unsigned short;
typedef int8_t char;
typedef uint8_t unsigned char;
#endif

typedef uint32_t jit_error;

typedef intptr_t jit_memory;


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
    JIT_ERROR_NO_MORE_VREGS,    // Somehow, we used over 2 billion jit registers
    JIT_ERROR_NULL_PTR,
    JIT_ERROR_MALLOC,
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
};

typedef enum e_jit_operand jit_operand;

/* Virtual registers in the jit are (near) infinite, and monotonically increase.
 *
 * Valid jit register indexes number from 0 to INT32_MAX. */
enum e_jit_register {
    JIT_REGISTER_INVALID = -1,
};

typedef enum e_jit_register jit_register;

enum e_jit_op {
    JIT_OP_INVALID = -1,
    JIT_OP_NOP = 0,
    JIT_OP_MOVE,
    JIT_OP_ADD,
    JIT_OP_SUB,
    JIT_OP_MUL,
    JIT_OP_DIV,
    JIT_OP_SHL,
    JIT_OP_SHR,
    JIT_OP_AND,
    JIT_OP_OR,
    JIT_OP_XOR,
    JIT_OP_CALL,
    JIT_OP_JUMP,
    JIT_OP_JUMP_IF,
    JIT_OP_RET,
};

typedef enum e_jit_op jit_op;

struct jit_pointer {
    intptr_t base;
    size_t index;
    size_t scale;
    size_t offset;
};

union u_jit_operand_union {
    int32_t reg;

    int64_t imm64;
    int32_t imm32;
    int16_t imm16;
    int8_t imm8;

    struct jit_pointer ptr;
};

typedef union u_jit_operand_union jit_operand_union;

struct jit_instruction;
struct jit_instruction {
    jit_op op;

    jit_operand in1_type;
    jit_operand in2_type;
    jit_operand out_type;

    jit_operand_union in1;
    jit_operand_union in2;
    jit_operand_union out;

    uintptr_t branch_dest;

    struct jit_instruction *next;
};

struct jit_state {
    size_t blk_ni;          // Number of instructions in the basic block.
    size_t blk_nb;          // Number of bytes in the basic block.
    struct jit_instruction *blk_is;

    uint8_t *p_bufstart;    // Pointer to start of block code buffer.
    uint8_t *p_bufcur;      // Pointer to current location in code buffer.

    int32_t regcur;         // Current highest jit register index.
    struct jit_instruction *p_icur;  // Last instruction added

    // Internal use
    struct jit_instruction *__g_pipool;
    size_t __g_nipool;
    size_t __g_nicur;
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


/* Allocate a new register and return its index. */

jit_register jit_register_new(struct jit_state *s);


/* Append a given instruction to the state's instrcution sequence. */

struct jit_instruction* jit_instruction_new(struct jit_state *s);

jit_error jit_emit_move(struct jit_state *s, struct jit_instruction *i);
jit_error jit_emit_ret(struct jit_state *s, struct jit_instruction *i);

#endif
