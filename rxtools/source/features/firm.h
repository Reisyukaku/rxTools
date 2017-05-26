/*
 * Copyright (C) 2015 The PASTA Team
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

#ifndef CFW_H
#define CFW_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fs.h"
#include "aes.h"

#define FIRM_MAGIC 'MRIF'
#define FIRM_SEG_NUM (4)
#define FIRM_PATCH_PATH_FMT PATCHES_PATH
#define FIRM_PATH_FMT "rxTools/data/%08X%08X.bin"
#define FIRM_ADDR (0x24000000)
#define FIRM_SIZE (33554432)

typedef struct {
	aes_key_data keyX;
	aes_key_data keyY;
	aes_ctr_data ctr;
	char size[8];
	uint8_t pad[8];
	uint8_t control[AES_BLOCK_SIZE];
	union {
		uint32_t pad[8];
		struct {
			uint8_t unk[16];
			aes_key_data keyX_0x16;
		} s;
	} ext;
} Arm9Hdr;

typedef enum {
	TID_HI_FIRM = 0x00040138
} TitleIdHi;

typedef enum {
        TID_CTR_NATIVE_FIRM = 0x00000002,
        TID_CTR_TWL_FIRM = 0x00000102,
        TID_CTR_AGB_FIRM = 0x00000202,
        TID_KTR_NATIVE_FIRM = 0x20000002
} TitleIdLo;

typedef struct {
	uint32_t offset;
	uint32_t addr;
	uint32_t size;
	uint32_t isArm11;
	uint8_t hash[32];
} FirmSeg;

typedef struct {
	uint32_t magic;
	uint32_t unused0;
	uint32_t arm11Entry;
	uint32_t arm9Entry;
	uint8_t unused1[48];
	FirmSeg segs[FIRM_SEG_NUM];
	uint8_t sig[256];
} FirmHdr;

extern const wchar_t *firmPathFmt;
extern const wchar_t *firmPatchPathFmt;

void FirmLoader();
void rxMode(uint8_t disk);
uint8_t* decryptFirmTitleNcch(uint8_t* title, size_t *size);
uint8_t *decryptFirmTitle(uint8_t *title, size_t size, size_t *firmSize, uint8_t key[16]);
FRESULT applyPatch(void *file, const char *patch);

static inline int getFirmPath(wchar_t *s, TitleIdLo id)
{
	return swprintf(s, _MAX_LFN, firmPathFmt, TID_HI_FIRM, id);
}

static inline int getFirmPatchPath(wchar_t *s, TitleIdLo id)
{
	return swprintf(s, _MAX_LFN, firmPatchPathFmt, TID_HI_FIRM, id);
}

#endif