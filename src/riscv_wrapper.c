#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "BBC.h"
#include "version.h"

// From bbccon.h
extern char *accs;              // String accumulator
extern char *buff;              // Temporary string buffer
//extern char* path;              // File path buffer
#define PAGE_OFFSET ACCSLEN + 0xc00     // Offset of PAGE from memory base
#undef MAXIMUM_RAM
#define MAXIMUM_RAM 0x800000    // Maximum amount of RAM to allocate (currently 8MB)

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
int MaximumRAM = MAXIMUM_RAM;
//timer_t UserTimerID;
unsigned int palette[256];


// Declared in bbmain.c:
void error(int, const char *);
void text(const char *);       // Output NUL-terminated string
void crlf(void);               // Output a newline
int basic(void *ecx, void *edx, void *prompt);

// Declared in bbeval.c:
unsigned int rnd(void);        // Return a pseudo-random number

typedef void (*handler_fn_type)(int a0);

extern void error_handler();
extern void escape_handler();

// Forward Declarations
unsigned long _osfile(int reason, char *filename, void *block);
int osargs(int reason, void *chan, void *data);
void oscli(char *cmd);
void oswrch(unsigned char vdu);
void osbput(void *chan, unsigned char byte);
unsigned char osbget(void *chan, int *peof);
int osgbpb(int reason, void *block);

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

   userRAM = (void *) 0;
   progRAM = userRAM + PAGE_OFFSET; // Will be raised if @cmd$ exceeds 255 bytes
   userTOP = (void *) userRAM + MaximumRAM;
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

static char *__heap_start = (char *) 0xf80000;
static char *__heap_end = (char *) 0xff0000;
static char *brk = (char *) 0xf80000;

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

// MOS Interface

// returns PPYYXXAA, as expected by USR

int oscall(int addr) {    // Call an emulated OS function
   char tmp[256];
   uint32_t a = stavar[1];
   uint32_t x = stavar[24];
   uint32_t y = stavar[25];
   uint32_t ret;
   int c = 0;
   sprintf(tmp, "oscall(0x%04x a=%08lx x=%08lx y=%08lx\r\n", addr, a, x, y);
   text(tmp);
   switch (addr) {
   case 0xFFD1:
      {
      c = osgbpb(a, (uint32_t *)x);
      break;
   }
   case 0xFFDA:
      ret = osargs(a, (void *)y, (void *)x);
      if (a == 0) {
         a = ret & 0xff;
      }
      break;
   case 0xFFDD:
      {
         // On 8-bit MOS, filename is only a two byte thing, so the
         // best we can do is to remap this assuming the string is
         // likely to be in the 0x0001xxxx address block. It might
         // be better to base this on LOMEM.
         char *filename = (char *)(uint32_t)(0x10000 + *(uint16_t *)x);
         int ret = _osfile(a, filename, (void *)x + 2);
         a = ret & 0xff;
         break;
      }
   case 0xFFEE:
      oswrch(a & 0xff);
      break;
   case 0xFFF7:
      oscli((void *) x);
      break;
   case 0xFFCE: text("TODO: emulated osfind() call"); crlf(); break;
   case 0xFFD4: text("TODO: emulated osbput() call"); crlf(); break;
   case 0xFFD7: text("TODO: emulated osbget() call"); crlf(); break;
   case 0xFFE0: text("TODO: emulated osrdch() call"); crlf(); break;
   case 0xFFE3: text("TODO: emulated osasci() call"); crlf(); break;
   case 0xFFE7: text("TODO: emulated osnewl() call"); crlf(); break;
   case 0xFFEC: text("TODO: emulated oswrcr() call"); crlf(); break;
   case 0xFFF1: text("TODO: emulated osword() call"); crlf(); break;
   case 0xFFF4: text("TODO: emulated osbyte() call"); crlf(); break;
   default:
      text("TODO: unexpected emulated oscall address");
      crlf();
   }
   return (c << 24) + ((y & 0xff) << 16) + ((x & 0xff) << 8) + (a & 0xff);
}

void oscli(char *cmd) {      // Execute an emulated OS command
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

int oskey(int wait) {     // Wait for character or test key
   text("TODO: oskey");
   crlf();
   return 0;
}

void oswrch(unsigned char vdu) {   // Write to display or other output stream (VDU)
   register int a0 asm ("a0") = vdu;
   register int a7 asm ("a7") = 4;
   asm volatile ("ecall"
                 : // outputs

                 : // inputs
                   "r"  (a0),
                   "r"  (a7)
                 );
}

unsigned char osrdch(void) { // Get character from console input
   register int a0 asm ("a0") ;
   register int a7 asm ("a7") = 6;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a0)
                 : // inputs
                   "r"  (a7)
                 );
   return (unsigned char)(a0 & 0xff);
}


void osline(char *buffer) {     // Get a line of console input
   unsigned int block[4];
   block[0] = (unsigned int) buffer;
   block[1] = 0xff;
   block[2] = 32;
   block[3] = 255;
   register int a0 asm ("a0") = 0;
   register unsigned int *a1 asm ("a1") = block;
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
   // If escape, make sure the line has a terminator
   if (a2 < 0) {
      *buffer = 0x0D;
   }
}

