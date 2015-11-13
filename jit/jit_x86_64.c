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

#include "jit.h"

#define REX(w,r,x,n) (0x40 | (!!(w)<<3) | (!!(r)<<2) | (!!(x)<<1) | (!!(n)))

uint8_t* jit_emit__mov_imm32_to_reg(uint8_t *p, int32_t imm, jit_register reg);
uint8_t* jit_emit__ret(uint8_t *p);

jit_error jit_emit_move(struct jit_state *s, struct jit_instruction *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;

    if(i->in1_type == JIT_OPERAND_IMM) {
        if(i->out_type == JIT_OPERAND_REG) {
            s->p_bufcur = jit_emit__mov_imm32_to_reg((uint8_t *)s->p_bufcur,
                    i->in1.imm32, i->out.reg);
        }
    }

    printf("mov:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

exit:
    return e;
}

jit_error jit_emit_ret(struct jit_state *s, struct jit_instruction *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;

    s->p_bufcur = jit_emit__ret((uint8_t *)s->p_bufcur);

    printf("ret:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

exit:
    return e;
}


/* ---------------------------------------------------------- */
/* Beginning of the emitters tied to specific x86_64 opcodes. */

uint8_t* jit_emit__mov_imm32_to_reg(uint8_t *p, int32_t imm, jit_register reg)
{
    if(reg & 0x8) *p++ = REX(0, 0, 0, reg & 0x8);
    *p++ = 0xb8 + (reg & 0x7);
    *(int32_t *)p = imm;
    p += sizeof(uint32_t);

    return p;
}

uint8_t* jit_emit__ret(uint8_t *p)
{
    *p++ = 0xc3;

    return p;
}

