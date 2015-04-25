#include "sk_index.h"

using namespace entdb;
using namespace std;

SKIndex::SKIndex():index_(comp_)
{
    cur_v_ = 0;
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
                    uint64_t* size,
                    uint64_t* disk_size)
{
    pthread_mutex_lock(outermutex_);
    entry_t e;
    UpdateDS();
    if (index_.Search(key, e))
    {
        *off = e.off;
        *size = e.value_size;
        *disk_size = e.disk_size;
        pthread_mutex_unlock(outermutex_);
        return Status::OK();
    }
    else
    {
        pthread_mutex_unlock(outermutex_);
        return Status::NotFound("not found");
    }
}

Status SKIndex::Put(const std::string& key,
                    uint64_t value_size,
                    offset_t off,
                    uint64_t disk_size)
{
    Status s;
    entry_t e;

    pthread_mutex_lock(outermutex_);
    UpdateDS();
    if(header_->num_free_entries == 0)
    {
        s = ExpandFile();
        if (!s.IsOK())
        {
            pthread_mutex_unlock(outermutex_);
            return s;
        }
    }
    cur_v_ = v_->IncVersion(INDEX);
    if (index_.Search(key, e))
    {
        //found
        //update
        e.off = off;
        e.value_size = value_size;
        e.disk_size = disk_size;
        index_.Insert(key, e);
        writeEntry((char*)data_ + e.pos * ENTRY_SIZE, cur_v_, e, true);
    }
    else
    {
        uint64_t pos = *free_entry_solts.begin();
        free_entry_solts.erase(free_entry_solts.begin());

        e.pos = pos;
        e.key = key;
        e.off = off;
        e.value_size = value_size;
        e.disk_size = disk_size;
        index_.Insert(key, e);
        writeEntry((char*)data_ + pos * ENTRY_SIZE, cur_v_, e, false);
    }
    header_->v = cur_v_; 
    pthread_cond_broadcast(cond_);
    pthread_mutex_unlock(outermutex_);
    return Status::OK();
}

Status SKIndex::Delete(const std::string& key)
{
    entry_t e;
    pthread_mutex_lock(outermutex_);
    cur_v_ = v_->IncVersion(INDEX);
    index_.Delete(key , e);
    recycleEntry(e);
    header_->v = cur_v_;
    pthread_cond_broadcast(cond_);
    pthread_mutex_unlock(outermutex_);
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
     * uint64_t version
     * uint32_t keysize
     * char* key // max 48
     * uint64_t off
     * uint64_t value size
     * uint64_t disk size
     * 结构大小固定为 88
     * */
   e.pos = (data - (char*)data_) / ENTRY_SIZE;

   e.flag = *(uint32_t*)data;
   if (e.flag != 1931)
       return 0;
   data += sizeof(uint32_t);

   e.v = *(uint64_t*) data;
   data += sizeof(uint64_t);

   uint32_t keysize = *(uint32_t*) data;
   data += sizeof(uint32_t);

   string s(data, data+keysize);
   e.key = s;
   data += keysize;

   e.off = *(uint64_t*)data;
   data += sizeof(uint64_t);

   e.value_size = *(uint64_t*)data;
   data += sizeof(uint64_t);

   e.disk_size = *(uint64_t*)data;

   return 1;
}

void SKIndex::writeEntry(char* data, version_t v, const entry_t& e, bool over_write)
{
    // free flag
   *(uint32_t*)data = 1931; 
   data += sizeof(uint32_t);
   
   //version
   *(uint64_t*)data = v;
   data += sizeof(uint64_t);

   //keysize
   *(uint32_t*)data = e.key.length();
   data += sizeof(uint32_t);

   //key 
   memcpy(data, e.key.c_str(), e.key.length());
   data += e.key.length();
    
   //offset
   *(uint64_t*)data = e.off;
   data += sizeof(uint64_t);

   //value size
    *(uint64_t*)data = e.value_size;
    data += sizeof(uint64_t);

    // disk size
    *(uint64_t*)data = e.disk_size;

    if(!over_write)
    {
        //23333 if run in multi thread, it's dangerous
        header_->num_free_entries -= 1;
    }
};

void SKIndex::recycleEntry(const entry_t e)
{
   uint32_t addr_off = e.pos * ENTRY_SIZE; 
   char* addr = (char*)data_ + addr_off;

   entry_t e1 = *(entry_t*)(addr);
   memset(addr, 0, ENTRY_SIZE);
   ((entry_t*)addr)->key = e1.key;
   ((entry_t*)addr)->v = cur_v_;
   *(uint32_t*)addr = 0; // set free

   free_entry_solts.insert(e.pos);
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

Status SKIndex::Open(const string& filename, Version* v, 
        pthread_mutex_t* outmutex, pthread_cond_t* cond, uint64_t num_entries)
{
    v_ = v;
    cond_ = cond;
    outermutex_ = outmutex;
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
    
    data_ = static_cast<entry_t*>(mmap(
                                    0,
                                    data_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd_,
                                    page_size));
    if (data_ == MAP_FAILED)
        return Status::IOError("Fail to mmap", strerror(errno));

    char* tmp_data = (char*)data_; 
    entry_t e;
    version_t new_v = 0;
    for(; (uint64_t)(tmp_data - (char*)data_) < data_size; tmp_data += ENTRY_SIZE)
    {
        int ret = readEntry(tmp_data, e);
        new_v = (e.v > new_v)? e.v:new_v;
        if (ret != 0)
        {
            // used entry
            index_.Insert(e.key, e);
        }
        else
        {
            // free entry
            uint64_t pos = (tmp_data - (char*)data_) / ENTRY_SIZE;
            free_entry_solts.insert(pos);
        }
    }
    cur_v_ = new_v;
    return Status::OK();
}

Status SKIndex::Close()
{
    pthread_kill(loop_id, SIGUSR1);
    pthread_join(loop_id, NULL);
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
    data_ = static_cast<entry_t*>(mmap(
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
        free_entry_solts.insert(i);
    return Status::OK();
}

void SKIndex::onFileChange()
{
    pthread_mutex_lock(outermutex_);
    UpdateDS();
    pthread_mutex_unlock(outermutex_);
}

void SKIndex::SniffingLoop()
{
    signal(SIGUSR1, exit_thread);
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while(1)
    {
       pthread_mutex_lock(&innermutex_); 
       pthread_cond_wait(cond_, &innermutex_); 
       onFileChange();
       pthread_mutex_unlock(&innermutex_);
    }
}

void SKIndex::StartLoop()
{
    pthread_create(&loop_id, NULL, loop_wrapper, this);
}

void SKIndex::UpdateDS()
{
   if(header_->v <= cur_v_)
       return;
   uint64_t i;
   version_t new_v = cur_v_;
   entry_t e;
   for(i = 0; header_->num_entries; i++)
   {
       if (data_[i].v > cur_v_)
       {
            new_v = (data_[i].v > new_v)? data_[i].v:new_v;
            if(data_[i].flag == 0)
            {
                // free
                index_.Delete(data_[i].key, e);
                free_entry_solts.insert(i);
            }
            else
            {
                index_.Insert(data_[i].key, data_[i]); 
                set<uint64_t>::iterator it = free_entry_solts.find(i);
                if (it != free_entry_solts.end())
                    free_entry_solts.erase(i);
            }
       }
   }
}


