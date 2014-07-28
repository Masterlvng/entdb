#ifndef INDEX_H
#define INDEX_H

#include "status.h"
#include "data_pool.h"

namespace entdb
{
    const uint32_t ENTRY_SIZE = 80;
    class Index
    {
        public:
            Index(){}
            virtual ~Index(){}
            virtual Status Open(const std::string &db_name,
                                const DataPool* dp,
                                int num_entries) = 0;        
            virtual Status Close() = 0;
            virtual Status Get(const std::string& key,
                                offset_t* off,
                                uint64_t* size,
                                uint64_t* padding) = 0;
            virtual Status Put(const std::string& key,
                                uint64_t value_size,
                                offset_t off,
                                uint64_t size_total) = 0;
            virtual Status Delete(const std::string& key) = 0;
            virtual Status Sync() = 0;
    };
};
#endif
