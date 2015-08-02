#define _FILE_OFFSET_BITS 64
#include <asm/unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <elf.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#define REP(i, n) FOR(i, 0, n)
#define FOR(i, a, b) for (int i = (a); i < (b); i++)

#define PAGESZ 0x1000

typedef unsigned long ul;
#define ALIGN_DOWN(x) ((x) & -PAGESZ)
#define ALIGN_UP(x) ((x)+PAGESZ-1 & -PAGESZ)
//#define INLINE static inline __attribute__((always_inline))
#define INLINE static inline

#if __WORDSIZE == 64
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
# define Elf_auxv_t Elf64_auxv_t
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
# define Elf_auxv_t Elf32_auxv_t
#endif

// should not intersect with normal mapping addresses
#ifdef __i386
# define LDSO_BASE 0x20000
#elif defined(__x86_64)
# define LDSO_BASE 0x20000
#elif defined(__arm__)
# define LDSO_BASE 0x200000
#endif

#define DEBUG

#ifdef DEBUG
# define DP(fmt, ...) fprintf(stderr, "%s:%d:%s " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
# define DP(...)
#endif

INLINE long syscall_impl(long number, long a0, long a1, long a2, long a3, long a4, long a5)
{
  long ret;
#ifdef __i386
  asm("push %%ebp\n\t"
      "movl %1, %%eax\n\t"
      "movl %2, %%ebx\n\t"
      "movl %3, %%ecx\n\t"
      "movl %4, %%edx\n\t"
      "movl %5, %%esi\n\t"
      "movl %6, %%edi\n\t"
      "movl %7, %%ebp\n\t"
      "int $0x80\n\t"
      "pop %%ebp\n\t"
      : "=&a"(ret) : "g"(number),"g"(a0),"g"(a1),"g"(a2),"g"(a3),"g"(a4),"g"(a5) : "ebx","ecx","edx","esi","edi","memory","cc"
     );
#elif defined(__x86_64)
  asm("movq %1, %%rax\n\t"
      "movq %%rsi, %%rdi\n\t"
      "movq %%rdx, %%rsi\n\t"
      "movq %%rcx, %%rdx\n\t"
      "movq %%r8, %%r10\n\t"
      "movq %%r9, %%r8\n\t"
      "movq %2, %%r9\n\t"
      "syscall\n\t"
      : "=&a"(ret) : "g"(number), "g"(a5) : "rdi","rsi","rdx","r10","r8","r9","memory","cc");
#else
  asm("ldr     r7, %1\n\t"
      "ldr     r0, %2\n\t"
      "ldr     r1, %3\n\t"
      "ldr     r2, %4\n\t"
      "ldr     r3, %5\n\t"
      "ldr     r4, %6\n\t"
      "ldr     r5, %7\n\t"
      "swi     0x0\n\t"
      "mov     %0, r0\n\t"
      : "=&r"(ret) : "g"(number),"g"(a0),"g"(a1),"g"(a2),"g"(a3),"g"(a4),"g"(a5) : "r0","r1","r2","r3","r4","r5","r7","memory","cc"
      );
#endif
  return ret;
}

long syscall(long number, ...) __attribute__((alias("syscall_impl"), always_inline));

#define close my_close
INLINE int my_close(int fd)
{
  return syscall(__NR_close, fd);
}

#define fstat my_fstat
INLINE int my_fstat(int fd, struct stat *buf)
{
#if defined(__i386) || defined(__arm__)
  return syscall(__NR_fstat64, fd, buf);
#else
  return syscall(__NR_fstat, fd, buf);
#endif
}

#define mmap my_mmap
INLINE void *my_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
#if defined(__i386) || defined(__arm__)
  return (void*)syscall(__NR_mmap2, addr, length, prot, flags, fd, offset/PAGESZ);
#else
  return (void*)syscall(__NR_mmap, addr, length, prot, flags, fd, offset);
#endif
}

#define mprotect my_mprotect
INLINE int my_mprotect(void *addr, size_t len, int prot)
{
  return syscall(__NR_mprotect, addr, len, prot);
}