int osbyte(int al, int xy) {
   register int a0 asm ("a0") = al;
   register int a1 asm ("a1") = xy & 0xFF;
   register int a2 asm ("a2") = (xy >> 8) & 0xFF;
   register int a7 asm ("a7") = 2;
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
   // Return 00 YY XX AA in an integer
   return a0 + ((a1 & 0xff) << 8) + ((a2 & 0xff) << 16);
}

void osword(int al, void *xy) {
   register int a0 asm ("a0") = al;
   register int *a1 asm ("a1") = xy;
   register int a7 asm ("a7") = 3;
   asm volatile ("ecall"
                 : // outputs
                   "+r" (a1)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a7)
                 : // clobber
                   "memory"
                 );
}

void oswait(int cs) { // Pause for a specified time
   text("TODO: oswait");
   crlf();
}

// MOS - File

unsigned long _osfile(int reason, char *filename, void *block) {
   register int   a0 asm ("a0") = reason;
   register void *a1 asm ("a1") = filename;
   register void *a2 asm ("a2") = block;
   register int   a7 asm ("a7") = 7;
   asm volatile ("ecall"
                 : // outputs
                   "+r"  (a0)
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a2),
                   "r"  (a7)
                 : // clobber
                   "memory"
                 );
   return a0;
}

void osfile(int reason, char *filename, void *load, void *exec, void *start, void *end) {
   void *block[4] = {load, exec, start, end};
   _osfile(reason, filename, block);
}

void osload(char *p, void *addr, int max) { // Load a file to memory
   osfile(255, p, addr, NULL, NULL, NULL);
}

void ossave(char *p, void *addr, int len) { // Save a file from memory
   osfile(0, p, NULL, NULL, addr, addr + len);
}

void *osopen(int type, char *p) { // Open a file
   register int   a0 asm ("a0");
   register void *a1 asm ("a1") = p;
   register int   a7 asm ("a7") = 12;
	if (type == 0) {
      a0 = 0x40; // input only
   } else if (type == 1) {
      a0 = 0x80; // output only
   } else {
      a0 = 0xc0; // update (random access)
   }
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

void osshut(void *chan) { // Close file(s)
   register int   a0 asm ("a0") = 0;
   register void *a1 asm ("a1") = chan;
   register int   a7 asm ("a7") = 12;
   asm volatile ("ecall"
                 : // outputs
                 : // inputs
                   "r"  (a0),
                   "r"  (a1),
                   "r"  (a7)
                 );
}

long long geteof(void *chan) {  // Get EOF status
   text("TODO: geteof");
   crlf();
   return 0;
}

int osargs(int reason, void *chan, void *data) {
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

int osgbpb(int reason, void *block) {
   register int   a0 asm ("a0") = reason;
   register void *a1 asm ("a1") = block;
   register int   a2 asm ("a2") = 0;
   register int   a7 asm ("a7") = 11;
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
   return a2 & 1;
}

long long getptr(void *chan) { // Get file pointer
   uint32_t ptr = 0;
   osargs(0, chan, &ptr);
   return ptr;
}

void setptr(void *chan, long long ptr) { // Set the file pointer
   osargs(1, chan, &ptr);
}

long long getext(void *chan) {  // Get file length
   uint32_t ext = 0;
   osargs(2, chan, &ext);
   return ext;
}


unsigned char osbget(void *chan, int *peof) { // Read a byte from a file
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
         *peof = -1;
      } else {
         *peof = 0;
      }
   }
   return (unsigned char) a0;
}

void osbput(void *chan, unsigned char byte) { // Write a byte to a file
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


// MOS - Graphics

unsigned int palette[256];

void getcsr(int *px, int *py) { // Get text cursor (caret) coords
   int yyxxaa = osbyte(0x86, 0);
   *px = (yyxxaa >>  8) & 0xff;
   *py = (yyxxaa >> 16) & 0xff;
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
      osword(9, block);
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
   osword(7, &block);
}

void envel(signed char *env) {  // ENVELOPE statement
   osword(8, env);
}

// MOS - Time

void putime(int n) {    // Store centisecond ticks
   uint32_t block[2];
   block[0] = n;
   block[1] = 0;
   osword(2, &block);
}

int getime(void) {     // Return centisecond count
   uint32_t block[2];
   osword(1, &block);
   return block[0];
}

int getims(void) {     // Get clock time string to accs
   text("TODO: getims");
   crlf();
   return 0;
}

// MOS - Misc

int adval(int n) {    // ADVAL function
   int yyxxaa = osbyte(128, n);
   return yyxxaa >> 8;
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
      osbyte(0x7e, 0);   // Acknowledge ESCape
      error (17, NULL) ; // 'Escape'
   }
}


// Test for escape, kill, pause, single-step, flash and alert:
heapptr xtrap(void) { // Handle an event interrupt etc.
   trap();
   return 0;
}
