VPATH = ../../src ../../include ../../../BBCSDL/src ../../../BBCSDL/include

BASE=riscv32-unknown-elf-
ARCH=rv32im_zicsr
ABI=ilp32

CXX = ${BASE}gcc -Wall -I ../../include -march=${ARCH} -mabi=${ABI}

AS = ${BASE}as -I ../../include -march=${ARCH} -mabi=${ABI}

OBJ = riscv_wrapper.o bbmain.o bbexec.o bbeval.o bbdata.o bbasmb.o

#bbccon.o
#bbccos.o

all: bbcbasic

riscv_wrapper.o: riscv_wrapper.c BBC.h
	$(CXX) -c -O2 $< -o $@

bbmain.o: bbmain.c BBC.h
	$(CXX) -c -O2 $< -o $@

bbexec.o: bbexec.c BBC.h
	$(CXX) -c -O2 $< -o $@

bbeval.o: bbeval.c BBC.h
	$(CXX) -Wno-array-bounds -c -O2 $< -o $@

# TODO
bbasmb.o: bbasmb_arm_32.c BBC.h
	$(CXX) -c -Os $< -o $@

#bbccos.o: bbccos.c bbccon.h
#	$(CXX) -Wno-array-bounds -Wno-unused-result -c -Os $< -o $@

#bccon.o: bbccon.c bbccon.h
#	$(CXX) -Wno-array-bounds -Wno-unused-result -c -Os $< -o $@

#bbdata.o: ../../../BBCSDL/src/bbdata_x86_64.nas
#	nasm -f elf64 -s ../../../BBCSDL/src/bbdata_x86_64.nas -o bbdata.o

bbdata.o: ../../../BBCSDL/src/bbdata_arm_32.s
	$(AS) ../../../BBCSDL/src/bbdata_arm_32.s -o bbdata.o

bbcbasic: $(OBJ)
	$(CXX) $(OBJ) -lm  -Tbbcbasic.ld -nostartfiles -o bbcbasic
	cp bbcbasic ../../
#-Wl,-s -Wl,-R,'$$ORIGIN'

clean:
	rm -f $(OBJ)
