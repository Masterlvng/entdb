#include "../sk_index.h"
#include "../sync_mgr.h"
#include "../version.h"

#include <iostream>

using namespace std;
using namespace entdb;

int main()
{
    Version v;
    v.Open("/tmp/version");
    SyncMgr sm = SyncMgr("/tmp");
    SKIndex ski;
    ski.Open("/tmp/entIndex", &v, sm.mutexr(entdb::INDEX), sm.condr(entdb::INDEX), 1024);
    string s("asdfsss");
    ski.Put(s, 1122, 1233, 22222);

    sleep(2);

    //uint64_t off, size;
    //ski.Get("asdf", &off, &size);
    //cout << off <<", "<< size << endl;
    cout << ski.NumIndex() << endl;
    return 0;
}

