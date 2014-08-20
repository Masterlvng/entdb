#include "../sync_mgr.h"
#include <unistd.h>
#include <iostream>

using namespace entdb;
using namespace std;

int main()
{
    SyncMgr m = SyncMgr("/tmp");    
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    
    pthread_cond_t* cond = m.condr(1);
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(cond, &mutex);
    cout << "awake" << endl;
    pthread_mutex_unlock(&mutex);
}
