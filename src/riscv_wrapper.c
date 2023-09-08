#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "BBC.h"
#include "version.h"

// From bbccon.h
extern char *accs;              // String accumulator
extern char *buff;              // Temporary string buffer
//extern char* path;              // File path buffer
#define PAGE_OFFSET ACCSLEN + 0xc00     // Offset of PAGE from memory base

// Global variables (external linkage):
void *userRAM = NULL;
void *progRAM = NULL;
void *userTOP = NULL;
const char szVersion[] = "BBC BASIC for RISC-V Co Processor "VERSION;
const char szNotice[] = "(C) Copyright R. T. Russell, "YEAR;
char *szLoadDir;
char *szLibrary;
char *szUserDir;
char *szTempDir;
char *szCmdLine;
static int MaximumRAM = 0;
//timer_t UserTimerID;
unsigned int palette[256];

// Malloc heap memory
static char *__heap_start;
static char *__heap_end;
static char *brk;

// Declared in bbmain.c:
void error(int, const char *);
void text(const char *);       // Output NUL-terminated string
void crlf(void);               // Output a newline
void trap (void);              // Test for ESCape
int basic(void *ecx, void *edx, void *prompt);

// Declared in bbeval.c:
unsigned int rnd(void);        // Return a pseudo-random number

typedef void (*handler_fn_type)(int a0);

extern void error_handler();
extern void escape_handler();
static void _osbyte(int al, int *x, int *y, int *c);

heapptr oshwm(void *addr, int settop) {  // Allocate memory above HIMEM
   if ((addr < userRAM) ||(addr > (userRAM + MaximumRAM))) {
      return 0;
   } else {
      if (settop && (addr > userTOP)) {
         userTOP = addr;
      }
      return (size_t) addr;
   }
}

void oshandlers(unsigned int num, void *handler_fn, void *handler_data, void **old_handler_fn, void **old_handler_data) {
   register int   a0 asm ("a0") = num;
   register void *a1 asm ("a1") = handler_fn;
   register void *a2 asm ("a2") = handler_data;
   register int   a7 asm ("a7") = 14;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a1),
                   "+r" (a2)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a2),
                   "r"  (a7)
                 );
   if (old_handler_fn) {
      *old_handler_fn = a1;
   }
   if (old_handler_data) {
      *old_handler_data = a2;
   }
}

int _main(char *params) {

   void* immediate = (void *) 1;

   _osbyte(0x83, (int *)&userRAM, NULL, NULL);
   progRAM = userRAM + PAGE_OFFSET; // Will be raised if @cmd$ exceeds 255 bytes
   _osbyte(0x84, (int *)&userTOP, NULL, NULL);

   // Allow 256KB for CPU Stack
   userTOP -= 0x40000;

   // Allow 256B for Malloc Heap
   __heap_end = userTOP;
   userTOP -= 0x40000;
   __heap_start = brk = userTOP;

   // Start by allocating half the remaining memory
   MaximumRAM = userTOP - userRAM;
   userTOP = (void *)((uint32_t)userTOP >> 1);

   szCmdLine = progRAM - 0x100;     // Must be immediately below default progRAM
   szTempDir = szCmdLine - 0x100;   // Strings must be allocated on BASIC's heap
   szUserDir = szTempDir - 0x100;
   szLibrary = szUserDir - 0x100;
   szLoadDir = szLibrary - 0x100;

   *szCmdLine = 0;
   strcpy(szTempDir, "/");
   strcpy(szUserDir, "/");
   strcpy(szLibrary, "/");
   strcpy(szLoadDir, "/");

   accs = (char*) userRAM;                 // String accumulator
   buff = (char*) accs + ACCSLEN;          // Temporary string buffer
   //   path = (char*) buff + 0x100;            // File path

   prand.l = 12345678;                    /// Seed PRNG
   prand.h = (prand.l == 0);
   rnd();                                 // Randomise !

   if (immediate == (void *) 1)
      {
         text(szVersion);
         crlf();
         text(szNotice);
         crlf();
      }

   // Install new handlers
   void *old_error_handler;
   void *old_escape_handler;

   oshandlers(0xfffd, error_handler,  0, &old_error_handler,  NULL);
   oshandlers(0xfffe, escape_handler, 0, &old_escape_handler, NULL);

   int ret = basic(progRAM, userTOP, immediate);

   // Restore old handlers
   oshandlers(0xfffd, old_error_handler,  0, NULL, NULL);
   oshandlers(0xfffe, old_escape_handler, 0, NULL, NULL);

   return ret;
}

