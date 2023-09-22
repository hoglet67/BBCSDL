   .global _start
   .global error_handler
   .global escape_handler

_start:
   j       _main

# Called with a0 pointing to the error code
error_handler:
   addi    a1, a0, 1          # a1 = error smessage
   lbu     a0, (a0)           # a0 = error code
# pass on to BBC Basic's error handler:
#     void error(int, const char *);
# this never returns
   j       error

# Called with a0 bit 6 being the escape flag
escape_handler:
   andi    a0, a0, 64         # mask escape flag
   slli    a0, a0, 1          # shift escape flag into bit 7
   la      t1, flags          # get the address of the flags variable
   lw      t0, (t1)           # read the flags variable from memory
   andi    t0, t0, 0xffffff7f # clear bit 7
   or      t0, t0, a0         # copy escape flag to bit 7
   sw      t0, (t1)           # write the flags variable back to memory
   ret
