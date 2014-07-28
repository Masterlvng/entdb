#ifndef MEMORY_MGR_H
#define MEMORY_MGR_H

#include "status.h"
#include "util.h"
#include "fm_persistence_mgr.h"
#include "data_pool.h"

#include <string>
#include <map>
#include <set>

namespace entdb
{

    class MemoryMgr
    {
        public:
            MemoryMgr();
            ~MemoryMgr();
            Status Open(const std::string& filename,
                        DataPool* dp,
                        uint64_t data_size,
                        offset_t off_init);
            Status Close();
            Status Allocate(uint64_t req_size, offset_t* off, uint64_t* rsp_size);
            Status Free(offset_t off, uint64_t size);
            Status Sync();
            uint64_t AlignSize(uint64_t req_size);
            
        private:
            std::string filename_;
            DataPool* dp_;
            FMBMgr* fb_mgr_;

            std::map<offset_t, fm_block_t> map_fm_;
            std::set<fm_block_t> set_fm_;
    };
};
#endif
