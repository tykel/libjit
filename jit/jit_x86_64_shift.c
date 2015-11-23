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

uint8_t*
jit_emit__shr_imm32_to_reg(uint8_t *p, int32_t imm, jit_host_reg regout)
{
    if(NEED_REX(regout)) *p++ = REX_R;
    *p++ = 0xc1;
    *p++ = MODRM(MOD_REGDIRECT, OX_SHR, HOSTREG(regout));
    *(int8_t *)p++ = (int8_t)imm;
    
    return p;
}

uint8_t*
jit_emit__shl_imm32_to_reg(uint8_t *p, int32_t imm, jit_host_reg regout)
{
    if(NEED_REX(regout)) *p++ = REX_R;
    *p++ = 0xc1;
    *p++ = MODRM(MOD_REGDIRECT, OX_SHL, HOSTREG(regout));
    *(int8_t *)p++ = (int8_t)imm;
    
    return p;
}

uint8_t*
jit_emit__sar_imm32_to_reg(uint8_t *p, int32_t imm, jit_host_reg regout)
{
    if(NEED_REX(regout)) *p++ = REX_R;
    *p++ = 0xc1;
    *p++ = MODRM(MOD_REGDIRECT, OX_SAR, HOSTREG(regout));
    *(int8_t *)p++ = (int8_t)imm;
    
    return p;
}