int _brk(void *addr)
{
   brk = addr;
   return 0;
}

void *_sbrk(ptrdiff_t incr)
{
   char *old_brk = brk;

   if (__heap_start == __heap_end) {
      return NULL;
   }

   if ((brk += incr) < __heap_end) {
      brk += incr;
   } else {
      brk = __heap_end;
   }
   return old_brk;
}

// Dummy functions:
void gfxPrimitivesSetFont(void) {
};

void gfxPrimitivesGetFont(void) {
};


// In bbcvdu.c

long long apicall_(long long (*APIfunc) (size_t, size_t, size_t, size_t, size_t, size_t,
                                          size_t, size_t, size_t, size_t, size_t, size_t), PARM *p) {
   text("TODO: apicall_");
   crlf();
   return 0;
}

double fltcall_(double (*APIfunc) (size_t, size_t, size_t, size_t, size_t, size_t,
                                    size_t, size_t, size_t, size_t, size_t, size_t,
                                    double, double, double, double, double, double, double, double), PARM *p) {
   text("TODO: fltcall_");
   crlf();
   return 0.0;
}


long double floorl(long double x) {
   return floor(x);
}

long double truncl(long double x) {
   return trunc(x);
}

// Simplified local OSCLI

#define NCMDS 3

static char *cmds[NCMDS] = {
   "float",
   "hex",
   "lowercase"
};

enum {
   FLOAT,
   HEX,
   LOWERCASE
};

// Parse ON or OFF:
static int onoff (char *p)
{
   int n = 0 ;
   if ((*p & 0x5F) == 'O') {
      p++;
   }
   sscanf (p, "%x", &n);
   return !n;
}

// Parse / Execute a local command
static int localcmd(char *cmd) {

   int b;
   int n;
   char *p;

   while (*cmd == ' ') cmd++;

   if ((*cmd == 0x0D) || (*cmd == '|') || (*cmd == '.'))
      return 0;

   // Simple match that deliberately doesn't allow abbreviations
   // (otherwise *H. matches *HEX rather than *HELP
   p = NULL;
   for (b = 0; b < NCMDS; b++) {
      n = strlen(cmds[b]);
      if (!strncasecmp(cmd, cmds[b], n)) {
         // parameters follow the command
         p = cmd + n;
         break;
      }
   }

   if (!p) {
      return 0;
   }

   while (*p == ' ') {
      p++;    // Skip leading spaces
   }

   switch (b) {
      case FLOAT:
         n = 0;
         sscanf (p, "%i", &n);
         switch (n) {
            case 40:
               liston &= ~(BIT0 + BIT1);
               break;
            case 64:
               liston &= ~BIT1;
               liston |= BIT0;
               break;
            case 80:
               liston |= (BIT0 + BIT1);
               break;
            default:
               error (254, "Bad command");
         }
         return 1;
      case HEX:
         n = 0;
         sscanf (p, "%i", &n);
         switch (n) {
            case 32:
               liston &= ~BIT2;
               break;
            case 64:
               liston |= BIT2;
               break;
            default:
               error (254, "Bad command");
         }
         return 1;
      case LOWERCASE:
         if (onoff (p)) {
            liston |= BIT3;
         } else {
            liston &= ~BIT3;
         }
         return 1;
   }
   return 0;
}

// ===================================================================
// Private Low level MOS Interface
// ===================================================================

