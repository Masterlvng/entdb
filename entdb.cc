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
    uint64_t off, value_size, disk_size;    
    s = index_->Get(key, &off, &value_size, &disk_size);
    if (!s.IsOK()) return s;
    
    dp_->Read(off + key.size(), value_size, value);

    return Status::OK();
}

Status Entdb::Put(const std::string& key, const std::vector<char>& value)
{
    uint64_t request_size = key.size() + value.size();
    uint64_t aligned_size = m_mgr_->AlignSize(request_size);
    uint64_t off,  value_size, disk_size;
    // 如果存在，返回在datapool的位置信息    
    // 如果大小刚好能用，则m_mgr 不需要再分配了
    
    vector<char> s(key.begin(), key.end());
    if (index_->Get(key, &off, &value_size, &disk_size).IsOK())
    {
        //大小刚好
       if (disk_size >= aligned_size)
       {
            dp_->Write(off+key.size(), value.size(), value);
            return Status::OK();
       }
       else
       {
            m_mgr_->Free(off, disk_size);
       }
    }
    m_mgr_->Allocate(request_size, &off, &disk_size);
    index_->Put(key, value.size(), off, disk_size);

    dp_->Write(off, key.size(), s);
    dp_->Write(off+key.size(), value.size(), value);
    
    return Status::OK();    
}

Status Entdb::Close()
{
    if (!index_->Close().IsOK() || !dp_->Close().IsOK() || !m_mgr_->Close().IsOK())
        return Status::IOError("Fail to Close");
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
    //删除索引记录
    //回收memory block
    uint64_t off, value_size, disk_size;
    if (index_->Get(key, &off, &value_size, &disk_size).IsOK())
    {
       m_mgr_->Free(off, disk_size); 
       return Status::OK();
    }
    return Status::NotFound("Key not exists");
}

Status Entdb::Sync()
{
    if (!dp_->Sync().IsOK() || !index_->Sync().IsOK() || !m_mgr_->Sync().IsOK())
        return Status::IOError("Sync error");
    return Status::OK();
}  
