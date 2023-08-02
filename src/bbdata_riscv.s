/*****************************************************************\
*       BBC BASIC for SDL 2.0 (ARM_32)                            *
*       Copyright (c) R. T. Russell, 2018-2021                    *
*                                                                 *
*       BBCDAT.S RAM data definitions                             *
*       Version 1.24a, 29-Jul-2021                                *
\*****************************************************************/

.global stavar
.global lc
.global oc
.global pc
.global dynvar
.global fnptr
.global proptr
.global accs
.global buff
.global vpage
.global tracen
.global lomem
.global pfree
.global himem
.global libase
.global errtxt
.global onersp
.global errtrp
.global datptr
.global vcount
.global curlin
.global alltrp
.global timtrp
.global clotrp
.global siztrp
.global systrp
.global moutrp
.global errlin
.global prand
.global vwidth
.global errnum
.global liston

.global lstopt
.global evtqw
.global evtqr
.global modeno
.global flist
.global tmps
.global sysvar
.global memhdc
.global flags
.global diradr
.global dirlen
.global libadr
.global liblen
.global cmdadr
.global cmdlen
.global usradr
.global usrlen
.global tmpadr
.global tmplen
.global link00
.global fvtab

/*.global fnarr*/
   
.data

/* Variables used by generic modules (bbmain, bbexec, bbeval, bbasmb) */

stavar: .fill   12,4,0  /* Static variables @% to K% */
lc:     .long   0       /* Static variable L% */
        .long   0,0     /* Static variables M%, N% */
oc:     .long   0       /* Static variable O% */
pc:     .long   0       /* Static variable P% */
        .fill   10,4,0  /* Static variables Q% to Z% */
dynvar: .fill   54,4,0  /* Pointers to dynamic vars */
fnptr:  .long   0       /* Pointer to user FuNctions */
proptr: .long   0       /* Pointer to user PROCedures */
accs:   .long   0       /* Pointer to string accumulator */
buff:   .long   0       /* Pointer to string input buffer */
vpage:  .long   0       /* Current value of PAGE */
tracen: .long   0       /* Maximum line number to trace */
lomem:  .long   0       /* Current value of LOMEM */
pfree:  .long   0       /* Pointer to start of free space */
himem:  .long   0       /* Current value of HIMEM */
libase: .long   0       /* Pointer to INSTALLed library */
errtxt: .long   0       /* Pointer to error text for REPORT */
onersp: .long   0       /* ESP storage for ON ERROR LOCAL */
errtrp: .long   0       /* Pointer to ON ERROR statement */
datptr: .long   0       /* Pointer to DATA statements */
vcount: .long   0       /* Current value of COUNT */
curlin: .long   0       /* Pointer to current statement */
alltrp:                 /* Start of trap pointers */
timtrp: .long   0       /* Pointer to ON TIME statement */
clotrp: .long   0       /* Pointer to ON CLOSE statement */
siztrp: .long   0       /* Pointer to ON MOVE statement */
systrp: .long   0       /* Pointer to ON SYS statement */
moutrp: .long   0       /* Pointer to ON MOUSE statement */
errlin: .long   0       /* Pointer to last error statement */

prand:  .byte   0,0,0,0,0       /* Current 'random' number (5 bytes) */
vwidth: .byte   0       /* Current value of WIDTH */
errnum: .byte   0       /* Error code of last error */
liston: .byte   0       /* *FLOAT / *HEX / *LOWERCASE / OPT */
   
lstopt: .byte   0       /* LISTO value (indentation) */
evtqw:  .byte   0       /* Event queue write pointer */
evtqr:  .byte   0       /* Event queue read pointer */
modeno: .byte   0       /* Mode number */
   
flist:  .fill   33,4,0  /* Pointers to string free lists */
tmps:   .long   0       /* Temp string descriptor: address */
        .long   0       /* Temp string descriptor: length  */
      
sysvar: .long   link12 - sysvar
        .asciz  "memhdc%"
memhdc: .long   0       /* Shadow screen device context */

        .byte   0       /* Padding */
link12: .long   link17 - link12
        .asciz  "flags%"
        .byte   0       /* DMB: now unused */
        .byte   0       /* DMB: now unused */
        .byte   0       /* DMB: now unused */
flags:  .byte   0       /* Boolean flags (byte) @ 3FBH */
   
        .byte   0,0,0   /* Padding */
link17: .long   link18 - link17
        .asciz  "dir$"
diradr: .long   0       /* Load directory address */
dirlen: .long   0       /* Load directory length */

        .byte   0,0,0   /* Padding */
link18: .long   link19 - link18
        .asciz  "lib$"
libadr: .long   0       /* Lib directory address */
liblen: .long   0       /* Lib directory length */

        .byte   0,0,0   /* Padding */
link19: .long   link20 - link19
        .asciz  "cmd$"
cmdadr: .long   0       /* Command line address */
cmdlen: .long   0       /* Command line length */

        .byte   0,0,0   /* Padding */
link20: .long   link21 - link20
        .asciz  "usr$"
usradr: .long   0       /* User directory address */
usrlen: .long   0       /* User directory length */
   
        .byte   0,0,0   /* Padding */
link21: .long   link00 - link21
        .asciz  "tmp$"
tmpadr: .long   0       /* Temp directory address */
tmplen: .long   0       /* Temp directory length */


         .byte  0,0,0   /* Padding */
link00: .long   0       /* End of list */
        .asciz  "fn%("
        .long   fnarr   /* Pointer to function array */


/* Array of function entry points */
fnarr:  .byte   1               /* Number of dimensions */
        .long   21              /* Number of entries */
        .long   loadn           /* Load numeric */
        .long   loads           /* Load string */
        .long   storen          /* Store numeric */
        .long   stores          /* Store string */
        .long   getvar          /* Get variable address & type */
        .long   putvar          /* Create a variable */
        .long   expr            /* Evaluate expression */
        .long   item            /* Evaluate item */
        .long   lexan           /* Lexical analysis (tokenise) */
        .long   token           /* Print character or keyword */
        .long   xfloat          /* Convert integer to float */
        .long   xfix            /* Convert float to integer */
        .long   str             /* Convert a number to a string */
        .long   con             /* Convert a string to a number */
        .long   0               /* Reserved for sortup */
        .long   0               /* Reserved for sortdn */
        .long   0               /* Reserved for hook */
        .long   xeq             /* Run BASIC code */
        .long   putevt          /* Put event in queue */
        .long   gfxPrimitivesGetFont
        .long   gfxPrimitivesSetFont

        .text

fvtab:  .byte   1               /* &19 v&  Unsigned byte (8 bits) */
        .byte   4               /* &1A v%  Signed dword (32 bits) */
        .byte   8               /* &1B v#  Float double (64 bits) */
        .byte   10              /* &1C v   Variant numeric (80 bits) */
        .byte   24              /* &1D v{} Structure (4+4 bytes) */
        .byte   40              /* &1E v%% Signed qword (64 bits) */
        .byte   136             /* &1F v$  String (4+4 bytes) */
   