static void _oswrch(unsigned char vdu) {   // Write to display or other output stream (VDU)
   register int a0 asm ("a0") = vdu;
   register int a7 asm ("a7") = 4;
   asm volatile ("ecall"
                 : // outputs

                 : // inputs
                   "r"  (a0),
                   "r"  (a7)
                 );
}

static int _osrdch(void) { // Get character from console input
   register int a0 asm ("a0");
   register int a7 asm ("a7") = 6;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0)
                 : // inputs
                   "r"  (a7)
                 );
   return a0;
}

static void _osbyte(int al, int *x, int *y, int *c) {
   register int a0 asm ("a0") = al;
   register int a1 asm ("a1") = x ? (*x) : 0;
   register int a2 asm ("a2") = y ? (*y) : 0;
   register int a3 asm ("a3");
   register int a7 asm ("a7") = 2;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a1),
                   "+r" (a2),
                   "+r" (a3)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a2),
                   "r"  (a7)
                 );
   if (x) {
      *x = a1;
   }
   if (y) {
      *y = a2;
   }
   if (c) {
      *c = a3;
   }
}

static int _osword(int al, void *xy) {
   register int a0 asm ("a0") = al;
   register int *a1 asm ("a1") = xy;
   register int a2 asm ("a2") = 0;
   register int a7 asm ("a7") = 3;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a2)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a7)
                 : // clobber
                   "memory"
                 );
   return a2;
}

static unsigned long _osfile(int reason, char *filename, uint32_t *load, uint32_t *exec, uint32_t *start, uint32_t *end) {
   register int      a0 asm ("a0") = reason;
   register char    *a1 asm ("a1") = filename;
   register uint32_t a2 asm ("a2") = load  ? (*load ) : 0;
   register uint32_t a3 asm ("a3") = exec  ? (*exec ) : 0;
   register uint32_t a4 asm ("a4") = start ? (*start) : 0;
   register uint32_t a5 asm ("a5") = end   ? (*end  ) : 0;
   register int   a7 asm ("a7") = 7;
   asm volatile ("ecall"
                 : // outputs
                   "+r"  (a0),
                   "+r"  (a2),
                   "+r"  (a3),
                   "+r"  (a4),
                   "+r"  (a5)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a2),
                   "r"  (a3),
                   "r"  (a4),
                   "r"  (a5),
                   "r"  (a7)
                 );
   if (load) {
      *load = a2;
   }
   if (exec) {
      *exec = a3;
   }
   if (start) {
      *start = a4;
   }
   if (end) {
      *end = a5;
   }
   return a0;
}

static void * _osfind(int a, char *p) {
   register int   a0 asm ("a0") = a;
   register void *a1 asm ("a1") = p;
   register int   a7 asm ("a7") = 12;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a7)
                 );
   return (void *) a0;
}

static int _osbget(void *chan, int *peof) { // Read a byte from a file
   register int   a0 asm ("a0");
   register void *a1 asm ("a1") = chan;
   register int   a7 asm ("a7") = 9;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0)
                 : // inputs
                   "r"  (a1),
                   "r"  (a7)
                 );
   if (peof) {
      if (a0 < 0) {
         *peof = -1; // TRUE
      } else {
         *peof = 0; // FALSE
      }
   }
   return a0 & 0xff;
}

static void _osbput(void *chan, unsigned char byte) { // Write a byte to a file
   register int   a0 asm ("a0") = byte;
   register void *a1 asm ("a1") = chan;
   register int   a7 asm ("a7") = 10;
   asm volatile ("ecall"
                 : // outputs
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a7)
                 );
}


static int _osargs(int reason, void *chan, void *data) {
   register int           a0 asm ("a0") = reason;
   register void         *a1 asm ("a1") = chan;
   register uint32_t      a2 asm ("a2") = *(uint32_t *)data;
   register int           a7 asm ("a7") = 8;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0),
                   "+r" (a2)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a2),
                   "r"  (a7)
                 );
   if (data) {
      *(uint32_t *)data = a2;
   }
   return a0;
}

