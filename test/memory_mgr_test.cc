#include "../memory_mgr.h"
#include "../data_pool.h"

#include <iostream>
#include <stdint.h>

using namespace std;
using namespace entdb;

int main()
{
    DataPool dp;
    dp.Open("/tmp/entdb_data", 1024);
    MemoryMgr m_mgr;
    m_mgr.Open("/tmp/ent_fm", &dp, 1024, 0);
    uint64_t off, off1;
    uint64_t size, size1;
    m_mgr.Allocate(124, &off, &size);

    m_mgr.Allocate(124, &off1, &size1);

    m_mgr.Free(off,size);
    m_mgr.Free(off1,size1);
    return 1;
}