#define munmap my_munmap
INLINE int munmap(void *addr, size_t len)
{
  return syscall(__NR_munmap, addr, len);
}

#define open(pathname, flags, ...) \
  syscall(__NR_open, pathname, flags, ##__VA_ARGS__)

///// main

int f()
{
  REP(i, 10)
    printf("%d\n", i);
  return 7;
}

long _atol(const char *x)
{
  long r = 0;
  while (*x != ' ') r = r*10+*x++-'0';
  return r;
}

void *map_file(const char *file)
{
  int fd = open(file, O_RDONLY);
  struct stat sb;
  fstat(fd, &sb);
  void *ret = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  DP("ret %p", ret);
  close(fd);
  return ret;
}

void *load_elf(void *elf, Elf_Ehdr **elf_ehdr, Elf_Ehdr **interp_ehdr)
{
  DP("entry");
  bool is_pic;
  Elf_Ehdr *ehdr = (Elf_Ehdr*)elf;
  switch (ehdr->e_type) {
  case ET_EXEC:
    is_pic = false;
    break;
  case ET_DYN:
    is_pic = true;
    break;
  default:
    abort();
    break;
  }
  int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS | (is_pic ? 0 : MAP_FIXED);
  Elf_Phdr *phdr = (Elf_Phdr*)((char*)ehdr+ehdr->e_phoff), *interp = NULL;
  ulong entry = ehdr->e_entry, first_addr = 0, first_specified, brk_addr = 0;
  if (interp_ehdr)
    *interp_ehdr = NULL;

  REP(i, ehdr->e_phnum) {
    switch (phdr->p_type) {
    case PT_INTERP:
      interp = phdr;
      break;
    case PT_LOAD: {
      ulong specified;
      if (! is_pic)
        specified = ALIGN_DOWN(phdr->p_vaddr);
      else if (first_addr) {
        specified = ALIGN_DOWN(phdr->p_vaddr-first_specified+first_addr);
        mmap_flags |= MAP_FIXED;
      } else {
        specified = 0;
        // TODO hack
        entry += specified = LDSO_BASE;
        mmap_flags |= MAP_FIXED;
      }
      ulong rounded_len = ALIGN_UP(phdr->p_vaddr%PAGESZ+phdr->p_memsz);
      ulong addr = (ulong)mmap((void*)specified, rounded_len, PROT_WRITE | PROT_EXEC, mmap_flags, -1, 0);
      memcpy((char*)(is_pic ? addr+phdr->p_vaddr%PAGESZ : phdr->p_vaddr), (const char*)ehdr+phdr->p_offset, phdr->p_filesz);
      mprotect((void*)addr, rounded_len, (phdr->p_flags & PF_R ? PROT_READ : 0) |
                                  (phdr->p_flags & PF_W ? PROT_WRITE : 0) |
                                  (phdr->p_flags & PF_X ? PROT_EXEC : 0));

      DP("mmap_addr %p", addr);
      if (! first_addr) {
        if (elf_ehdr)
          *elf_ehdr = addr;
        first_addr = addr;
        if (is_pic) {
          first_specified = phdr->p_vaddr;
          entry += addr-specified;
        }
      }
      ulong t = phdr->p_vaddr%PAGESZ+phdr->p_memsz;
      if (t > brk_addr)
        brk_addr = t;
    }
    }
    phdr++;
  }
  if (interp)
    entry = (long)load_elf(map_file((const char*)ehdr+interp->p_offset), interp_ehdr, NULL);
  if (! is_pic)
    brk((void*)ALIGN_UP(brk_addr));
  return (void*)entry;
}

void myexec2(void *elf, long len)
{
  Elf_Ehdr *elf_ehdr, *interp_ehdr;
  void *entry = load_elf(elf, &elf_ehdr, &interp_ehdr);

  volatile long stack[999], *p = stack;
  *p++ = 1; // argc
  long *argv = p; // argv[]
  *p++ = (long)"/bin/bash";
  *p++ = 0;
  long *envp = p; // envp[]
  //*p++ = "LD_SHOW_AUXV=1";
  *p++ = 0;
  long *auxv = p; // auxv[]
  //*p++ = AT_SYSINFO_EHDR; // AT_SYSINFO_EHDR for x86-64 arm; AT_SYSINFO for x86
  //*p++ = -1;
  *p++ = AT_PHDR;
  *p++ = (long)((char*)elf_ehdr+elf_ehdr->e_phoff);
  *p++ = AT_PHNUM;
  *p++ = elf_ehdr->e_phnum;
  *p++ = AT_BASE;
  *p++ = (long)interp_ehdr;
  *p++ = AT_ENTRY;
  *p++ = elf_ehdr->e_entry;
  *p++ = AT_RANDOM;
  *p++ = "meowmeowmeowmeow";
  *p++ = AT_NULL;
  *p++ = 0;

#ifdef __i386
  asm("movl %0, %%esp\n\t"
      "jmp *%1\n\t"
      :: "g"(stack), "r"(entry));
#elif defined(__x86_64)
  asm("movq %0, %%rsp\n\t"
      "jmp *%1\n\t"
      :: "g"(stack), "r"(entry));
#elif defined(__arm__)
  asm("mov sp, %0\n\t"
      "bx %1\n\t"
      :: "r"(stack), "r"(entry));
#else
# error "this arch is not supported"
#endif
}

void myexec(int argc, void *elf, long len)
{
  char buf[999], *pbuf = buf;
  int fd = open("/proc/self/stat", O_RDONLY);
  read(fd, buf, sizeof buf);
  close(fd);
  REP(i, 47) // man 5 proc , (48) arg_start
    while (*pbuf++ != ' ');
  long *p = (long *)(_atol(pbuf) & -16); // arg_start in /proc/[pid]/stat
  while (p--, p[0] || p[1]);
  // p points at AT_NULL or zero padding
  while (p--, p[-1] || (p[0] != AT_SYSINFO && p[0] != AT_SYSINFO_EHDR));
  // p points at auxv start
  Elf_auxv_t *auxv = (Elf_auxv_t*)p;
  while ((--p)[-1]);
  // p points at envp start
  p -= argc+2;
  // p points at argc
  *p = argc; // TODO *p will be modified by unknown code in ARM

  Elf_Ehdr *elf_ehdr, *interp_ehdr;
  void *entry = load_elf(elf, &elf_ehdr, &interp_ehdr);
  munmap(elf, len);
  for (; auxv->a_type; auxv++)
    switch (auxv->a_type) {
    case AT_PHDR:
      auxv->a_un.a_val = (long)((char*)elf_ehdr+elf_ehdr->e_phoff);
      break;
    case AT_PHNUM:
      auxv->a_un.a_val = elf_ehdr->e_phnum;
      break;
    case AT_BASE:
      auxv->a_un.a_val = (long)interp_ehdr;
      break;
    case AT_ENTRY:
      auxv->a_un.a_val = elf_ehdr->e_entry;
      break;

//    case AT_NULL	:
//
//case AT_RANDOM:
//      auxv->a_un.a_val = "meowmeowmeowmeow";
//      break;
//    default:
//      auxv->a_type = AT_UID;
//      auxv->a_un.a_val = 1000;
//      break;
    }
#ifdef __i386
  asm("movl %0, %%esp\n\t"
      "jmp *%1\n\t"
      :: "g"(p), "r"(entry));
#elif defined(__x86_64)
  asm("movq %0, %%rsp\n\t"
      "jmp *%1\n\t"
      :: "g"(p), "r"(entry));
#elif defined(__arm__)
  asm("ldr sp, %0\n\t"
      "bx %1\n\t"
      :: "g"(p), "r"(entry));
#else
# error "this arch is not supported"
#endif
}

int main(int argc, char *argv[], char *envp[])
{
  struct stat sb = {};
  int fd = open(argv[1], O_RDONLY);
  int g = fstat(fd, &sb);
  DP("size of argv[1]: %lld\n", (long long)sb.st_size);
  void *elf = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  //myexec(argc, elf, sb.st_size);
  myexec2(elf, sb.st_size);
}