static int _osgbpb(uint8_t reason, uint8_t chan, uint32_t *data, uint32_t *num, uint32_t *ptr, int *peof) {
   register uint32_t a0 asm ("a0") = reason;
   register uint32_t a1 asm ("a1") = chan;
   register uint32_t a2 asm ("a2") = data ? (*data) : 0;
   register uint32_t a3 asm ("a3") = num  ? (*num ) : 0;
   register uint32_t a4 asm ("a4") = ptr  ? (*ptr ) : 0;
   register int   a7 asm ("a7") = 11;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0),
                   "+r" (a2),
                   "+r" (a3),
                   "+r" (a4)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a2),
                   "r"  (a3),
                   "r"  (a4),
                   "r"  (a7)
                 );
   if (data) {
      *data = a2;
   }
   if (num) {
      *num = a3;
   }
   if (ptr) {
      *ptr = a4;
   }
   *peof = (a0 & 0x80000000) ? 1 : 0;
   return a0 & 0xff;
}

static void _oscli(char *cmd) {      // Execute an emulated OS command
   if (localcmd(cmd)) {
      return;
   }
   register int a0 asm ("a0") = (int) cmd;
   register int a7 asm ("a7") = 1;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0)
                 : // inputs
                   "r"  (a0),
                   "r"  (a7)
                 );
}

// ===================================================================
// BBC Basic MOS API Abstraction
// ===================================================================

// Helpers to avoid unaligned accessses

static uint16_t read16(uint8_t *addr) {
   uint16_t data = *addr++;
   data |= (*addr++) << 8;
   return data;
}

static uint32_t read32(uint8_t *addr) {
   uint32_t data = *addr++;
   data |= (*addr++) << 8;
   data |= (*addr++) << 16;
   data |= (*addr++) << 24;
   return data;
}

static void write32(uint8_t *addr, uint32_t data) {
   *addr++ = (uint8_t) (data & 0xff);
   *addr++ = (uint8_t) ((data >> 8) & 0xff);
   *addr++ = (uint8_t) ((data >> 16) & 0xff);
   *addr++ = (uint8_t) ((data >> 24) & 0xff);
}

// returns PPYYXXAA, as expected by USR
int oscall(int addr) {    // Call an emulated OS function
   int a = stavar[1];
   int x = stavar[24];
   int y = stavar[25];
   int ret;
   int c = 0;
   // Heuristic to determine 32-bit block address
   //
   // The call pattern is likely to be either
   //
   // X% = block
   // Y% = block DIV 256
   // CALL osword
   //
   // X% = block MOD 256
   // Y% = block DIV 256
   // CALL osword
   //
   uint8_t *block = (uint8_t *)((x < 0x100) ? (x | (y << 8)) : x);
   switch (addr) {
   case 0xFFCE:
      if (a) {
         // Open file
         a = ((uint32_t)_osfind(a, (char *)block)) & 0xff; // filename in xy
      } else {
         // Close file
         _osfind(a, (void *)y); // handle in y
      }
      break;
   case 0xFFD1:
      {
         uint8_t  chan = *block;
         uint32_t data = read32(block + 1);
         uint32_t num  = read32(block + 5);
         uint32_t ptr  = read32(block + 9);
         a = _osgbpb(a, chan, &data, &num, &ptr, &c);
         write32(block + 1, data);
         write32(block + 5, num);
         write32(block + 9, ptr);
         break;
      }
   case 0xFFD4:
      _osbput((void *)y, a);
      break;
   case 0xFFD7:
      a = _osbget((void *) y, &c); // return -1 in c for EOF, otherwise 0
      c = c ? 1 : 0; // map EOF to C=1
      break;
   case 0xFFDA:
      ret = _osargs(a, (void *)y, (void *)x);
      if (a == 0) {
         a = ret & 0xff;
      }
      break;
   case 0xFFDD:
      {
         // On 8-bit MOS, filename is only a two byte thing, so the
         // best we can do is to remap this assuming the string is
         // likely to be in the same 64K address range as the
         // control block.
         char *filename = (char *)(((uint32_t)block) & 0xffff0000) + read16(block);
         uint32_t load  = read32(block +  2);
         uint32_t exec  = read32(block +  6);
         uint32_t start = read32(block + 10);
         uint32_t end   = read32(block + 14);
         a = _osfile(a, filename, &load, &exec, &start, &end) & 0xff;
         write32(block +  2, load);
         write32(block +  6, exec);
         write32(block + 10, start);
         write32(block + 14, end);
         break;
      }
   case 0xFFE0:
      ret = _osrdch();
      if (ret < 0) {
         c = 1;
      }
      a = ret & 0xff;
      break;
   case 0xFFE3:
      if (a == 13) {
         _oswrch(10);
      }
      _oswrch(a);
      break;
   case 0xFFE7:
      _oswrch(10);
      _oswrch(13);
      break;
   case 0xFFEC:
      _oswrch(13);
      break;
   case 0xFFEE:
      _oswrch(a & 0xff);
      break;
   case 0xFFF1:
      _osword(a, block);
      break;
   case 0xFFF4:
      _osbyte(a, &x, &y, &c);
      break;
   case 0xFFF7:
      _oscli((void *) x);
      break;
   default:
      text("TODO: unexpected emulated oscall address");
      crlf();
   }
   return ((c & 0x01) << 24) + ((y & 0xff) << 16) + ((x & 0xff) << 8) + (a & 0xff);
}

