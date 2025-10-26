/*#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// entry.S jumps here in machine mode on stack0.
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  w_mepc((uint64)main);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// ask each hart to generate timer interrupts.
void
timerinit()
{
  // enable supervisor-mode timer interrupts.
  w_mie(r_mie() | MIE_STIE);
  
  // enable the sstc extension (i.e. stimecmp).
  w_menvcfg(r_menvcfg() | (1L << 63)); 
  
  // allow supervisor to use stimecmp and time.
  w_mcounteren(r_mcounteren() | 2);
  
  // ask for the very first timer interrupt.
  w_stimecmp(r_time() + 1000000);
}

*/












//assignment 2 q1
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "string.h"

// global variable to store total physical memory
uint64 totalmem = 128*1024*1024; // default 128 MB

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// Basic FDT header structure (see RISC-V spec)
struct fdt_header {
  uint32 magic;
  uint32 totalsize;
  uint32 off_dt_struct;
  uint32 off_dt_strings;
  uint32 off_mem_rsvmap;
  uint32 version;
  uint32 last_comp_version;
  uint32 boot_cpuid_phys;
  uint32 size_dt_strings;
  uint32 size_dt_struct;
};

#define FDT_MAGIC 0xd00dfeed
#define FDT_BEGIN_NODE  1
#define FDT_END_NODE    2
#define FDT_PROP        3
#define FDT_NOP         4
#define FDT_END         9

static inline uint32 fdt32_to_cpu(uint32 x) {
  return ((x & 0xff000000) >> 24) |
         ((x & 0x00ff0000) >> 8)  |
         ((x & 0x0000ff00) << 8)  |
         ((x & 0x000000ff) << 24);
}

// Minimal strcmp for early boot (we can't rely on full kernel lib yet)
static int early_strcmp(const char *p, const char *q) {
  while (*p && *p == *q)
    p++, q++;
  return (unsigned char)*p - (unsigned char)*q;
}

// Parse the flattened device tree to find "memory" node -> "reg" property
void parse_fdt(void *fdt) {
  struct fdt_header *hdr = (struct fdt_header*) fdt;
  if (fdt32_to_cpu(hdr->magic) != FDT_MAGIC) {
    printf("bad FDT magic\n");
    return;
  }

  uint32 struct_off = fdt32_to_cpu(hdr->off_dt_struct);
  uint32 strings_off = fdt32_to_cpu(hdr->off_dt_strings);
  uint32 *p = (uint32*)((char*)fdt + struct_off);

  char *strtab = (char*)fdt + strings_off;
  char nodename[64];
  int depth = 0;

  while (1) {
    uint32 token = fdt32_to_cpu(*p++);
    if (token == FDT_BEGIN_NODE) {
      char *name = (char*)p;
      strncpy(nodename, name, sizeof(nodename)-1);
      while (*p++) ; // skip name + null terminator
      depth++;
    } else if (token == FDT_END_NODE) {
      depth--;
    } else if (token == FDT_PROP) {
      uint32 len = fdt32_to_cpu(*p++);
      uint32 nameoff = fdt32_to_cpu(*p++);
      char *propname = strtab + nameoff;
      uint8 *val = (uint8*)p;
      if (!early_strcmp(propname, "reg") && !early_strcmp(nodename, "memory")) {
        // "reg" contains base address and size, both 64-bit
        uint64 base = ((uint64)fdt32_to_cpu(*(uint32*)(val)) << 32) |
                      fdt32_to_cpu(*(uint32*)(val+4));
        uint64 size = ((uint64)fdt32_to_cpu(*(uint32*)(val+8)) << 32) |
                      fdt32_to_cpu(*(uint32*)(val+12));
        totalmem = size;
//        printf("Memory base 0x%p, size %p bytes\n", base, size);
	printf("Memory base 0x%lx, size %ld bytes\n", base, size);
      }
      p += (len + 3) / 4;
    } else if (token == FDT_END) {
      break;
    } else if (token == FDT_NOP) {
      continue;
    } else {
      break;
    }
  }
}

// entry.S jumps here in machine mode on stack0.
void
start()
{
  void *fdt;
  asm volatile("mv %0, a1" : "=r"(fdt));
  parse_fdt(fdt);

  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  w_mepc((uint64)main);

  // disable paging for now.
  w_satp(0);

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE);

  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  timerinit();

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}

// ask each hart to generate timer interrupts.
void
timerinit()
{
  w_mie(r_mie() | MIE_STIE);
  w_menvcfg(r_menvcfg() | (1L << 63));
  w_mcounteren(r_mcounteren() | 2);
  w_stimecmp(r_time() + 1000000);
}

