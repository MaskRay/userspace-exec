#define _GNU_SOURCE
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sysexits.h>
#include <unistd.h>

void print_usage(FILE *fh)
{
  fprintf(stderr, "usage: %s shared.so elf\n", program_invocation_name);
  exit(fh == stdout ? 0 : EX_USAGE);
}

int main(int argc, char *argv[], char *envp[])
{
  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1)
    switch (opt) {
    case 'h':
      print_usage(stdout);
      break;
    }
  if (optind+2 != argc)
    print_usage(stderr);

  // load shared.so
  struct stat sb, sb2;
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0)
    err(EX_IOERR, "open %s", argv[optind]);
  if (fstat(fd, &sb) < 0)
    err(EX_IOERR, "fstat %s", argv[optind]);
  void *handle = dlopen(argv[optind], RTLD_NOW);
  if (! handle)
    errx(EX_DATAERR, "dlopen: %s\n", dlerror());
  void (*myexec)(void*, long) = dlsym(handle, "myexec");
  if (! myexec)
    errx(EX_UNAVAILABLE, "dlsym: %s\n", dlerror());

  // load ELF
  int fd2 = open(argv[2], O_RDONLY);
  if (fd2 < 0)
    err(EX_IOERR, "open %s", argv[optind+1]);
  if (fstat(fd2, &sb2) < 0)
    err(EX_IOERR, "fstat %s", argv[optind+1]);
  void *elf = mmap(NULL, sb2.st_size, PROT_READ, MAP_SHARED, fd2, 0);
  if (elf == MAP_FAILED)
    err(EX_OSERR, "mmap %s", argv[optind+1]);

  // userspace exec
  myexec(elf, sb2.st_size);

  // should not reach here
  return EX_UNAVAILABLE;
}
