VPATH = ../../src ../../include ../../../BBCSDL/src ../../../BBCSDL/include

BASE=riscv32-unknown-elf-
ARCH=rv32im_zicsr
ABI=ilp32

CXX = ${BASE}gcc
AS  = ${BASE}as

OBJ = crt0.o riscv_wrapper.o bbmain.o bbexec.o bbeval.o bbdata.o bbasmb.o

ASFLAGS = -I ../../include -march=${ARCH} -mabi=${ABI}
CFLAGS  = --specs=nano.specs -u _printf_float -Wall -I ../../include -march=${ARCH} -mabi=${ABI} -c -Os
LDFLAGS = --specs=nano.specs -u _printf_float -lm -nostartfiles -Tbbcbasic.ld  -Os

all: bbcbasic

crt0.o: crt0.s
	$(AS) $(ASFLAGS) $< -o $@

riscv_wrapper.o: riscv_wrapper.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbmain.o: bbmain.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbexec.o: bbexec.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbeval.o: bbeval.c BBC.h
	$(CXX) -Wno-array-bounds $(CFLAGS) $< -o $@

# TODO
bbasmb.o: bbasmb_riscv.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbdata.o: ../../../BBCSDL/src/bbdata_riscv.s
	$(AS) $(ASFLAGS) ../../../BBCSDL/src/bbdata_riscv.s -o bbdata.o

bbcbasic: $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o bbcbasic
	cp bbcbasic ../../

clean:
	rm -f $(OBJ)
