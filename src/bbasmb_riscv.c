/*****************************************************************\
*       32-bit BBC BASIC Interpreter                              *
*       (c) 2018-2021  R.T.Russell  http://www.rtrussell.co.uk/   *
*                                                                 *
*       bbasmb.c: Simple ARM 4 assembler                          *
*       Version 1.24a, 12-Jul-2021                                *
\*****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "BBC.h"

#ifndef __WINDOWS__
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#if defined(__x86_64__) || defined(__aarch64__)
#define OC ((unsigned int) stavar[15] + (void *)((long long) stavar[13] << 32))
#define PC ((unsigned int) stavar[16] + (void *)((long long) stavar[17] << 32))
#else
#define OC (void *) stavar[15]
#define PC (void *) stavar[16]
#endif

// External routines:
void newlin (void) ;
void *getput (unsigned char *) ;
void error (int, const char *) ;
void token (signed char) ;
void text (const char*) ;
void crlf (void) ;
void comma (void) ;
void spaces (int) ;
int range0 (char) ;
signed char nxt (void) ;

long long itemi (void) ;
long long expri (void) ;
VAR expr (void) ;
VAR exprs (void) ;
VAR loadn (void *, unsigned char) ;
void storen (VAR, void *, unsigned char) ;

static char *mnemonics[] = {
   "addi",
   "add",
   "align",
   "andi",
   "and",
   "auipc",
   "beq",
   "bgeu",
   "bge",
   "bltu",
   "blt",
   "bne",
   "db",
   "dcb",
   "dcd",
   "dcs",
   "dcw",
   "divu",
   "div",
   "ebreak",
   "ecall",
   "equb",
   "equd",
   "equq",
   "equs",
   "equw",
   "jalr",
   "jal",
   "lbu",
   "lb",
   "lhu",
   "lh",
   "lui",
   "lw",
   "mulh",
   "mulsu",
   "mulu",
   "mul",
   "opt",
   "ori",
   "or",
   "remu",
   "rem",
   "ret",
   "sb",
   "sh",
   "slli",
   "sll",
   "slti",
   "sltui",
   "sltu",
   "slt",
   "srai",
   "sra",
   "srli",
   "srl",
   "sub",
   "sw",
   "xori",
   "xor"
};

static uint32_t opcodes[] = {
   0x00000013, // addi
   0x00000033, // add
   0xffffffff, // align
   0x00007013, // andi
   0x00007033, // and
   0x00000017, // auipc
   0x00000063, // beq
   0x00007063, // bgeu
   0x00005063, // bge
   0x00006063, // bltu
   0x00004063, // blt
   0x00001063, // bne
   0xffffffff, // db
   0xffffffff, // dcb
   0xffffffff, // dcd
   0xffffffff, // dcs
   0xffffffff, // dcw
   0x02005033, // divu
   0x02004033, // div
   0x00100073, // ebreak
   0x00000073, // ecall
   0xffffffff, // equb
   0xffffffff, // equd
   0xffffffff, // equq
   0xffffffff, // equs
   0xffffffff, // equw
   0x00000067, // jalr
   0x0000006f, // jal
   0x00004003, // lbu
   0x00000003, // lb
   0x00005003, // lhu
   0x00001003, // lh
   0x00000037, // lui
   0x00002003, // lw
   0x02001033, // mulh
   0x02002033, // mulsu
   0x02003033, // mulu
   0x02000033, // mul
   0xffffffff, // opt
   0x00006013, // ori
   0x00006033, // or
   0x02007033, // remu
   0x02006033, // rem
   0x00008067, // ret      // jalr zero, ra 0
   0x00000023, // sb
   0x00001023, // sh
   0x00001013, // slli
   0x00001033, // sll
   0x00002013, // slti
   0x00003013, // sltui
   0x00003033, // sltu
   0x00002033, // slt
   0x40005013, // srai
   0x40005033, // sra
   0x00005013, // srli
   0x00005033, // srl
   0x40000033, // sub
   0x00002023, // sw
   0x00004013, // xori
   0x00004033  // xor
};

enum {
   ADDI,
   ADD,
   ALIGN,
   ANDI,
   AND,
   AUIPC,
   BEQ,
   BGEU,
   BGE,
   BLTU,
   BLT,
   BNE,
   DB,
   DCB,
   DCD,
   DCS,
   DCW,
   DIVU,
   DIV,
   EBREAK,
   ECALL,
   EQUB,
   EQUD,
   EQUQ,
   EQUS,
   EQUW,
   JALR,
   JAL,
   LBU,
   LB,
   LHU,
   LH,
   LUI,
   LW,
   MULH,
   MULSU,
   MULU,
   MUL,
   OPT,
   ORI,
   OR,
   REMU,
   REM,
   RET,
   SB,
   SH,
   SLLI,
   SLL,
   SLTI,
   SLTUI,
   SLTU,
   SLT,
   SRAI,
   SRA,
   SRLI,
   SRL,
   SUB,
   SW,
   XORI,
   XOR
};

static char *registers[] = {
   "a0",
   "a1",
   "a2",
   "a3",
   "a4",
   "a5",
   "a6",
   "a7",
   "gp",
   "ra",
   "s0",
   "s10",
   "s11",
   "s1",
   "s2",
   "s3",
   "s4",
   "s5",
   "s6",
   "s7",
   "s8",
   "s9",
   "sp",
   "t0",
   "t1",
   "t2",
   "t3",
   "t4",
   "t5",
   "t6",
   "tp",
   "x0",
   "x10",
   "x1",
   "x11",
   "x12",
   "x13",
   "x14",
   "x15",
   "x16",
   "x17",
   "x18",
   "x19",
   "x20",
   "x21",
   "x2",
   "x22",
   "x23",
   "x24",
   "x25",
   "x26",
   "x27",
   "x28",
   "x29",
   "x30",
   "x31",
   "x3",
   "x4",
   "x5",
   "x6",
   "x7",
   "x8",
   "x9",
   "zero"
};

static unsigned char regno[] = {
   10, // a0
   11, // a1
   12, // a2
   13, // a3
   14, // a4
   15, // a5
   16, // a6
   17, // a7
   3,  // gp
   1,  // ra
   8,  // s0
   26, // s10
   27, // s11
   9,  // s1
   18, // s2
   19, // s3
   20, // s4
   21, // s5
   22, // s6
   23, // s7
   24, // s8
   25, // s9
   2,  // sp
   5,  // t0
   6,  // t1
   7,  // t2
   28, // t3
   29, // t4
   30, // t5
   31, // t6
   4,  // tp
   0,  // x0
   10, // x10
   1,  // x1
   11, // x11
   12, // x12
   13, // x13
   14, // x14
   15, // x15
   16, // x16
   17, // x17
   18, // x18
   19, // x19
   20, // x20
   21, // x21
   2,  // x2
   22, // x22
   23, // x23
   24, // x24
   25, // x25
   26, // x26
   27, // x27
   28, // x28
   29, // x29
   30, // x30
   31, // x31
   3,  // x3
   4,  // x4
   5,  // x5
   6,  // x6
   7,  // x7
   8,  // x8
   9,  // x9
   0   // zero
};

static int lookup (char **arr, int num)
{
   int i, n ;
   for (i = 0; i < num; i++)
      {
         n = strlen (*(arr + i)) ;
         if (strnicmp ((const char *)esi, *(arr + i), n) == 0)
            break ;
      }
   if (i >= num)
      return -1 ;
   esi += n ;
   return i ;
}

static unsigned char reg (void)
{
   int i ;
   nxt () ;
   i = lookup (registers, sizeof(registers) / sizeof(registers[0])) ;
   if (i < 0)
      {
         i = itemi() ;
         if ((i < 0) || (i > 31))
            error (16, NULL) ; // 'Syntax error'
         return i ;
      }
   return regno[i] ;
}

static unsigned int imm12(void) {
   long long immediate = expri();
   // immediate is a 12-bit signed
   if (immediate < -0x8000 || immediate >= 0x8000) {
      error (16, NULL) ; // 'Syntax error'
   }
   return ((unsigned int) immediate) & 0xFFF;
}

static unsigned int imm20(void) {
   long long immediate = expri();
   // immediate is a 20-bit signed
   if (immediate < -0x80000 || immediate >= 0x80000) {
      error (16, NULL) ; // 'Syntax error'
   }
   return ((unsigned int) immediate) & 0xFFFFF;
}

static void tabit (int x)
{
   if (vcount == x)
      return ;
   if (vcount > x)
      crlf () ;
   spaces (x - vcount) ;
}

static void poke (void *p, int n)
{
   char *d ;
   if (liston & BIT6)
      {
         d = OC ;
         stavar[15] += n ;
      }
   else
      d = PC ;

   stavar[16] += n ;
   memcpy (d, p, n) ;
}

static void *align (void)
{
   while (stavar[16] & 3)
      {
         stavar[16]++ ;
         if (liston & BIT6)
            stavar[15]++ ;
      } ;
   return PC ;
}

void assemble (void)
{
   signed char al ;
   signed char *oldesi = esi ;
   int init = 1 ;
   void *oldpc = PC ;

   while (1)
      {
         int mnemonic, instruction = 0 ;

         if (liston & BIT7)
            {
               int tmp ;
               if (liston & BIT6)
                  tmp = stavar[15] ;
               else
                  tmp = stavar[16] ;
               if (tmp >= stavar[12])
                  error (8, NULL) ; // 'Address out of range'
            }

         al = nxt () ;
         esi++ ;

         switch (al)
            {
            case 0:
               esi-- ;
               liston = (liston & 0x0F) | 0x30 ;
               return ;

            case ']':
               liston = (liston & 0x0F) | 0x30 ;
               return ;

            case 0x0D:
               newlin () ;
               if (*esi == 0x0D)
                  break ;
            case ':':
               if (liston & BIT4)
                  {
                     void *p ;
                     int n = PC - oldpc ;
                     if (liston & BIT6)
                        p = OC - n ;
                     else
                        p = PC - n ;

                     do
                        {
                           unsigned int i = *(unsigned int *)p ;
                           sprintf (accs, "%08lX ", (long) (size_t) oldpc) ;
                           switch (n)
                              {
                              case 0: break ;
                              case 1: i &= 0xFF ;
                              case 2: i &= 0xFFFF ;
                              case 3: i &= 0xFFFFFF ;
                              case 4: sprintf (accs + 9, "%0*X ", n*2, i) ;
                                 break ;
                              default: sprintf (accs + 9, "%08X ", i) ;
                              }
                           if (n > 4)
                              {
                                 n -= 4 ;
                                 p += 4 ;
                                 oldpc += 4 ;
                              }
                           else
                              n = 0 ;

                           text (accs) ;

                           if (*oldesi == '.')
                              {
                                 tabit (18) ;
                                 do
                                    {
                                       token (*oldesi++ ) ;
                                    }
                                 while (range0(*oldesi)) ;
                                 while (*oldesi == ' ') oldesi++ ;
                              }
                           tabit (30) ;
                           while ((*oldesi != ':') && (*oldesi != 0x0D))
                              token (*oldesi++) ;
                           crlf () ;
                        }
                     while (n) ;
                  }
               nxt () ;
#ifdef __arm__
               if ((liston & BIT6) == 0)
                  __builtin___clear_cache (oldpc, PC) ;
#endif
               oldpc = PC ;
               oldesi = esi ;
               break ;

            case ';':
            case TREM:
               while ((*esi != 0x0D) && (*esi != ':')) esi++ ;
               break ;

            case '.':
               if (init)
                  oldpc = align () ;
               {
                  VAR v ;
                  unsigned char type ;
                  void *ptr = getput (&type) ;
                  if (ptr == NULL)
                     error (16, NULL) ; // 'Syntax error'
                  if (type >= 128)
                     error (6, NULL) ; // 'Type mismatch'
                  if ((liston & BIT5) == 0)
                     {
                        v = loadn (ptr, type) ;
                        if (v.i.n)
                           error (3, NULL) ; // 'Multiple label'
                     }
                  v.i.t = 0 ;
                  v.i.n = (intptr_t) PC ;
                  storen (v, ptr, type) ;
               }
               break ;

            default:
               esi-- ;
               mnemonic = lookup (mnemonics, sizeof(mnemonics)/sizeof(mnemonics[0])) ;

               if (mnemonic != OPT)
                  init = 0 ;

               switch (mnemonic)
                  {
                  case OPT:
                     liston = (liston & 0x0F) | (expri () << 4) ;
                     continue ;

                  case ALIGN:
                     oldpc = align () ;
                     if ((nxt() >= '1') && (*esi <= '9'))
                        {
                           int n = expri () ;
                           if ((n & (n - 1)) || (n & 0xFFFFFF03) || (n == 0))
                              error (16, NULL) ; // 'Syntax error'
                           instruction = 0xE1A00000 ;
                           while (stavar[16] & (n - 1))
                              poke (&instruction, 4) ;
                        }
                     continue ;

                  case DB:
                     {
                        VAR v = expr () ;
                        if (v.s.t == -1)
                           {
                              if (v.s.l > 256)
                                 error (19, NULL) ; // 'String too long'
                              poke (v.s.p + zero, v.s.l) ;
                              continue ;
                           }
                        if (v.i.t)
                           v.i.n = v.f ;
                        poke (&v.i.n, 1) ;
                        continue ;
                     }

                  case DCB:
                  case EQUB:
                     {
                        int n = expri () ;
                        poke (&n, 1) ;
                        continue ; // n.b. not break
                     }

                  case DCW:
                  case EQUW:
                     {
                        int n = expri () ;
                        poke (&n, 2) ;
                        continue ; // n.b. not break
                     }

                  case DCD:
                  case EQUD:
                  case EQUQ:
                     {
                        VAR v = expr () ;
                        long long n ;
                        if (v.s.t == -1)
                           {
                              error (6, NULL) ; // 'Type mismatch'
                           }
                        else if (v.i.t == 0)
                           n = v.i.n ;
                        else
                           n = v.f ;
                        if (mnemonic == EQUQ)
                           {
                              poke (&n, 8) ;
                              continue ;
                           }
                        instruction = (int) n ;
                     }
                     break ;

                  case DCS:
                  case EQUS:
                     {
                        VAR v = exprs () ;
                        if (v.s.l > 256)
                           error (19, NULL) ; // 'String too long'
                        poke (v.s.p + zero, v.s.l) ;
                        continue ;
                     }

                  case ADD:
                  case SUB:
                  case XOR:
                  case OR:
                  case AND:
                  case SLL:
                  case SRL:
                  case SRA:
                  case SLT:
                  case SLTU:
                  case MUL:
                  case MULH:
                  case MULSU:
                  case MULU:
                  case DIV:
                  case DIVU:
                  case REM:
                  case REMU:
                     // Format R
                     // e.g. add rd, rs1, rs2
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 7; // rd
                        comma();
                        instruction |= reg() << 15; // rs1
                        comma();
                        instruction |= reg() << 20; // rs2
                     }
                     break;

                  case ADDI:
                  case XORI:
                  case ORI:
                  case ANDI:
                  case SLLI:
                  case SRLI:
                  case SRAI:
                  case SLTI:
                  case SLTUI:
                  case JALR:
                     // Format I
                     // e.g. addi rd, rs1, immediate
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 7; // rd
                        comma();
                        instruction |= reg() << 15; // rs1
                        comma();
                        instruction |= imm12() << 20; // imm12
                     }
                     break;

                  case LB:
                  case LH:
                  case LW:
                  case LBU:
                  case LHU:
                     // Format I
                     // e.g. lb rd, immediate(rs1)
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 7; // rd
                        comma();
                        instruction |= imm12() << 20; // imm12
                        if (nxt () != '(') {
                           error (27, "Missing (") ; // 'Missing ('
                        }
                        esi++ ;
                        instruction |= reg() << 15; // rs1
                        if (nxt () != ')') {
                           error (27, NULL) ; // 'Missing )'
                        }
                        esi++ ;
                     }
                     break;

                  case SB:
                  case SH:
                  case SW:
                     // Format S
                     // e.g. sb rs2, immediate(rs1)
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 20; // rs2
                        comma();
                        unsigned int u = imm12(); // imm12
                        instruction |= (u & 0x01f) << 7;
                        instruction |= (u & 0xfe0) << 20;
                        if (nxt () != '(') {
                           error (27, "Missing (") ; // 'Missing ('
                        }
                        esi++ ;
                        instruction |= reg() << 15; // rs1
                        if (nxt () != ')') {
                           error (27, NULL) ; // 'Missing )'
                        }
                        esi++ ;
                     }
                     break;

                  case BEQ:
                  case BNE:
                  case BLT:
                  case BGE:
                  case BLTU:
                  case BGEU:
                     {
                        // Format B
                        // bne rs1, rs2, target
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 15; // rs1
                        comma();
                        instruction |= reg() << 20; // rs2
                        comma();
                        int dest = ((void *) (size_t) expri () - PC) >> 1 ;
                        if ((dest < -0x800 || dest >= 0x800) && ((liston & BIT5) != 0)) {
                           error (1, NULL) ; // 'Jump out of range'
                        }
                        unsigned int u = (dest & 0xFFF) << 1;
                        instruction |= ((u >> 12) & 0x01) << 31;
                        instruction |= ((u >>  5) & 0x3f) << 25;
                        instruction |= ((u >>  1) & 0x0f) << 8;
                        instruction |= ((u >> 11) & 0x01) << 7;
                     }
                     break;

                  case JAL:
                     // Format J
                     // jal rd, target
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 7; // rd
                        comma();
                        int dest = ((void *) (size_t) expri () - PC) >> 1 ;
                        if ((dest < -0x80000 || dest >= 0x80000) && ((liston & BIT5) != 0)) {
                           error (1, NULL) ; // 'Jump out of range'
                        }
                        unsigned int u = (dest & 0xFFFFF) << 1;
                        // 20 | 10:1 | 11 : 19:12
                        instruction |= ((u >> 20) & 0x001) << 31;
                        instruction |= ((u >>  1) & 0x3ff) << 21;
                        instruction |= ((u >> 11) & 0x001) << 20;
                        instruction |= ((u >> 12) & 0x0ff) << 12;
                     }
                     break ;

                  case LUI:
                     // Formal U
                     // e.g. lui rd, immediate
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 7; // rd
                        comma();
                        instruction |= imm20() << 12; // imm20
                     }
                     break;

                  case AUIPC:
                     // Formal U
                     // e.g. auipc rd, target
                     {
                        instruction = opcodes[mnemonic];
                        instruction |= reg() << 7; // rd
                        comma();
                        int dest = ((void *) (size_t) expri () - PC);
                        if ((dest < -0x80000 || dest >= 0x80000) && ((liston & BIT5) != 0)) {
                           error (1, NULL) ; // 'Jump out of range'
                        }
                        instruction |= ((unsigned int) dest) << 12; // imm20
                     }
                     break;


                  case ECALL:
                  case EBREAK:
                  case RET:
                     {
                        instruction = opcodes[mnemonic];
                     }
                     break;


                  default:
                     error (16, NULL) ; // 'Syntax error'
                  }

               oldpc = align () ;

               poke (&instruction, 4) ;
            }
      } ;
}
