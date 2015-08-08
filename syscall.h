#pragma once
#include "common.h"

SHELLCODE INLINE long syscall1(long number, long a0)
{
#ifdef __i386
  long ret;
  asm("int $0x80\n\t"
      : "=a"(ret) : "a"(number),"b"(a0) : "memory","cc");
#elif defined(__x86_64)
  long ret;
  asm("syscall\n\t"
      : "=a"(ret) : "a"(number),"D"(a0) : "rcx","r11","memory","cc");
#elif defined(__arm__)
  register long ret asm("r0");
  register long number_ asm("r7") = number;
  register long b0 asm("r0") = b0;
  asm("swi 0\n\t" : "=r"(ret) : "r"(number_),"r"(b0): "memory","cc");
#endif
  return ret;
}

SHELLCODE INLINE long syscall2(long number, long a0, long a1)
{
#ifdef __i386
  long ret;
  asm("int $0x80\n\t"
      : "=a"(ret) : "a"(number),"b"(a0),"c"(a1) : "memory","cc");
#elif defined(__x86_64)
  long ret;
  asm("syscall\n\t"
      : "=a"(ret) : "a"(number),"D"(a0),"S"(a1) : "rcx","r11","memory","cc");
#elif defined(__arm__)
  register long ret asm("r0");
  register long number_ asm("r7") = number;
  register long b0 asm("r0") = a0;
  register long b1 asm("r1") = a1;
  asm("swi 0\n\t" : "=r"(ret) : "r"(number_),"r"(b0),"r"(b1) : "memory","cc");
#endif
  return ret;
}

SHELLCODE INLINE long syscall3(long number, long a0, long a1, long a2)
{
#ifdef __i386
  long ret;
  asm("int $0x80\n\t"
      : "=a"(ret) : "a"(number),"b"(a0),"c"(a1),"d"(a2) : "memory","cc");
#elif defined(__x86_64)
  long ret;
  asm("syscall\n\t"
      : "=a"(ret) : "a"(number),"D"(a0),"S"(a1),"d"(a2) : "rcx","r11","memory","cc");
#elif defined(__arm__)
  register long ret asm("r0");
  register long number_ asm("r7") = number;
  register long b0 asm("r0") = a0;
  register long b1 asm("r1") = a1;
  register long b2 asm("r2") = a2;
  asm("swi 0\n\t" : "=r"(ret) : "r"(number_),"r"(b0),"r"(b1),"r"(b2) : "memory","cc");
#endif
  return ret;
}

SHELLCODE INLINE long syscall6(long number, long a0, long a1, long a2, long a3, long a4, long a5)
{
#ifdef __i386
  long ret;
  asm(
      "movl %%ebp, -4(%%esp)\n\t"
      //"movl %1, %%eax\n\t"
      "movl %2, %%ebx\n\t"
      "movl %3, %%ecx\n\t"
      "movl %4, %%edx\n\t"
      "movl %5, %%esi\n\t"
      "movl %6, %%edi\n\t"
      "movl %7, %%ebp\n\t"
      "int $0x80\n\t"
      "movl -4(%%esp), %%ebp\n\t"
      : "=a"(ret) : "a"(number),"g"(a0),"g"(a1),"g"(a2),"g"(a3),"g"(a4),"g"(a5) : "ebx","ecx","edx","esi","edi","memory","cc"
     );
#elif defined(__x86_64)
  long ret;
  register long b3 asm("r10") = a3;
  register long b4 asm("r8") = a4;
  register long b5 asm("r9") = a5;
  asm ("syscall\n\t"
      : "=a"(ret) : "a"(number),"D"(a0),"S"(a1),"d"(a2),"r"(b3),"r"(b4),"r"(b5) : "rcx","r11","memory","cc");
#else
  register long ret asm("r0");
  register long number_ asm("r7") = number;
  register long b0 asm("r0") = a0;
  register long b1 asm("r1") = a1;
  register long b2 asm("r2") = a2;
  register long b3 asm("r3") = a3;
  register long b4 asm("r4") = a4;
  register long b5 asm("r5") = a5;
  asm("swi 0\n\t"
      : "=r"(ret) : "r"(number_),"r"(b0),"r"(b1),"r"(b2),"r"(b3),"r"(b4),"r"(b5) : "memory","cc");
#endif
  return ret;
}

#define brk my_brk
SHELLCODE INLINE void *my_brk(void *addr)
{
  return (void*)syscall1(__NR_brk, (long)addr);
}

#define close my_close
SHELLCODE INLINE int my_close(int fd)
{
  return syscall1(__NR_close, fd);
}

#define lseek my_lseek
SHELLCODE INLINE int my_lseek(int fd, off_t offset, int whence)
{
  return syscall3(__NR_lseek, fd, offset, whence);
}

#define mmap my_mmap
SHELLCODE INLINE void *my_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
#if defined(__i386) || defined(__arm__)
  return (void*)syscall6(__NR_mmap2, (long)addr, length, prot, flags, fd, offset/PAGESZ);
#else
  return (void*)syscall6(__NR_mmap, (long)addr, length, prot, flags, fd, offset);
#endif
}

#define mprotect my_mprotect
SHELLCODE INLINE int my_mprotect(void *addr, size_t len, int prot)
{
  return syscall3(__NR_mprotect, (long)addr, len, prot);
}

#define munmap my_munmap
SHELLCODE INLINE int munmap(void *addr, size_t len)
{
  return syscall2(__NR_munmap, (long)addr, len);
}

#define open(pathname, flags) \
  syscall2(__NR_open, (long)pathname, flags)
