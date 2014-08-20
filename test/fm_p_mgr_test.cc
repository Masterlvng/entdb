#include "../fm_persistence_mgr.h"
#include <iostream>

using namespace entdb;
using namespace std;
int main()
{
    FMBMgr mgr("entdb_mgr", 102400, 0);
    mgr.Open("/tmp/entdb");
    mgr.AddBlock(0,4, 3);
    mgr.AddBlock(16,4, 3);
    mgr.AddBlock(44,4, 3);
    mgr.AddBlock(882,4, 3);
    mgr.DeleteBlock(16, 4);
    return 0;
}
