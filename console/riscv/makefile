VPATH = ../../src ../../include ../../../BBCSDL/src ../../../BBCSDL/include

BASE=riscv32-unknown-elf-
ARCH=rv32im_zicsr
ABI=ilp32

CXX = ${BASE}gcc

AS = ${BASE}as -I ../../include -march=${ARCH} -mabi=${ABI}

OBJ = crt0.o riscv_wrapper.o bbmain.o bbexec.o bbeval.o bbdata.o bbasmb.o

CFLAGS = -Wall -I ../../include -march=${ARCH} -mabi=${ABI} -c -Os

all: bbcbasic

crt0.o: crt0.s
	$(AS) $< -o $@

riscv_wrapper.o: riscv_wrapper.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbmain.o: bbmain.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbexec.o: bbexec.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbeval.o: bbeval.c BBC.h
	$(CXX) -Wno-array-bounds $(CFLAGS) $< -o $@

# TODO
bbasmb.o: bbasmb_arm_32.c BBC.h
	$(CXX) $(CFLAGS) $< -o $@

bbdata.o: ../../../BBCSDL/src/bbdata_arm_32.s
	$(AS) ../../../BBCSDL/src/bbdata_arm_32.s -o bbdata.o

bbcbasic: $(OBJ)
	$(CXX) $(OBJ) -lm  -Tbbcbasic.ld -nostartfiles -o bbcbasic
	cp bbcbasic ../../
#-Wl,-s -Wl,-R,'$$ORIGIN'

clean:
	rm -f $(OBJ)
