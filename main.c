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

#include "syscall.h"

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
# define LDSO_BASE 0x6666666000l
#elif defined(__arm__)
# define LDSO_BASE 0x200000
#endif

///// main

SHELLCODE INLINE void *map_file(const char *file)
{
  int fd = open(file, O_RDONLY);
  off_t len = lseek(fd, 0, SEEK_END);
  void *ret = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
  DP("ret %p", ret);
  close(fd);
  return ret;
}

SHELLCODE INLINE void *my_memcpy(void *dst, const void *src, size_t n)
{
  char *x = dst;
  const char *y = src;
  for (; n; n--)
    *x++ = *y++;
  return dst;
}

SHELLCODE static void *load_elf(void *elf, Elf_Ehdr **elf_ehdr, Elf_Ehdr **interp_ehdr)
{
  DP("entry");
  Elf_Ehdr *ehdr = (Elf_Ehdr*)elf;
  bool is_pic = ehdr->e_type == ET_DYN;
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
      //DP("mmap(%#lx,%#lx) = %#lx", specified, rounded_len, addr);
      my_memcpy((char*)(is_pic ? addr+phdr->p_vaddr%PAGESZ : phdr->p_vaddr), (const char*)ehdr+phdr->p_offset, phdr->p_filesz);
      mprotect((void*)addr, rounded_len, (phdr->p_flags & PF_R ? PROT_READ : 0) |
                                  (phdr->p_flags & PF_W ? PROT_WRITE : 0) |
                                  (phdr->p_flags & PF_X ? PROT_EXEC : 0));

      DP("mmap_addr %p", addr);
      if (! first_addr) {
        if (elf_ehdr)
          *elf_ehdr = (Elf_Ehdr*)addr;
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

SHELLCODE void myexec(void *elf, long len)
{
  Elf_Ehdr *elf_ehdr, *interp_ehdr;
  void *entry = load_elf(elf, &elf_ehdr, &interp_ehdr);

  volatile long stack[999], *p = stack;
  *p++ = 1; // argc
  volatile long *argv = p; // argv[]
  *p++ = (long)stack;
  *p++ = 0;
  volatile long *envp = p; // envp[]
  //*p++ = "LD_SHOW_AUXV=1";
  *p++ = 0;
  volatile long *auxv = p; // auxv[]
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
  *p++ = (long)stack;
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

long _atol(const char *x)
{
  long r = 0;
  while (*x != ' ') r = r*10+*x++-'0';
  return r;
}

void myexec2(int argc, void *elf, long len)
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
  asm("mov sp, %0\n\t"
      "bx %1\n\t"
      :: "r"(p), "r"(entry));
#else
# error "this arch is not supported"
#endif
}

int main(int argc, char *argv[], char *envp[])
{
  struct stat sb = {};
  int fd = open(argv[1], O_RDONLY);
  DP("fd: %d\n", fd);
  off_t len = lseek(fd, 0, SEEK_END);
  DP("size of argv[1]: %lld\n", (long long)len);
  void *elf = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
  DP("elf %p", elf);
  close(fd);

  //myexec2(argc, elf, sb.st_size);
  myexec(elf, sb.st_size);
}
