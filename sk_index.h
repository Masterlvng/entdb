#ifndef SK_INDEX_H
#define SK_INDEX_H

#include "index.h"
#include "skiplist.hpp"
#include <string>
#include <vector>
#include <string.h>

namespace entdb
{

    class SKIndex
    {
        public:
            SKIndex();
            ~SKIndex();
            virtual Status Open(const std::string& db_name,
                                uint64_t num_entries);

            virtual Status Close();
            virtual Status Get(const std::string& key,
                                offset_t* off,
                                uint64_t* size,
                                uint64_t* disk_size);

            virtual Status Put(const std::string& key,
                                uint64_t value_size,
                                offset_t off,
                                uint64_t disk_size);

            virtual Status Delete(const std::string& key);
            virtual Status Sync();
            uint64_t NumIndex();
            
        
        private:
            typedef struct header
            {
                uint64_t num_entries;
                uint64_t num_free_entries;
            }header_t;

            typedef struct entry
            {
               uint32_t  pos;
               std::string key;
               uint64_t off; 
               uint64_t value_size;
               uint64_t disk_size;
            }entry_t;
            struct Comparator {
                int operator()(const std::string& a, const std::string& b) const 
                {
                    if ( a < b)
                        return -1;
                    else if( a > b)
                        return 1;
                    else 
                        return 0;
                }
            };

            int readEntry(const char* data, entry_t& e);
            void writeEntry(char* data, const entry_t& e, bool over_write);
            void recycleEntry(const uint32_t pos);    

        private:
            std::string filename_;
            
            struct Comparator comp_;
            typedef util::SkipList<std::string, entry_t, Comparator> Index;
            
            Index index_;
            int fd_;
            std::vector<uint64_t> free_entry_solts;
            header_t* header_;
            char* data_;
            
        private:
            Status CreateFile(const std::string& filename, uint64_t num_entries);
            Status OpenFile(const std::string& filename, uint64_t num_entries);
            Status CloseFile();
            Status ExpandFile();
    };
};
#endif
