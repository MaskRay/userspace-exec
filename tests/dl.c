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
  //void (*myexec)(void*, long) = (void(*)(void*,long))((char*)exec_base+0x504);

  void *dl = dlopen(argv[1], RTLD_NOW);
  if (! dl)
    return printf("dlopen: %s\n", dlerror()), 1;
  void (*myexec)(void*, long) = dlsym(dl, "myexec");
  if (! myexec)
    return printf("dlsym: %s\n", dlerror()), 1;
  //dlsym(dl, "myexec");

  struct stat sb2;
  int fd2 = open(argv[2], O_RDONLY);
  fstat(fd2, &sb2);
  void *elf = mmap(NULL, sb2.st_size, PROT_READ, MAP_SHARED, fd2, 0);

  myexec(elf, sb2.st_size);
}
