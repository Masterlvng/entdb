#ifndef ENTDB_UTIL_H
#define ENTDB_UTIL_H

#include <stdint.h>

#define index_t uint64_t
#define offset_t uint64_t
#define kvsize_t uint32_t
#define ALIGNMENT_DATA 8
#define MAGIC_NUM 0xdeadbeef

uint64_t align_size(uint64_t size, int align_size);

#endif
