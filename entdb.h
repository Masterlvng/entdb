#ifndef ENTDB_H
#define ENTDB_H

#include "status.h"
#include "sk_index.h"
#include "data_pool.h"
#include "memory_mgr.h"
#include "version.h"
#include "sync_mgr.h"
#include <vector>

namespace entdb
{
    class Entdb
    {
        public:
            Entdb();
            ~Entdb();
            Status Open(const std::string& location, const std::string& db_name);
            Status Close();
            Status Get(const std::string& key, std::vector<char>* value) const;
            Status Put(const std::string& key, const std::vector<char>& value);
            Status Delete(const std::string& key);
            Status Sync();

        private:
            std::string db_name_;
            std::string location_;

            SKIndex* index_;
            DataPool* dp_;
            MemoryMgr* m_mgr_;
            Version* v_;
            SyncMgr* sm_;

    };
};

#endif
