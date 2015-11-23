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
jit_emit__call_m32(uint8_t *p, void *m)
{
    size_t ibs = 1 + sizeof(int32_t);
    int32_t disp = (int32_t)((int64_t)m - (int64_t)(p + ibs));
    *p++ = 0xe8;
    *(int32_t *)p = disp;
    p += sizeof(int32_t);
    return p;
}

uint8_t*
jit_emit__ret(uint8_t *p)
{
    *p++ = 0xc3;
    return p;
}

uint8_t*
jit_emit__push_reg(uint8_t *p, jit_host_reg regout)
{
    if(NEED_REX(regout)) *p++ = REX_R;
    *p++ = 0x50 + HOSTREG(regout);
    return p;
}

uint8_t*
jit_emit__pop_reg(uint8_t *p, jit_host_reg regout)
{
    if(NEED_REX(regout)) *p++ = REX_R;
    *p++ = 0x58 + HOSTREG(regout);
    return p;
}
