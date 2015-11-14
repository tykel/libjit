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

#include <stdio.h>
#include <stdlib.h>

#include "jit.h"

#define FAILPATH(err) {e=(err);goto l_exit;}

#define NUM_HOST_REGS 16

enum e_jit_x86_64_register {
    JIT_X86_64_REGISTER_INVALID = -1,
    rax = 0, rcx, rdx, rbx, rsp, rbp, rsi, rdi,
    r8, r9, r10, r11, r12, r13, r14, r15,
};

typedef enum e_jit_x86_64_register jit_x86_64_register;

/* The x86_64 variant of the emitter reference in jit_state. */
struct jit_emitter {
    jit_register host_regmap[NUM_HOST_REGS];
};

#define REX(w,r,x,n) (0x40 | (!!(w)<<3) | (!!(r)<<2) | (!!(x)<<1) | (!!(n)))

jit_error
jit_create_emitter(struct jit_state *s)
{
    jit_error e = JIT_SUCCESS;
    size_t n;

    s->p_emitter = malloc(sizeof(struct jit_emitter));
    if(s->p_emitter == NULL) {
        FAILPATH(JIT_ERROR_MALLOC);
    }
    for(n = 0; n < NUM_HOST_REGS; n++) {
        s->p_emitter->host_regmap[n] = JIT_X86_64_REGISTER_INVALID;
    }

l_exit:
    return e;
}

jit_error
jit_destroy_emitter(struct jit_state *s)
{
    free(s->p_emitter);

    return JIT_SUCCESS;
}

jit_x86_64_register
jit_get_mapped_x64_64_register(struct jit_state *s, jit_register reg)
{
    size_t n;
    jit_x86_64_register hostreg = JIT_X86_64_REGISTER_INVALID;
    jit_register *regmap = s->p_emitter->host_regmap;
    
    if(reg == JIT_REGISTER_INVALID) {
        goto l_exit;
    }
    for(n = 0; n < NUM_HOST_REGS; n++) {
        if(regmap[n] == reg) {
            hostreg = regmap[n];
        }
    }

l_exit:
    return hostreg;
}

uint8_t* jit_emit__mov_imm32_to_reg(uint8_t *p, int32_t imm, jit_register reg);
uint8_t* jit_emit__ret(uint8_t *p);

jit_error
jit_emit_move(struct jit_state *s, struct jit_instruction *i)
{
    size_t n;
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    jit_x86_64_register hostreg = JIT_X86_64_REGISTER_INVALID;

    if(i->in1_type == JIT_OPERAND_IMM) {
        if(i->out_type == JIT_OPERAND_REG) {
            hostreg = jit_get_mapped_x64_64_register(s, i->out.reg);
            if(hostreg == JIT_X86_64_REGISTER_INVALID) {
                fprintf(stderr, "jit register %d does not map to a host register\n", i->out.reg);
            }
            s->p_bufcur = jit_emit__mov_imm32_to_reg((uint8_t *)s->p_bufcur,
                    i->in1.imm32, i->out.reg);
        }
    }

    printf("> mov:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

l_exit:
    return e;
}

jit_error
jit_emit_add(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_sub(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_mul(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_div(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_shl(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_shr(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_and(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_or(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_xor(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_call(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_jump(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_jump_if(struct jit_state *s, struct jit_instruction *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_ret(struct jit_state *s, struct jit_instruction *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;

    s->p_bufcur = jit_emit__ret((uint8_t *)s->p_bufcur);

    printf("> ret:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

l_exit:
    return e;
}


/* ---------------------------------------------------------- */
/* Beginning of the emitters tied to specific x86_64 opcodes. */

uint8_t*
jit_emit__mov_imm32_to_reg(uint8_t *p, int32_t imm, jit_register reg)
{
    if(reg & 0x8) *p++ = REX(0, 0, 0, reg & 0x8);
    *p++ = 0xb8 + (reg & 0x7);
    *(int32_t *)p = imm;
    p += sizeof(uint32_t);

    return p;
}

uint8_t*
jit_emit__ret(uint8_t *p)
{
    *p++ = 0xc3;

    return p;
}

