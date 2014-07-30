#ifndef FM_PERSISTENCE_MGR_H
#define FM_PERSISTENCE_MGR_H

#include "util.h"
#include "status.h"
#include "version.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/file.h>

#include <fcntl.h>

namespace entdb {
    typedef struct fm_block
    {
        version_t v;
        index_t pos;
        offset_t offset;
        uint64_t size;
        
        // used for stl set
        bool operator < (const fm_block &block) const
        {
            if (size < block.size || (size == block.size && offset > block.offset))
                return true;
            return false;
        }
    }fm_block_t;

    typedef struct fm_header
    {
        uint64_t size_file;
        uint64_t num_pages;
        uint64_t num_slots_free;
        uint64_t num_slots_total;
    }fm_header_t;

    class FMBMgr // free memory block manager,管理空闲块
    {
        public:
            friend class MemoryMgr;
            FMBMgr(const std::string& db_name, uint64_t size_data, offset_t init_offset)
            : db_name_(db_name),
              size_of_data_(size_data),
              init_offset_(init_offset)
              {}

            ~FMBMgr() {}
            Status Open(const std::string& filename);
            Status Close();
            Status AddBlock(offset_t offset, uint64_t size, version_t v);
            Status DeleteBlock(offset_t off, version_t v);
            Status UpdateBlock(offset_t off, offset_t newoff, uint64_t size, version_t);
            Status Sync();
            
        private:
            std::string db_name_;
            std::string filename_;
            int fd_;

            std::map<offset_t, fm_block_t> map_fm_; // 需要互斥锁保护这些成员
            std::set<index_t> free_slots_; // 记录空闲块在数组中的位置

            Status CreateFile(const std::string& filename);
            Status OpenFile(const std::string& filename);
            Status CloseFile();
            Status ExpandFile();

            Status RLockFile(uint64_t off, uint64_t size);
            Status WLockFile(uint64_t off, uint64_t size);

            Status UnLockFile(uint64_t off, uint64_t size);

            fm_header_t* header_;
            fm_block_t* freememory_; //空闲块数组
            uint64_t size_of_freememory_;
            uint64_t size_of_data_;
            uint64_t init_offset_;
            
    };
};
#endif
