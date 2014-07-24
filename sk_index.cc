#include "sk_index.h"

using namespace entdb;
using namespace std;

SKIndex::SKIndex()
{

}

SKIndex::~SKIndex()
{

}

uint64_t SKIndex::NumIndex()
{
   return header_->num_entries - header_->num_free_entries; 
}

Status SKIndex::Get(const std::string& key,
                    offset_t* off,
                    uint64_t* size)
{
    entry_t e;
    if (index_.Search(key, e))
    {
        *off = e.off;
        *size = e.size;
        return Status::OK();
    }
    else
        return Status::NotFound("not found");
}

Status SKIndex::Put(const std::string& key,
                    uint64_t value_size,
                    offset_t off)
{
    Status s;
    entry_t e;
    if(header_->num_free_entries == 0)
    {
        s = ExpandFile();
        if (!s.IsOK()) return s;
    }
    if (index_.Search(key, e))
    {
        //found
        e.off = off;
        e.size = value_size;
        index_.Insert(key, e);
        writeEntry(data_ + e.pos * ENTRY_SIZE, e, true);
    }
    else
    {
        uint64_t pos = free_entry_solts.back();
        free_entry_solts.pop_back();

        e.pos = pos;
        e.key = key;
        e.off = off;
        e.size = value_size;

        index_.Insert(key, e);
        writeEntry(data_ + pos * ENTRY_SIZE, e, false);
    }
    
    return Status::OK();
}

Status SKIndex::Delete(const std::string& key)
{
    entry_t e;
    index_.Delete(key , e);
    recycleEntry(e.pos);
    return Status::OK();
}

Status SKIndex::Sync()
{
    int page_size = getpagesize();
    if(msync(header_, page_size, MS_SYNC) != 0)
    {
        return Status::IOError("Could not sync mmap header_", strerror(errno));
    }
    
    if(msync(data_, header_->num_entries * ENTRY_SIZE, MS_SYNC) != 0)
    {
        return Status::IOError("Could not sync mmap header_", strerror(errno));
    }
    return Status::OK();
}

int SKIndex::readEntry(const char* data, entry_t& e)
{
    /* uint32_t flag
     * uint32_t keysize
     * char* key // max 48
     * uint64_t off
     * uint64_t size
     * 结构大小固定为72
     * */
   e.pos = (data - data_) / ENTRY_SIZE;

   uint32_t flag = *(uint32_t*)data;
   if (flag != 1931)
       return 0;
   data += sizeof(uint32_t);
   uint32_t keysize = *(uint32_t*) data;
   data += sizeof(uint32_t);
   string s(data, data+keysize);
   e.key = s;
   data += keysize;
   e.off = *(uint64_t*)data;
   data += sizeof(uint64_t);
   e.size = *(uint64_t*)data;

   return 1;
}

void SKIndex::writeEntry(char* data, const entry_t& e, bool over_write)
{
    // free flag
   *(uint32_t*)data = 1931; 
   data += sizeof(uint32_t);
   
   //keysize
   *(uint32_t*)data = e.key.length();
   data += sizeof(uint32_t);

   //key 
   memcpy(data, e.key.c_str(), e.key.length());
   data += e.key.length();
    
   //offset
   *(uint64_t*)data = e.off;
   data += sizeof(uint64_t);

   //size
    *(uint64_t*)data = e.size;

    if(!over_write)
        header_->num_free_entries -= 1;
};

void SKIndex::recycleEntry(const uint32_t pos)
{
   uint32_t addr_off = pos * ENTRY_SIZE; 
   char* addr = data_ + addr_off;
   memset(addr, 0, ENTRY_SIZE);
   free_entry_solts.push_back(pos);
   header_->num_free_entries += 1;
}


Status SKIndex::CreateFile(const string& filename, uint64_t num_entries)
{
    if((fd_=open(filename.c_str(), O_RDWR | O_CREAT, 0644)) == -1)
    {
       return Status::IOError("Fail to create file in SKIndex,", strerror(errno)); 
    }

    uint32_t page_size = getpagesize();
    char headerbuf[page_size];
    memset(headerbuf, 0, page_size);
    header_t* h = (header_t*)headerbuf;
    h->num_entries = num_entries;
    h->num_free_entries = num_entries;

    write(fd_, headerbuf, page_size);

    uint64_t entry_size = num_entries * ENTRY_SIZE; 
    uint64_t whole_size = entry_size + page_size;

    if(ftruncate(fd_, whole_size) == -1)
    {
        return Status::IOError("Fail to fruncate file in SKIndex,", strerror(errno));
    }
    close(fd_);
    return Status::OK();
}

Status SKIndex::Open(const string& filename, uint64_t num_entries)
{
    return OpenFile(filename, num_entries);
}

Status SKIndex::OpenFile(const string& filename, uint64_t num_entries)
{
    Status s;
    uint32_t page_size = getpagesize();
    if(access(filename.c_str(), F_OK) == -1)
    {
       s = CreateFile(filename, num_entries); 
       if (!s.IsOK()) return s;
    }
    if((fd_ = open(filename.c_str(), O_RDWR)) == -1)
    {
        return Status::IOError("Fail to open file in SKIndex OpenFile", strerror(errno));
    }

    header_ = static_cast<header_t*>(mmap(
                                         0,
                                         page_size,
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED,
                                         fd_,
                                         0));

    if (header_ == MAP_FAILED)
        return Status::IOError("Fail to mmap", strerror(errno));

    uint64_t data_size = header_->num_entries * ENTRY_SIZE;
    
    data_ = static_cast<char*>(mmap(
                                    0,
                                    data_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd_,
                                    page_size));
    if (data_ == MAP_FAILED)
        return Status::IOError("Fail to mmap", strerror(errno));

    char* tmp_data = data_; 
    entry_t e;
    for(; (uint64_t)(tmp_data - data_) < data_size; tmp_data += ENTRY_SIZE)
    {
        if (readEntry(tmp_data, e))
        {
            // used entry
            index_.Insert(e.key, e);
        }
        else
        {
            // free entry
            uint64_t pos = (tmp_data - data_) / ENTRY_SIZE;
            free_entry_solts.push_back(pos);
        }
    }
    return Status::OK();
}

Status SKIndex::Close()
{
    return CloseFile();
}

Status SKIndex::CloseFile()
{
    uint32_t page_size = getpagesize();
    munmap(data_, header_->num_entries * ENTRY_SIZE);
    munmap(header_, page_size);
    close(fd_);
    return Status::OK();
}

Status SKIndex::ExpandFile()
{
    uint32_t page_size = getpagesize();
    munmap(data_, header_->num_entries * ENTRY_SIZE);
    uint64_t new_data_size = header_->num_entries * ENTRY_SIZE * 2;    
    if (ftruncate(fd_, page_size + new_data_size) == -1)
    {
        Status::IOError("Fail to Expand File In SKIndex", strerror(errno));
    }

    //re map
    data_ = static_cast<char*>(mmap(
                                    0,
                                    new_data_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd_,
                                    page_size));

    if (data_ == MAP_FAILED)
    {
        return Status::IOError("Fail to mmap in Expand File");
    }
    
    // update info
    uint64_t i = header_->num_entries;
    header_->num_free_entries += header_->num_entries; 
    header_->num_entries = header_->num_entries * 2;
    
    for (;i < header_->num_entries; ++i)
        free_entry_solts.push_back(i);
    return Status::OK();
}