void oscli(char *cmd) {      // Execute an emulated OS
   _oscli(cmd);
}

int oskey(int wait) {     // Wait for character or test key
   int x = wait & 0xff;
   int y = (wait >> 8) & 0xff;
   _osbyte(0x81, &x, &y, NULL);
   if (y) {
      return -1;
   } else {
      return x & 0xff;
   }
}

void oswrch(unsigned char vdu) {   // Write to display or other output stream (VDU)
   _oswrch(vdu);
}

unsigned char osrdch(void) { // Get character from console input
   return _osrdch() & 0xff;
}

void osline(char *buffer) {     // Get a line of console input
   unsigned int block[4];
   block[0] = (unsigned int) buffer;
   block[1] = 0xff;
   block[2] = 32;
   block[3] = 255;
   int ret = _osword(0, block);
   // If escape, make sure the line has a terminator
   if (ret < 0) {
      trap();
   }
}

// MOS - File

void osload(char *p, void *addr, int max) { // Load a file to memory
   uint32_t load = (uint32_t) addr;
   _osfile(255, p, &load, NULL, NULL, NULL);
}

void ossave(char *p, void *addr, int len) { // Save a file from memory
   uint32_t start = (uint32_t) addr;
   uint32_t end   = (uint32_t) (addr + len);
   _osfile(0, p, NULL, NULL, &start, &end);
}

void *osopen(int type, char *p) { // Open a file
   if (type == 0) {
      return _osfind(0x40, p); // input only
   } else if (type == 1) {
      return _osfind(0x80, p); // output only
   } else {
      return _osfind(0xc0, p); // update (random access)
   }
}

void osshut(void *chan) { // Close file(s)
   _osfind(0x00, chan);
}

long long geteof(void *chan) {  // Get EOF status
   int x = (int) chan;
   _osbyte(0x7F, &x, NULL, NULL);
   if (x) {
      // EOF
      return -1; // TRUE
   } else {
      // not EOF
      return 0;  // FALSE
   }
}

long long getptr(void *chan) { // Get file pointer
   uint32_t ptr = 0;
   _osargs(0, chan, &ptr);
   return ptr;
}

void setptr(void *chan, long long ptr) { // Set the file pointer
   _osargs(1, chan, &ptr);
}

long long getext(void *chan) {  // Get file length
   uint32_t ext = 0;
   _osargs(2, chan, &ext);
   return ext;
}

unsigned char osbget(void *chan, int *peof) { // Read a byte from a file
   return _osbget(chan, peof);
}

