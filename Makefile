# Copyright (C) 2015 The PASTA Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

CODE_FILE := code.bin
SYS_PATH := rxTools/sys
PATCH_PATH := $(SYS_PATH)/patches
SET_CODE_PATH := CODE_PATH=$(SYS_PATH)/$(CODE_FILE)
SET_DATNAME := DATNAME=$(SYS_PATH)/$(CODE_FILE)

export INCDIR := -I$(CURDIR)/include
export RXTOOLSMK := $(CURDIR)/common.mk

export GIT_VERSION := $(shell git rev-parse --short HEAD)

ifeq ($(GIT_VERSION),)
    GIT_VERSION := "N/A"
endif

CFLAGS = -std=c11 -O2 -Wall -Wextra
ROPFLAGS = $(SET_DATNAME) DISPNAME=rxTools GRAPHICS=../logo
BRAHFLAGS = name=$(CODE_FILE) filepath=$(SYS_PATH)/ \
			APP_TITLE='rxTools' \
			APP_DESCRIPTION='Roxas75 3DS Toolkit & Custom Firmware' \
			APP_AUTHOR='Patois, et al.' \
			ICON=$(abspath icon.png)
sp :=
sp +=
THEMES := $(subst .json+,.json ,$(subst $(sp),+,$(wildcard theme/*.json)))

.Phony: all
all: release-licenses release-rxtools release-doc release-lang	\
	release-theme release-tools release-mset release-brahma

distclean:
	@rm -rf release

clean: distclean
	@$(MAKE) -C rxtools clean
	@$(MAKE) $(BRAHFLAGS) -C CakeBrah clean
	@$(MAKE) -C theme clean
	@$(MAKE) $(ROPFLAGS) -C CakesROP clean
	@$(MAKE) $(SET_DATNAME) -C CakesROP/CakesROPSpider clean

release-licenses:
	@mkdir -p release
	@cp LICENSE release
	@cp rxtools/source/lib/jsmn/LICENSE release/LICENSE_JSMN
	@cp rxtools/CakeHax/LICENSE.txt release/LICENSE_CakeHax.txt
	@cp CakesROP/LICENSE release/LICENSE_CakesROP

release-rxtools:
	@$(MAKE) SYS_PATH=$(SYS_PATH) -C rxtools
	@mkdir -p release/$(SYS_PATH)
	@cp rxtools/build/code.bin release/$(SYS_PATH)
	@cp rxtools/gui.json release/$(SYS_PATH)
	@cp rxtools/build/arm9loaderhax.bin release

release-doc:
	@cp README.md "docs/QuickStartGuide(v3.0_BETA).pdf" release

release-lang:
	mkdir -p release/rxTools/lang
	@cp lang/* release/rxTools/lang

release-theme: themes $(THEMES)
themes:
	@$(MAKE) -C theme

%.json:
	mkdir -p $(subst +,\ ,$(addprefix release/rxTools/,$(basename $@)))
	mv $(subst +,\ ,$(addsuffix /*.bgr,$(basename $@))) $(subst +,\ ,$(addprefix release/rxTools/,$(basename $@)))
	cp $(subst +,\ ,$@) release/rxTools/theme

release-tools:
	@mkdir -p release/Tools/scripts
	@cp tools/o3ds_cdn_firm.py tools/n3ds_cdn_firm.py tools/readme.txt release/Tools
	@cp tools/scripts/* release/Tools/scripts/

release-mset:
	@$(MAKE) $(ROPFLAGS) -C CakesROP
	@$(MAKE) $(SET_DATNAME) -C CakesROP/CakesROPSpider
	@mkdir -p release/mset
	@cp CakesROP/CakesROP.nds release/mset/rxinstaller.nds
	@cp CakesROP/CakesROPSpider/code.bin release/mset/rxinstaller.bin

release-brahma:
	$(MAKE) $(BRAHFLAGS) -C CakeBrah
	@mkdir -p release/ninjhax/rxTools
	@cp CakeBrah/code.bin.3dsx release/ninjhax/rxTools/rxTools.3dsx
	@cp CakeBrah/code.bin.smdh release/ninjhax/rxTools/rxTools.smdh
