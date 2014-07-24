#ifndef DATA_POOL_H
#define DATA_POOL_H

#include "status.h"
#include "util.h"
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <errno.h>
#include <vector>

namespace entdb {

    class DataPool
    {
        public:
            DataPool();
            ~DataPool();
            Status Open(const std::string& filename, uint64_t size_data);
            Status Close();
            Status Read(offset_t off, uint64_t size, std::vector<char>* data);
            Status Write(offset_t off, uint64_t size, const std::vector<char>& data);
            //int CompareData(offset_t off, uint64_t size, const std::string& data);
            Status ExpandMemory(uint64_t new_size);
            uint64_t PoolSize() { return header_->size; }
            Status Sync();

        private:
            Status CreateFile(const std::string& filename, uint64_t size_data);
            Status OpenFile(const std::string& filename, uint64_t size_data);
            Status CloseFile();
            Status ExpandFile(uint64_t new_size);
            
            char* data_;

            typedef struct header
            {
                uint64_t size;
            } header_t;

            header_t* header_;
            int fd_;
            std::string filename_;
    };
};
#endif