void osbput(void *chan, unsigned char byte) { // Write a byte to a file
   _osbput(chan, byte);
}

// MOS - Graphics

unsigned int palette[256];

void getcsr(int *px, int *py) { // Get text cursor (caret) coords
   _osbyte(0x86, px, py, NULL);
}

int vgetc(int x, int y) { // Get character at specified coords
   text("TODO: vgetx");
   crlf();
   return 0;
}

int vpoint(int x, int y) { // Get palette index or -1
   if (x < -32768 || x > 32767 || y < -32768 || y > 32767) {
      return -1;
   } else {
      int16_t block[3];
      block[0] = (int16_t) x;
      block[1] = (int16_t) y;
      block[2] = 0;
      _osword(9, block);
      return block[2];
   }
}

int vtint(int x, int y) { // Get RGB pixel colour or -1
   text("TODO: vyint");
   crlf();
   return 0;
}

int widths(unsigned char *s, int l) { // Get string width in graphics units
   text("TODO: widths");
   crlf();
   return 0;
}

// MOS - Mouse

void mouse(int *px, int *py, int *pb) { // Get mouse state
   text("TODO: mouse");
   crlf();
}

void mouseon(int type) { // Set mouse cursor (pointer)
   text("TODO: mouseon");
   crlf();
}

void mouseoff(void) { // Hide mouse cursor
   text("TODO: mouseoff");
   crlf();
}

void mouseto(int x, int y) { // Move mouse cursor
   text("TODO: mouseto");
   crlf();
}


// MOS - Sound

void quiet(void) {     // Disable sound generation
   text("TODO: quiet");
   crlf();
}

void sound(short chan, signed char ampl, unsigned char pitch, unsigned char duration) { // Synthesise sound waveform
   uint16_t block[4];
   block[0] = chan;
   block[1] = ampl;
   block[2] = pitch;
   block[3] = duration;
   _osword(7, &block);
}

void envel(signed char *env) {  // ENVELOPE statement
   _osword(8, env);
}

// MOS - Time

void putime(int n) {    // Store centisecond ticks
   uint32_t block[2];
   block[0] = n;
   block[1] = 0;
   _osword(2, &block);
}

int getime(void) {     // Return centisecond count
   uint32_t block[2];
   _osword(1, &block);
   return block[0];
}

void oswait(int cs) { // Pause for a specified time
   int time = getime();
   while (getime() < time + cs) {
      trap();
   }
}


int getims(void) {     // Get clock time string to accs
   accs[0] = 0;
   _osword(14, accs);
   if (accs[0]) {
      accs[24] = 13;
      return 24;
   } else {
      strcpy(accs, "Not Implemented\n");
      return 15;
   }
}

// MOS - Misc

int adval(int n) {    // ADVAL function
   int x = n;
   _osbyte(128, &x, NULL, NULL);
   return x;
}

void faterr(const char *msg) {  // Report a 'fatal' error message
   //reset ();
   //syserror (msg);
   //error (256, "");
   error(512, "");
}


size_t guicall(void *, PARM *) {   // Call function in GUI thread context
   text("TODO: guicall");
   crlf();
   return 0;
}

int putevt(heapptr handler, int msg, int wparam, int lparam) { // Store event in queue
   text("TODO: putevt");
   crlf();
   return 0;
}

void reset(void) {     // Prepare for reporting an error
}

void *sysadr(char *) { // Get the address of an API function
   text("TODO: sysadr");
   crlf();
   return NULL;
}

// Check for Escape (if enabled) and kill:
void trap(void) { // Test for ESCape
   if (flags & ESCFLG) {
      flags &= ~ESCFLG;  // Clear Basic's escape flag
      _osbyte(0x7e, NULL, NULL, NULL); // Acknowledge ESCape
      error (17, NULL);  // 'Escape'
   }
}


// Test for escape, kill, pause, single-step, flash and alert:
heapptr xtrap(void) { // Handle an event interrupt etc.
   trap();
   return 0;
}
