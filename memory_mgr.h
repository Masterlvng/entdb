#ifndef MEMORY_MGR_H
#define MEMORY_MGR_H

#include "status.h"
#include "util.h"
#include "fm_persistence_mgr.h"
#include "data_pool.h"
#include "version.h"

#include <string>
#include <map>
#include <set>

#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>

#include <iostream>


#define EVENT_SIZE ( sizeof(struct inotify_event) )
#define BUF_SIZE ( 1024 * ( EVENT_SIZE + 16 ) )

namespace entdb
{

    class MemoryMgr
    {
        public:
            MemoryMgr();
            ~MemoryMgr();
            Status Open(const std::string& filename,
                        DataPool* dp,
                        Version* v,
                        pthread_cond_t* cond,
                        uint64_t data_size,
                        offset_t off_init);
            Status Close();
            Status Allocate(uint64_t req_size, offset_t* off, uint64_t* rsp_size);
            Status Free(offset_t off, uint64_t size);
            Status Sync();
            uint64_t AlignSize(uint64_t req_size);
            //temp
            uint64_t Size();
            
        private:
            std::string filename_;
            DataPool* dp_;
            Version* v_;
            version_t cur_v_;
            FMBMgr* fb_mgr_;
                        
            pthread_t loop_id_;
            pthread_cond_t* cond_;
            pthread_mutex_t mutex_;
            
            std::set<fm_block_t> set_fm_;
            
            void onFileChange();
            void SniffingLoop();
            void StartLoop();
            void UpdateDS(); //更新数据结构

            static void exit_thread(int sig) { pthread_exit(NULL); }
            static void* loop_wrapper(void *context) { ((MemoryMgr* )context)->SniffingLoop(); return NULL;}
    };
};
#endif
