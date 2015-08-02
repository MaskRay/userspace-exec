#include <fcntl.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
  struct stat sb;
  int fd = open(argv[1], O_RDONLY);
  fstat(fd, &sb);
  void *exec_base = mmap((void*)0x1000000, sb.st_size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED | MAP_PRIVATE, fd, 0);
  void (*myexec)(void*, long) = (void(*)(void*,long))((char*)exec_base+0x545);

  struct stat sb2;
  int fd2 = open(argv[2], O_RDONLY);
  fstat(fd2, &sb2);
  void *elf = mmap(NULL, sb2.st_size, PROT_READ, MAP_SHARED, fd2, 0);

  myexec(elf, sb2.st_size);
}
