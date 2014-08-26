#ifndef SYNC_MGR_H
#define SYNC_MGR_H

#include <string>
#include <pthread.h>
#include "status.h"

namespace entdb{
    enum
    {
        INDEX,
        FM,
        DATA
    };

    class SyncMgr
    {
        public:
            SyncMgr(std::string location);
            ~SyncMgr();
            pthread_mutex_t* mutexr(int module);
            pthread_cond_t* condr(int module);

        private:
            std::string location_;
            int fd_;
            char* data_;
            Status OpenFile();
            Status CreateFile();
            Status CloseFile();
            int FileSize();
            void InitMutex(char* addr);
            void InitCond(char* addr);
    };
}
#endif
