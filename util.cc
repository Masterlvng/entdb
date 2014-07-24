#include "util.h"

uint64_t align_size(uint64_t size, int alignment_size)
{
   if (size % alignment_size == 0)
       return size;
   else
   {
       return (size / alignment_size + 1 ) * alignment_size;
   }
}
