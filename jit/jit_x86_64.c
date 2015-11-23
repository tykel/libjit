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

#include "jit_x86_64.h"

static const jit_host_reg g_regmap[] = {
    JIT_HOST_REG_INVALID,
    rdi, rsi, rdx, rcx, r8, r9, rax, rsp,
};

static const char *g_opsz[JIT_NUM_OPS] = {
    "nop", "mov", "add", "sub", "mul", "div", "shl", "shr", "sar", "and",
    "or", "xor", "call", "jump", "jx", "ret"
};

static const char *g_hostregsz[NUM_HOST_REGS] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
};

static const pfn_e_r_to_r g_e_r_to_r[JIT_NUM_OPS] = {
    NULL,
    jit_emit__mov_reg32_to_reg,
    jit_emit__add_reg32_to_reg,
    jit_emit__sub_reg32_to_reg,
    NULL, NULL, NULL, NULL, NULL,
    jit_emit__and_reg32_to_reg,
    jit_emit__or_reg32_to_reg,
    jit_emit__xor_reg32_to_reg,
    NULL, NULL, NULL, NULL, NULL,
};

static const pfn_e_imm32_to_r g_e_imm32_to_r[JIT_NUM_OPS] = {
    NULL,
    jit_emit__mov_imm32_to_reg,
    jit_emit__add_imm32_to_reg,
    jit_emit__sub_imm32_to_reg,
    NULL, NULL,
    jit_emit__shl_imm32_to_reg,
    jit_emit__shr_imm32_to_reg,
    jit_emit__sar_imm32_to_reg,
    jit_emit__and_imm32_to_reg,
    jit_emit__or_imm32_to_reg,
    jit_emit__xor_imm32_to_reg,
    NULL, NULL, NULL, NULL, NULL,
};

static const pfn_e_imm16_to_r g_e_imm16_to_r[JIT_NUM_OPS] = {
    NULL,
    NULL,
    jit_emit__add_imm16_to_reg,
    NULL,
    NULL, NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, NULL, NULL, NULL, NULL,
};

static const pfn_e_imm8_to_r g_e_imm8_to_r[JIT_NUM_OPS] = {
    NULL,
    NULL,
    jit_emit__add_imm8_to_reg,
    NULL,
    NULL, NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, NULL, NULL, NULL, NULL,
};

static const int32_t p_scalemap[] = {
    -1,             // 0
    0,              // 1
    1, -1,          // 2
    2, -1, -1, -1,  // 4
    3               // 8
};

