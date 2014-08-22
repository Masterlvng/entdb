#include "../memory_mgr.h"
#include "../data_pool.h"
#include "../version.h"
#include "../sync_mgr.h"

#include <iostream>
#include <stdint.h>
#include <unistd.h>

using namespace std;
using namespace entdb;

int main()
{
    DataPool dp;
    dp.Open("/tmp/entdb_data", 1024);
    Version v;
    v.Open("/tmp/version");
    MemoryMgr m_mgr;
    SyncMgr sm = SyncMgr("/tmp");

    m_mgr.Open("/tmp/ent_fm", &dp, &v,sm.condr(entdb::FM), 1024, 0);
    uint64_t off, off1;
    uint64_t size, size1;
    m_mgr.Allocate(124, &off, &size);
    m_mgr.Allocate(124, &off1, &size1);
    cout << "off: " << off << endl;
    cout << "off1: " << off1 << endl;
    sleep(2);
    m_mgr.Free(off,size);
    m_mgr.Free(off1,size1);
    m_mgr.Close();
    return 1;
}
