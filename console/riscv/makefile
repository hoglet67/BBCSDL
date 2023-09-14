VPATH = ../../src ../../include ../../../BBCSDL/src ../../../BBCSDL/include

BASE=riscv32-unknown-elf-
ARCH=rv32im_zicsr
ABI=ilp32

CXX     = ${BASE}gcc
AS      = ${BASE}as
STRIP   = ${BASE}strip
OBJCOPY = ${BASE}objcopy

OBJ = crt0.o riscv_wrapper.o bbmain.o bbexec.o bbeval.o bbdata.o bbasmb.o

ASFLAGS = -I ../../include -march=${ARCH} -mabi=${ABI}
CFLAGS  = --specs=nano.specs -u _printf_float -Wall -I ../../include -march=${ARCH} -mabi=${ABI} -c -Os
LDFLAGS = --specs=nano.specs -u _printf_float -lm -nostartfiles -Tbbcbasic.ld  -Os

all: version rvbasic.ssd

version:
	./version.sh

crt0.o: crt0.s
	$(AS) $(ASFLAGS) $< -o $@

riscv_wrapper.o: riscv_wrapper.c build.h BBC.h
	$(CXX) $(CFLAGS) -I. $< -o $@

bbmain.o: bbmain.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbexec.o: bbexec.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbeval.o: bbeval.c BBC.h
	$(CXX) -Wno-array-bounds $(CFLAGS) $< -o $@

bbasmb.o: bbasmb_riscv.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbdata.o: ../../../BBCSDL/src/bbdata_riscv.s
	$(AS) $(ASFLAGS) ../../../BBCSDL/src/bbdata_riscv.s -o bbdata.o

bbcbasic: $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o bbcbasic

RVBASIC: bbcbasic
	$(OBJCOPY) -O binary $< $@

rvbasic.ssd: RVBASIC
	rm -f $@
	beeb blank_ssd $@
	beeb title $@ "RISC-V BASIC"
	beeb putfile $@ $<
	beeb putfile $@ examples/*
	beeb opt4 $@ 3
	beeb info $@

clean:
	rm -f $(OBJ) build.h bbcbasic RVBASIC rvbasic.ssd
