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
   andi    a0, a0, 64
   beqz    a0, done
   la      a0, flags
   lw      t0, (a0)
   ori     t0, t0, 0x80
   sw      t0, (a0)
done:
   ret
