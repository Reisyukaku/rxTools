/*
 * Copyright (C) 2015 The PASTA Team
 * Original version by megazig
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "crypto.h"

void setup_aeskeyX(uint_fast8_t keyslot, void* keyx)
{
    uint32_t * _keyx = (uint32_t*)keyx;
    *REG_AESKEYCNT = (*REG_AESKEYCNT >> 6 << 6) | keyslot| 0x80;
    *REG_AESKEYXFIFO = _keyx[0];
    *REG_AESKEYXFIFO = _keyx[1];
    *REG_AESKEYXFIFO = _keyx[2];
    *REG_AESKEYXFIFO = _keyx[3];
}

void setup_aeskeyN(uint_fast8_t keyslot, void* keyx)
{
    volatile uint32_t* aes_regs[] =
    {
        (volatile uint32_t*)0x10009040,
        (volatile uint32_t*)0x10009070,
        (volatile uint32_t*)0x100090A0,
        (volatile uint32_t*)0x100090D0
    };
    uint32_t * _keyx = (uint32_t*)keyx;
    *REG_AESKEYCNT = (*REG_AESKEYCNT >> 6 << 6) | keyslot| 0x80;
    if (keyslot > 3)
    {
    *REG_AESKEYNFIFO = _keyx[0];
    *REG_AESKEYNFIFO = _keyx[1];
    *REG_AESKEYNFIFO = _keyx[2];
    *REG_AESKEYNFIFO = _keyx[3];
	}
else{
        volatile uint32_t* aes_reg = aes_regs[keyslot];
        if (AES_CNT_INPUT_ORDER)
        {
            aes_reg[0] = _keyx[3];
            aes_reg[1] = _keyx[2];
            aes_reg[2] = _keyx[1];
            aes_reg[3] = _keyx[0];
        }
        else
        {
            aes_reg[0] = _keyx[0];
            aes_reg[1] = _keyx[1];
            aes_reg[2] = _keyx[2];
            aes_reg[3] = _keyx[3];
        }
}
}

void decrypt(void* key, void* inbuf, void* outbuf, size_t size)
{
    setup_aeskey(0x2C, AES_BIG_INPUT|AES_NORMAL_INPUT, key);
    use_aeskey(0x2C);
    aes_decrypt(inbuf, outbuf, size / AES_BLOCK_SIZE, AES_CTR_MODE);
}

void setup_aeskey(uint_fast8_t keyno, int value, void* key)
{
	if (!key) return;
    volatile uint32_t* aes_regs[] =
    {
        (volatile uint32_t*)0x19009060,
        (volatile uint32_t*)0x10009090,
        (volatile uint32_t*)0x100090C0,
        (volatile uint32_t*)0x100090F0
    };
    uint32_t * _key = (uint32_t*)key;
    *REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN|AES_CNT_INPUT_ORDER)) | (value << 23);
    if (keyno > 3)
    {
        if (keyno > 0x3F)
            return;
        *REG_AESKEYCNT = (*REG_AESKEYCNT >> 6 << 6) | (uint8_t)keyno | 0x80;
        *REG_AESKEYFIFO = _key[0];
        *REG_AESKEYFIFO = _key[1];
        *REG_AESKEYFIFO = _key[2];
        *REG_AESKEYFIFO = _key[3];
    }
    else
    {
        volatile uint32_t* aes_reg = aes_regs[keyno];
        if (value & 0x4)
        {
            aes_reg[0] = _key[3];
            aes_reg[1] = _key[2];
            aes_reg[2] = _key[1];
            aes_reg[3] = _key[0];
        }
        else
        {
            aes_reg[0] = _key[0];
            aes_reg[1] = _key[1];
            aes_reg[2] = _key[2];
            aes_reg[3] = _key[3];
        }
    }

}

void use_aeskey(uint_fast8_t keyno)
{
    if (keyno > 0x3F)
        return;
    *REG_AESKEYSEL = keyno;
    *REG_AESCNT    = *REG_AESCNT | 0x04000000; /* mystery bit */
}

void set_ctr(int mode, aes_ctr_old *iv) {
    *REG_AESCNT = (*REG_AESCNT & ~(AES_CNT_INPUT_ENDIAN|AES_CNT_INPUT_ORDER)) | (mode << 23);
    for (size_t i = 0; i < 4; i++)
        *(REG_AESCTR + i) = iv->data32[(mode & AES_NORMAL_INPUT) ? 3-i : i];
}

void add_ctr(aes_ctr_old *ctr, uint32_t carry) {
    uint32_t counter;
    for (size_t i = 4; i--;) {
        counter = __builtin_bswap32(ctr->data32[i]);
        carry += counter;
        ctr->data32[i] = __builtin_bswap32(carry);
        carry = carry < counter ? 1 : 0;
    }
}

void aes_decrypt(void* inbuf, void* outbuf, size_t size, uint32_t mode) //Initialization vector not used?
{
    uint32_t in  = (uint32_t)inbuf;
    uint32_t out = (uint32_t)outbuf;
    size_t block_count = size;
    size_t blocks;
    while (block_count != 0)
    {
        blocks = (block_count >= 0xFFFF) ? 0xFFFF : block_count;
        _decrypt(mode, (void*)in, (void*)out, blocks);
        in  += blocks * AES_BLOCK_SIZE;
        out += blocks * AES_BLOCK_SIZE;
        block_count -= blocks;
    }
}

void _decrypt(uint32_t value, void* inbuf, void* outbuf, size_t blocks)
{
    *REG_AESCNT = 0;
    *REG_AESBLKCNT = blocks << 16;
    *REG_AESCNT = value |
                  AES_CNT_START |
                  AES_CNT_INPUT_ORDER |
                  AES_CNT_OUTPUT_ORDER |
                  AES_CNT_INPUT_ENDIAN |
                  AES_CNT_OUTPUT_ENDIAN |
                  AES_CNT_FLUSH_READ |
                  AES_CNT_FLUSH_WRITE;
    aes_fifos(inbuf, outbuf, blocks);
}

void aes_fifos(void* inbuf, void* outbuf, size_t blocks)
{
    uint32_t in  = (uint32_t)inbuf;
    uint32_t out = (uint32_t)outbuf;
    size_t curblock = 0;
    while (curblock != blocks)
    {
        if (in)
        {
            while (aescnt_checkwrite()) ;
            int ii = 0;
            for (ii = in; ii != in + AES_BLOCK_SIZE; ii += 4)
            {
                set_aeswrfifo( *(uint32_t*)(ii) );
            }
            if (out)
            {
                while (aescnt_checkread()) ;
                for (ii = out; ii != out + AES_BLOCK_SIZE; ii += 4)
                {
                    *(uint32_t*)ii = read_aesrdfifo();
                }
            }
        }
        curblock++;
    }
}

void set_aeswrfifo(uint32_t value)
{
    *REG_AESWRFIFO = value;
}

uint32_t read_aesrdfifo(void)
{
    return *REG_AESRDFIFO;
}

uint32_t aes_getwritecount()
{
    return *REG_AESCNT & 0x1F;
}

uint32_t aes_getreadcount()
{
    return (*REG_AESCNT >> 5) & 0x1F;
}

uint32_t aescnt_checkwrite()
{
    size_t ret = aes_getwritecount();
    return (ret > 0xF);
}

uint32_t aescnt_checkread()
{
    size_t ret = aes_getreadcount();
    return (ret <= 3);
}
