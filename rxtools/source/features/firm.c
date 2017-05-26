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

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "firm.h"
#include "mpcore.h"
#include "hid.h"
#include "lang.h"
#include "console.h"
#include "fs.h"
#include "nand.h"
#include "ncch.h"
#include "draw.h"
#include "menu.h"
#include "fileexplorer.h"
#include "TitleKeyDecrypt.h"
#include "configuration.h"
#include "lang.h"
#include "aes.h"
#include "progress.h"

const wchar_t *firmPathFmt= L"" FIRM_PATH_FMT;
const wchar_t *firmPatchPathFmt = L"" FIRM_PATCH_PATH_FMT;

unsigned int emuNandMounted = 0;

static int loadFirm(wchar_t *path, UINT *fsz)
{
	FIL fd;
	FRESULT r;

	r = f_open(&fd, path, FA_READ);
	if (r != FR_OK)
		return r;

	r = f_read(&fd, (void *)FIRM_ADDR, f_size(&fd), fsz);
	if (r != FR_OK)
		return r;

	f_close(&fd);

	return ((FirmHdr *)FIRM_ADDR)->magic == FIRM_MAGIC ? 0 : -1;
}

static int decryptFirmKtrArm9(void *p) {
//	uint8_t key[AES_BLOCK_SIZE];
	aes_key *key;
//	PartitionInfo info;
	Arm9Hdr *hdr;
	FirmSeg *seg, *btm;

	seg = ((FirmHdr *)p)->segs;
	for (btm = seg + FIRM_SEG_NUM; seg->isArm11; seg++)
		if (seg == btm)
			return 0;

	hdr = (void *)(p + seg->offset);

//	info.ctr = &hdr->ctr;
//	info.buffer = (uint8_t *)hdr + 0x800;
//	info.keyY = hdr->keyY;
//	info.size = atoi(hdr->size);

//	use_aeskey(0x11);
	aes_set_key(&(aes_key){NULL, 0, 0x11, 0});
	if (hdr->ext.pad[0] == 0xFFFFFFFF) {
//		info.keyslot = 0x15;
//		aes_decrypt(hdr->keyX, key, 1, AES_ECB_DECRYPT_MODE);
//		setup_aeskeyX(info.keyslot, key);
		key = &(aes_key){&(aes_key_data){{0}}, AES_CNT_INPUT_BE_NORMAL, 0x15, KEYX};
		aes((void*)key->data, (void*)&hdr->keyX, sizeof(aes_key_data), NULL, AES_ECB_DECRYPT_MODE);
		aes_set_key(key);
		key->data = &hdr->keyY;
		key->type = KEYY;
	} else {
//		info.keyslot = 0x16;
//		aes_decrypt(hdr->ext.s.keyX_0x16, key, 1, AES_ECB_DECRYPT_MODE);
		key = &(aes_key){&hdr->keyY, AES_CNT_INPUT_BE_NORMAL, 0x16, KEYY};
//		aes(key->data, hdr->ext.s.keyX_0x16, sizeof(aes_key_data), NULL, AES_ECB_DECRYPT_MODE);
	}

	aes_set_key(key);
//	return DecryptPartition(&info);
	aes_ctr ctr = {hdr->ctr, AES_CNT_INPUT_BE_NORMAL};
	aes((uint8_t *)hdr + 0x800, (uint8_t *)hdr + 0x800, strtoul(hdr->size, NULL, 10), &ctr, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);

	return 1;
}

uint8_t *decryptFirmTitleNcch(uint8_t* title, size_t *size) {
	ctr_ncchheader *NCCH = ((ctr_ncchheader*)title);
	if (NCCH->magic != NCCH_MAGIC) return NULL;
	aes_ctr ctr;
	ncch_get_counter(NCCH, &ctr, NCCHTYPE_EXEFS);

	aes_set_key(&(aes_key){(aes_key_data*)NCCH->signature, AES_CNT_INPUT_BE_NORMAL, 0x2C, KEYY});
	aes(title + NCCH->exefsoffset * NCCH_MEDIA_UNIT_SIZE, title + NCCH->exefsoffset * NCCH_MEDIA_UNIT_SIZE, NCCH->exefssize * NCCH_MEDIA_UNIT_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);

	if (size != NULL)
		*size = NCCH->exefssize * NCCH_MEDIA_UNIT_SIZE - sizeof(ctr_ncchheader);

	uint8_t *firm = (uint8_t*)(title + NCCH->exefsoffset * NCCH_MEDIA_UNIT_SIZE + sizeof(FirmHdr));

	if (getMpInfo() == MPINFO_KTR && !decryptFirmKtrArm9(firm))
		return NULL;
	return firm;
}

uint8_t *decryptFirmTitle(uint8_t *title, size_t size, size_t *firmSize, uint8_t key[16]){
	aes_ctr ctr = {{{0}}, AES_CNT_INPUT_BE_NORMAL};
	aes_set_key(&(aes_key){(aes_key_data*)key, AES_CNT_INPUT_BE_NORMAL, 0x2C, NORMALKEY});
	aes(title, title, size, &ctr, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);

	return decryptFirmTitleNcch(title, firmSize);
}

void rxMode(uint8_t disk){
    wchar_t path[64];
    uint32_t tid;
    UINT fsz;
    
	switch (getMpInfo()) {
		case MPINFO_KTR:
			tid = TID_KTR_NATIVE_FIRM;
			break;

		case MPINFO_CTR:
			tid = TID_CTR_NATIVE_FIRM;
			break;

		default:
            return;
	}

    getFirmPath(path, tid);
    loadFirm(path, &fsz);
    
	unsigned arm9Entry = 0x0801B01C;
	((void (*)())arm9Entry)();
}

void FirmLoader(wchar_t *firm_path){

	UINT fsz;
	if (loadFirm(firm_path, &fsz)){
		ConsoleInit();
		ConsoleSetTitle(strings[STR_LOAD], strings[STR_FIRMWARE_FILE]);
		print(strings[STR_WRONG], L"", strings[STR_FIRMWARE_FILE]);
		print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
		ConsoleShow();
		WaitForButton(keys[KEY_A].mask);
		return;
	}
}
