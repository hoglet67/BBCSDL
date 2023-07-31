   .global _start
   .global escape_handler

_start:
   j       _main

escape_handler:
   andi    a0, a0, 64
   beqz    a0, done
   la      a0, flags
   lw      t0, (a0)
   ori     t0, t0, 0x80
   sw      t0, (a0)
done:
   ret
