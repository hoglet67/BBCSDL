#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "BBC.h"

#define YEAR    "2023"          // Copyright year
#define VERSION "1.35a"         // Version string

// From bbccon.h
#define MAX_PORTS       4       // Maximum number of port channels
#define MAX_FILES       8       // Maximum number of file channels
extern char *accs;              // String accumulator
extern char *buff;              // Temporary string buffer
extern char* path;              // File path buffer
extern char **keystr;           // Pointers to user *KEY strings
extern char* keybdq;            // Keyboard queue (indirect)
extern int* eventq;             // Event queue (indirect)
extern void* filbuf[];
extern unsigned char farray;    // @hfile%() number of dimensions
extern unsigned int fasize;     // @hfile%() number of elements
extern RND prand;               // Pseudo-random number
extern FILE *exchan;            // EXEC channel
extern FILE *spchan;            // SPOOL channel
#define PAGE_OFFSET ACCSLEN + 0x2000     // Offset of PAGE from memory base
#define MAXIMUM_RAM 0x800000    // Maximum amount of RAM to allocate (currently 8MB)
extern unsigned int platform;   // OS platform

// Global variables (external linkage):
void *userRAM = NULL;
void *progRAM = NULL;
void *userTOP = NULL;
const char szVersion[] = "BBC BASIC for RISC-V Console "VERSION;
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

extern void escape_handler();

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

   platform = 0;

   void* immediate = (void *) 1;

   userRAM = (void *) 0x100000;
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
   path = (char*) buff + 0x100;            // File path
   keystr = (char**)(path + 0x100);       // *KEY strings
   keybdq = (char*) keystr + 0x100;        // Keyboard queue
   eventq = (void*) keybdq + 0x100;        // Event queue
   filbuf[0] = (eventq + 0x200 / 4);       // File buffers n.b. pointer arithmetic!!

   farray = 1;                             // @hfile%() number of dimensions
   fasize = MAX_PORTS + MAX_FILES + 4;     // @hfile%() number of elements


   prand.l = 12345678;                    /// Seed PRNG
   prand.h = (prand.l == 0);
   rnd();                                 // Randomise !

   memset(keystr, 0, 256);
   spchan = NULL;
   exchan = NULL;

   if (immediate == (void *) 1)
      {
         text(szVersion);
         crlf();
         text(szNotice);
         crlf();
      }

   // Install new escape handler
   void *old_handler;

   oshandlers(0xfffe, escape_handler, 0, &old_handler, NULL);

   int ret = basic(progRAM, userTOP, immediate);

   // Restore old escape handler
   oshandlers(0xfffe, old_handler, 0, NULL, NULL);

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

int oscall(int addr) {    // Call an emulated OS function
   text("TODO: oscall");
   crlf();
   return 0;
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

                 : // inptputs
                   "r"  (a0),
                   "r"  (a7)
                 );
}

unsigned char osrdch(void) { // Get character from console input
   text("TODO: oscrdch");
   crlf();
   return 0;
}


void osline(char *buffer) {     // Get a line of console input
   unsigned int block[4];
   block[0] = (unsigned int) buffer;
   block[1] = 0xff;
   block[2] = 32;
   block[3] = 255;
   register int a0 asm ("a0") = 0;
   register unsigned int *a1 asm ("a1") = block;
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


void osload(char *p, void *addr, int max) { // Load a file to memory
   text("TODO: osload");
   crlf();
}

void ossave(char *p, void *addr, int len) { // Save a file from memory
   text("TODO: ossave");
   crlf();
}

void *osopen(int type, char *p) { // Open a file
   text("TODO: osopen");
   crlf();
   return NULL;
}

void osshut(void *chan) { // Close file(s)
   text("TODO: osshut");
   crlf();
}

long long geteof(void *chan) {  // Get EOF status
   text("TODO: geteof");
   crlf();
   return 0;
}

long long getext(void *chan) {  // Get file length
   text("TODO: getext");
   crlf();
   return 0;
}

void setptr(void *chan, long long ptr) { // Set the file pointer
   text("TODO: setptr");
   crlf();
}

long long getptr(void *chan) { // Get file pointer
   text("TODO: getptr");
   crlf();
   return 0;
}

unsigned char osbget(void *chan, int *peof) { // Read a byte from a file
   text("TODO: osbget");
   crlf();
   return 0;
}

void osbput(void *chan, unsigned char byte) { // Write a byte to a file
   text("TODO: osbput");
   crlf();
}


// MOS - Graphics

unsigned int palette[256];

void getcsr(int *px, int *py) { // Get text cursor (caret) coords
   text("TODO: getcsr");
   crlf();

}

int vgetc(int x, int y) { // Get character at specified coords
   text("TODO: vgetx");
   crlf();
   return 0;
}

int vpoint(int x, int y) { // Get palette index or -1
   text("TODO: vpoint");
   crlf();
   return 0;
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

   text("TODO: sount");
   crlf();
}

void envel(signed char *env) {  // ENVELOPE statement
   text("TODO: eval");
   crlf();
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
   text("TODO: adval");
   crlf();
   return -1;
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
