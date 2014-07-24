#include "entdb.h"
#include <sys/stat.h>
#include <iostream>

using namespace std;
using namespace entdb;

Entdb::Entdb()
{

}

Entdb::~Entdb()
{

}

Status Entdb::Get(const std::string& key, std::vector<char>* value) const
{
    Status s;
    uint64_t off, size;    
    s = index_->Get(key, &off, &size);
    if (!s.IsOK()) return s;
    
    dp_->Read(off + key.size(), size, value);

    return Status::OK();
}

Status Entdb::Put(const std::string& key, const std::vector<char>& value)
{
    uint64_t request_size = key.size() + value.size();
    uint64_t off, rsp_size;

    m_mgr_->Allocate(request_size, &off, &rsp_size);
    
    index_->Put(key, value.size(), off);

    vector<char> s(key.begin(), key.end());
    dp_->Write(off, key.size(), s);
    dp_->Write(off+key.size(), value.size(), value);

    return Status::OK();    
}

Status Entdb::Close()
{
    
    return Status::OK();
}

Status Entdb::Open(const std::string& location, const std::string& db_name)
{
    db_name_ = db_name;
    location_ = location + std::string("/") + db_name_;
    // init instance;
    string index_name = location_ + std::string("/index");

    string dp_name = location_ + std::string("/data");
    string fm_name = location_ + std::string("/fm");
   
    if (access(location_.c_str(), F_OK) == -1)
    {
        if(mkdir(location_.c_str(), 0744) == -1)
        {
            return Status::IOError("Fail to create directory in Entdb", strerror(errno));
        }
    }
   
    index_ = new SKIndex();

    index_->Open(index_name, 1024);

    dp_ = new DataPool();
    // 数值待定
    dp_->Open(dp_name, 1024);

    m_mgr_ = new MemoryMgr();

    m_mgr_->Open(fm_name, dp_, 1024, 0);

    return Status::OK();
}

Status Entdb::Delete(const std::string& key)
{
    
    return Status::OK();
}

Status Entdb::Sync()
{
    return Status::OK();
}  
