#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SEC_RANGE 1024
#define ALIGN 128

char *allocate_shared_buffer();
void deallocate_shared_buffer(char *buf);

#endif // UTIL_H_