jit_error
jit_create_emitter(struct jit_state *s)
{
    jit_error e = JIT_SUCCESS;
    size_t n;

    s->p_emitter = calloc(1, sizeof(struct jit_emitter));
    if(s->p_emitter == NULL) {
        FAILPATH(JIT_ERROR_MALLOC);
    }
    for(n = 0; n < NUM_HOST_REGS; n++) {
        s->p_emitter->host_regmap[n] = JIT_HOST_REG_INVALID;
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

jit_error
jit_set_reg_mapping(struct jit_state *s, jit_reg reg, int32_t map)
{
    jit_error e = (s->p_emitter->host_regmap[map] == 
            JIT_REG_INVALID) ?
        JIT_SUCCESS : JIT_ERROR_REG_BUSY;
    if(e == JIT_SUCCESS) {
        s->p_emitter->host_regmap[g_regmap[map]] = reg;
        s->p_emitter->host_busy |= (1 << g_regmap[map]);
    }
    return e;
}

jit_error
jit_clear_reg_mapping(struct jit_state *s, jit_reg reg)
{
    jit_error e = (reg != JIT_REG_INVALID) ?
        JIT_SUCCESS : JIT_ERROR_VREG_INVALID;
    if(e == JIT_SUCCESS) {
        s->p_emitter->host_busy &= ~(1 << reg);
    }
    return e;
}

jit_host_reg
jit_get_mapped_host_reg(struct jit_state *s, jit_reg reg, jit_reg_access a)
{
    int n;
    jit_host_reg hostreg = JIT_HOST_REG_INVALID;
    jit_reg *regmap = s->p_emitter->host_regmap;
    jit_reg_age *agemap = s->p_emitter->host_agemap;
    jit_reg evicted = JIT_REG_INVALID;
    int64_t *spill = s->p_emitter->spill_vreg;
    
    if(reg == JIT_REG_INVALID) {
        goto l_exit;
    }

    // First, check if virtual register is already mapped to a host reg.
    for(n = 0; n < NUM_HOST_REGS; n++) {
        if(regmap[n] == reg) {
            hostreg = n;
            goto l_exit;
        }
    }

    // Virtual register not yet mapped; try to find a spare host register
    if(hostreg == JIT_HOST_REG_INVALID) {
        for(n = 0; n < NUM_HOST_REGS; n++) {
            if(regmap[n] == JIT_REG_INVALID) {
                regmap[n] = reg;
                hostreg = n;
                printf(GRAY("  vreg %d not mapped, using %s (host reg %d)\n"),
                        reg, g_hostregsz[n], n);
                goto l_spillcheck;
            }
        }
    }

    // All the slots are taken: evict the oldest one, if they are not all
    // mapped to specific slots, and spill it so its value is saved
    if(hostreg == JIT_HOST_REG_INVALID) {
        int oldest = -1;
        int maxage = -1;
        for(n = 0; n < NUM_HOST_REGS; n++) {
            if(agemap[n] > maxage &&
                    !(s->p_emitter->host_busy & (1 << n))) {
                oldest = n;
                maxage = agemap[n];
            }
        }
        if(oldest != -1) {
            evicted = regmap[oldest];
            s->p_bufcur = jit_emit__mov_reg32_to_m(s->p_bufcur, oldest,
                    (int32_t *)&spill[evicted]);
            s->p_emitter->spill_busy |= (1 << evicted);
            regmap[oldest] = reg;
            hostreg = oldest;
            printf(GRAY("  vreg %d in %s (host reg %d): need evict/spill vreg %d\n"),
                    reg, g_hostregsz[hostreg], hostreg, evicted);
        }
    }
l_spillcheck:
    if(a == JIT_ACCESS_W) {
        goto l_exit;
    }
    if(reg < NUM_SPILL_SLOTS && s->p_emitter->spill_busy & (1 << reg)) {
        printf(GRAY("  vreg %d was spilled, restoring\n"), reg);
        s->p_bufcur = jit_emit__mov_m32_to_reg(s->p_bufcur,
                (int32_t *)&spill[reg], hostreg);
        s->p_emitter->spill_busy &= ~(1 << reg);
        s->p_emitter->host_agemap[hostreg] = 0;
    } else if(reg >= NUM_SPILL_SLOTS) {
        printf("error: could not check spill, too many vregs\n");
    }

l_exit:
    return hostreg;
}

jit_error
jit_inc_reg_ages(struct jit_state *s, struct jit_instr *i)
{
    jit_error e = JIT_SUCCESS;
    size_t n;

    for(n = 0; n < NUM_HOST_REGS; n++) {
        jit_reg vreg = s->p_emitter->host_regmap[n];
        if((i->in1_type == JIT_OPERAND_REG && i->in1.reg == vreg) ||
                (i->in2_type == JIT_OPERAND_REG && i->in2.reg == vreg) ||
                (i->out_type == JIT_OPERAND_REG && i->out.reg == vreg)) {
            s->p_emitter->host_agemap[n] = 0;
        } else {
            s->p_emitter->host_agemap[n] += 1;
        }
    }

    return e;
}

struct jit_host_ptr
jit_get_host_regptr(struct jit_state *s, struct jit_ptr *p)
{
    struct jit_host_ptr hp;
    hp.base = s->p_emitter->host_regmap[p->base];
    hp.index = (p->base != JIT_REG_INVALID) ?
        s->p_emitter->host_regmap[p->base] : JIT_HOST_REG_INVALID;
    hp.scale = (p->scale >= 0 && p->scale <= 8) ? p_scalemap[p->scale] : -1;
    hp.offset = p->offset;

    return hp;
}

jit_error
jit_emit_instr(struct jit_state *s, struct jit_instr *i)
{
    jit_error e = JIT_SUCCESS;
    switch(i->op) {
        case JIT_OP_MOVE:
            e = jit_emit_move(s, i);
            break;
        case JIT_OP_ADD:
        case JIT_OP_SUB:
        case JIT_OP_AND:
        case JIT_OP_OR:
        case JIT_OP_XOR:
        case JIT_OP_SHR:
            e = jit_emit_arith(s, i);
            break;
        case JIT_OP_CALL:
            e = jit_emit_call(s, i);
            break;
        case JIT_OP_RET:
            e = jit_emit_ret(s, i);
            break;
        case JIT_OP_JUMP:
            e = jit_emit_jump(s, i);
            break;
        case JIT_OP_JUMP_IF:
            e = jit_emit_jump_if(s, i);
            break;
        case JIT_OP_PUSH:
            e = jit_emit_push(s, i);
            break;
        case JIT_OP_POP:
            e = jit_emit_pop(s, i);
            break;
        default:
            printf("error: emitter cannot handle op type %d\n", i->op);
            break;
    }
    jit_inc_reg_ages(s, i);
    return e;
}

jit_error
jit_emit_move(struct jit_state *s, struct jit_instr *i)
{
    size_t n;
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    jit_host_reg hostreg_in = JIT_HOST_REG_INVALID;
    jit_host_reg hostreg_out = JIT_HOST_REG_INVALID;

    if(i->in1_type == JIT_OPERAND_IMM) {
        if(i->out_type == JIT_OPERAND_REG) {
            hostreg_out = jit_get_mapped_host_reg(s, i->out.reg, JIT_ACCESS_W);
            if(hostreg_out == JIT_HOST_REG_INVALID) {
                fprintf(stderr, "error: jit reg. %d does not map to host reg.\n",
                        i->out.reg);
            }
            switch(i->opsz) {
                case JIT_32BIT:
                    s->p_bufcur = jit_emit__mov_imm32_to_reg(s->p_bufcur,
                            i->in1.imm32, hostreg_out);
                    break;
                case JIT_16BIT:
                    s->p_bufcur = jit_emit__mov_imm16_to_reg(s->p_bufcur,
                            i->in1.imm16, hostreg_out);
                    break;
                case JIT_8BIT:
                    s->p_bufcur = jit_emit__mov_imm8_to_reg(s->p_bufcur,
                            i->in1.imm8, hostreg_out);
                    break;
            }
        }
    } else if(i->in1_type == JIT_OPERAND_REG) {
        hostreg_in = jit_get_mapped_host_reg(s, i->in1.reg, JIT_ACCESS_R);
        if(i->out_type == JIT_OPERAND_REG) {
            // Move to self; we can short-circuit this
            if(i->out.reg == i->in1.reg) {
                goto l_exit;
            }
            hostreg_out = jit_get_mapped_host_reg(s, i->out.reg, JIT_ACCESS_W);
            s->p_bufcur = jit_emit__mov_reg32_to_reg(s->p_bufcur,
                    hostreg_in, hostreg_out);
        } else if(i->out_type == JIT_OPERAND_IMMPTR) {
            s->p_bufcur = jit_emit__mov_reg32_to_m(s->p_bufcur,
                    hostreg_in, i->out.m32ptr);
        }
    } else if(i->in1_type == JIT_OPERAND_IMMPTR) {
        if(i->out_type == JIT_OPERAND_REG) {
            hostreg_out = jit_get_mapped_host_reg(s, i->out.reg, JIT_ACCESS_W);
            switch(i->opsz) {
                case JIT_32BIT:
                    s->p_bufcur = jit_emit__mov_m32_to_reg(s->p_bufcur,
                            i->in1.m32ptr, hostreg_out);
                    break;
                case JIT_16BIT:
                    s->p_bufcur = jit_emit__mov_m16_to_reg(s->p_bufcur,
                            i->in1.m32ptr, hostreg_out);
                    break;
                case JIT_8BIT:
                    s->p_bufcur = jit_emit__mov_m8_to_reg(s->p_bufcur,
                            i->in1.m8ptr, hostreg_out);
                    break;
            }
        }
    } else if(i->in1_type == JIT_OPERAND_REGPTR) {
        if(i->out_type == JIT_OPERAND_REG) {
            struct jit_host_ptr hp = jit_get_host_regptr(s, 
                    &i->in1.regptr);
            hostreg_out = jit_get_mapped_host_reg(s, i->out.reg, JIT_ACCESS_W);
            s->p_bufcur = jit_emit__mov_regptr32_to_reg(s->p_bufcur,
                    &hp, hostreg_out);
        }
    } else if(i->in1_type == JIT_OPERAND_IMMDISP) {
        if(i->out_type == JIT_OPERAND_REG) {
            hostreg_out = jit_get_mapped_host_reg(s, i->out.reg, JIT_ACCESS_W);
            s->p_bufcur = jit_emit__lea_immdisp32_to_reg(s->p_bufcur,
                    i->in1.ptr, hostreg_out);
        }
    }

    printf("> mov:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

    s->blk_nb += (s->p_bufcur - begin);

l_exit:
    return e;
}

jit_error
jit_emit_arith(struct jit_state *s, struct jit_instr *i)
{
    size_t n;
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    jit_host_reg hostreg_in1 = JIT_HOST_REG_INVALID;
    jit_host_reg hostreg_in2 = JIT_HOST_REG_INVALID;
    jit_host_reg hostreg_out = JIT_HOST_REG_INVALID;

    if(i->out_type == JIT_OPERAND_REG) {
        hostreg_out = jit_get_mapped_host_reg(s, i->out.reg, JIT_ACCESS_W);
        if(i->in2_type == JIT_OPERAND_REG) {
            hostreg_in2 = jit_get_mapped_host_reg(s, i->in2.reg, JIT_ACCESS_R);
            if(i->in1_type == JIT_OPERAND_REG) {
                jit_host_reg hr_in = JIT_HOST_REG_INVALID;
                hostreg_in1 = jit_get_mapped_host_reg(s, i->in1.reg,
                        JIT_ACCESS_R);
                
                if(hostreg_in1 != hostreg_out && hostreg_in2 != hostreg_out) {
                    s->p_bufcur = jit_emit__mov_reg32_to_reg(s->p_bufcur,
                            hostreg_in1, hostreg_out);
                    hr_in = hostreg_in2;
                } else if(hostreg_in1 == hostreg_out) {
                    hr_in = hostreg_in2;
                } else if(hostreg_in2 == hostreg_out) {
                    hr_in = hostreg_in1;
                }
                s->p_bufcur = g_e_r_to_r[i->op](s->p_bufcur,
                        hr_in, hostreg_out);
            } else if(i->in1_type == JIT_OPERAND_IMM) {
                switch(i->opsz) {
                    case JIT_32BIT:
                        s->p_bufcur = g_e_imm32_to_r[i->op](s->p_bufcur,
                                i->in1.imm32, hostreg_out);
                        break;
                    case JIT_16BIT:
                        s->p_bufcur = g_e_imm16_to_r[i->op](s->p_bufcur,
                                i->in1.imm16, hostreg_out);
                        break;
                    case JIT_8BIT:
                        s->p_bufcur = g_e_imm8_to_r[i->op](s->p_bufcur,
                                i->in1.imm8, hostreg_out);
                        break;
                }
            }
        }
    }

    printf("> %s:\t", g_opsz[i->op]);
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

    s->blk_nb += (s->p_bufcur - begin);

l_exit:
    return e;
}

jit_error
jit_emit_call(struct jit_state *s, struct jit_instr *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;
    
    if(i->in1_type == JIT_OPERAND_IMMPTR) {
        s->p_bufcur = jit_emit__call_m32(s->p_bufcur, i->in1.m32ptr);
    }

    printf("> call:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

    s->blk_nb += (s->p_bufcur - begin);

l_exit:
    return e;
}

jit_error
jit_emit_jump(struct jit_state *s, struct jit_instr *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_jump_if(struct jit_state *s, struct jit_instr *i)
{
    return JIT_SUCCESS;
}

jit_error
jit_emit_ret(struct jit_state *s, struct jit_instr *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;

    s->p_bufcur = jit_emit__ret(s->p_bufcur);

    printf("> ret:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

    s->blk_nb += (s->p_bufcur - begin);

l_exit:
    return e;
}

jit_error
jit_emit_push(struct jit_state *s, struct jit_instr *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;
    jit_host_reg hostreg = jit_get_mapped_host_reg(s, i->in1.reg,
            JIT_ACCESS_R);

    s->p_bufcur = jit_emit__push_reg(s->p_bufcur, hostreg);

    printf("> push:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

    s->blk_nb += (s->p_bufcur - begin);

    return e;
}

jit_error
jit_emit_pop(struct jit_state *s, struct jit_instr *i)
{
    jit_error e = JIT_SUCCESS;
    uint8_t *begin = s->p_bufcur;
    size_t n;
    jit_host_reg hostreg = jit_get_mapped_host_reg(s, i->in1.reg,
            JIT_ACCESS_W);

    s->p_bufcur = jit_emit__pop_reg(s->p_bufcur, hostreg);

    printf("> pop:\t");
    for(n = 0; n < (s->p_bufcur - begin); n++)
        printf("%02x ", begin[n]);
    printf("\n");

    s->blk_nb += (s->p_bufcur - begin);

    return e;
}

