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

#ifdef __CPLUSPLUS
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libjit.h"

#define FAILPATH(err) {e=(err);goto l_exit;}

#define __JIT_POOL_ALLOC 1024


jit_error
jit_create(struct jit_state **s, jit_flags flags)
{
    jit_error e = JIT_SUCCESS;

    if(s == NULL) {
        FAILPATH(JIT_ERROR_NULL_PTR);
    }

    *s =(struct jit_state *) calloc(1, sizeof(struct jit_state));
    if(*s == NULL) {
        FAILPATH(JIT_ERROR_MALLOC);
    }

    (*s)->p_ipool = (struct jit_instr *) calloc(__JIT_POOL_ALLOC,
            sizeof(struct jit_instr));
    if((*s)->p_ipool == NULL) {
        FAILPATH(JIT_ERROR_MALLOC);
    }
    (*s)->nipool = __JIT_POOL_ALLOC;

    e = jit_create_emitter(*s);

l_exit:
    return e;
}

jit_error
jit_destroy(struct jit_state *s)
{
    jit_destroy_emitter(s);
    free(s->p_ipool);
    free(s);

    return JIT_SUCCESS;
}

jit_reg
jit_reg_new(struct jit_state *s)
{
    jit_reg r = JIT_REG_INVALID;

    if(s->regcur < INT32_MAX) { 
        r = s->regcur++;
    }
    //printf("new vreg %d created\n", r);

    return r;
}

jit_reg
jit_reg_new_fixed(struct jit_state *s, int32_t map)
{
    jit_reg r = jit_reg_new(s);
    jit_set_reg_mapping(s, r, map);
    return r;
}

struct jit_instr*
jit_instr_new(struct jit_state *s)
{
    struct jit_instr *i = NULL;

    // Grow the pool if needed.
    if(s->nicur == s->nipool) {
        s->nipool += __JIT_POOL_ALLOC;
        s->p_ipool = (struct jit_instr *) realloc(s->p_ipool,
                s->nipool * __JIT_POOL_ALLOC);
        if(s->p_ipool == NULL) {
            goto l_exit;
        }
    }

    //printf("creating instr %zu\n", s->blk_ni);
    i = &s->p_ipool[s->nicur];
    if(s->p_icur) {
        s->p_icur->next = i; 
    }
    s->nicur++;
    s->blk_ni++;
    
    if(s->blk_is == NULL) {
        s->blk_is = i;
    }

    s->p_icur = i;

l_exit:
    return i;
}

jit_label
jit_label_here(struct jit_state *s)
{
    return (jit_label)s->blk_ni;
}

jit_error
jit_begin_block(struct jit_state *s, void *buf)
{
    jit_error e = JIT_SUCCESS;

    s->p_bufstart = s->p_bufcur = (uint8_t *) buf;
    s->blk_ni = s->blk_nb = 0;
    s->blk_is = s->p_ipool;
    
    return e;
}

jit_error
jit_end_block(struct jit_state *s)
{
    jit_error e = JIT_SUCCESS;

    return e;
}

jit_error
jit_reg_life(struct jit_state *s, jit_reg reg, size_t *start,
        size_t *end)
{
    struct jit_instr *i = NULL;
    jit_error e = JIT_SUCCESS;
    size_t n, first, last;

    if(s == NULL || start == NULL || end == NULL) {
        FAILPATH(JIT_ERROR_NULL_PTR);
    }

    for(i = s->blk_is, n = 0, first = SIZE_MAX, last = SIZE_MAX;
            i != NULL;
            i = i->next, n++) {
        if(i->out_type == JIT_OPERAND_REG && i->out.reg == reg) {
            if(first == SIZE_MAX) {
                first = n;
                last = n;
            } else {
                break;
            }
        }

        if(i->in1_type == JIT_OPERAND_REG && i->in1.reg == reg) {
            last = n;
        }
        if(i->in2_type == JIT_OPERAND_REG && i->in2.reg == reg) {
            last = n;
        }
    }
    if(first == SIZE_MAX) {
        FAILPATH(JIT_ERROR_VREG_NOT_FOUND);
    }

    *start = first;
    *end = last;

    printf("register %d: live from insn %zu to %zu\n", reg, first, last);

l_exit:
    return e;
}

jit_error
jit_emit_all(struct jit_state *s)
{
    jit_error e = JIT_SUCCESS;
    struct jit_instr *i = s->blk_is;

    while(i != NULL) {
        e = jit_emit_instr(s, i);
        if(e != JIT_SUCCESS) {
            break;
        }
        i = i->next;
    }
    return e;
}

#ifdef __CPLUSPLUS
}
#endif

