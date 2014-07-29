#include "fm_persistence_mgr.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace entdb;

Status FMBMgr::Open(const std::string& filename)
{
    //openfile
    //if fail, then create file
    //map header and freememory
    filename_ = filename;
    return OpenFile(filename_);
}

Status FMBMgr::Close()
{
    //close file
    //mumap
    return CloseFile();
}

Status FMBMgr::AddBlock(offset_t offset, uint64_t size)
{
    //new block
    //select free slots
    //if full expand file
    //set pos back to new block
    int page_size = getpagesize();
    uint64_t size_file;
    RLockFile(0, page_size);    
    if (header_->num_slots_free == 0)
    {
        Status s = ExpandFile();
        if (!s.IsOK())
        {
            UnLockFile(0, page_size);
            return s;
        }
    }

    UnLockFile(0, page_size);

    fm_block_t fbt;
    uint64_t pos = free_slots_.back();
    free_slots_.pop_back();
    
    
    RLockFile(0, page_size);
    size_file = header_->size_file;
    UnLockFile(0, page_size);

    WLockFile(0, size_file);

    header_->num_slots_free -= 1;
    fbt.pos = pos;
    fbt.offset = offset;
    fbt.size = size;
    map_fm_[fbt.offset] = fbt;
    freememory_[pos] = fbt;
    
    UnLockFile(0, size_file);
    return Status::OK();
    
}

Status FMBMgr::UpdateBlock(offset_t off, offset_t newoff, uint64_t size)
{
    // update freememory_ and map_fm
    
    fm_block_t fbt = map_fm_[off];
    fbt.size = size;
    map_fm_.erase(off);
    map_fm_[newoff] = fbt;
    
    WLockFile(fbt.pos * sizeof(fm_block_t), sizeof(fm_block_t));

    freememory_[fbt.pos].offset = newoff;
    freememory_[fbt.pos].size = size;
    
    UnLockFile(fbt.pos * sizeof(fm_block_t), sizeof(fm_block_t));

    return Status::OK();
}

Status FMBMgr::DeleteBlock(offset_t off)
{
    if (map_fm_.find(off) != map_fm_.end())
    {
        uint64_t pos = map_fm_[off].pos;
        map_fm_.erase(off);
        freememory_[pos].size = 0; //free
        freememory_[pos].offset = 0;
        header_->num_slots_free += 1;
        free_slots_.push_back(pos);
    }
    return Status::OK();
}

Status FMBMgr::Sync()
{
    int page_size = getpagesize();
    if (msync(header_, page_size, MS_SYNC) != 0)
        return Status::IOError("Could sync header memory", strerror(errno));
    if (msync(freememory_, size_of_freememory_, MS_SYNC) != 0)
        return Status::IOError("Cound sync freememory memory");
    return Status::OK();
}

Status FMBMgr::CreateFile(const std::string& filename)
{
    if ((fd_ = open(filename.c_str(), O_RDWR | O_CREAT, 0644)) < 0)
    {
        return Status::IOError("Can not create file", strerror(errno));
    }
    //init free memory file
    //分配给header的空间是一个page的大小，暂时用不完，留作扩展之用
    int page_size = getpagesize();
    char buf[page_size];
    memset(buf, 0, page_size);
    uint64_t num_slots = page_size / sizeof(fm_block_t);
    size_of_freememory_ = page_size;
    fm_header_t *fht = reinterpret_cast<fm_header_t*>(buf);

    fht->num_slots_total = num_slots;
    fht->num_slots_free = num_slots - 1;
    fht->num_pages = 1;
    fht->size_file = 2 * page_size;

    write(fd_, buf, page_size);

    //初始分配给free memory block的大小也是一个page
    
    memset(buf, 0, page_size);
    fm_block_t *fbt = reinterpret_cast<fm_block_t*>(buf);

    fbt->offset = init_offset_;
    fbt->size = size_of_data_ - init_offset_;
    write(fd_, buf, page_size);

    close(fd_);
    return Status::OK();
}

Status FMBMgr::OpenFile(const std::string& filename)
{
    //
    if (access(filename.c_str(), F_OK) == -1)
    {
        Status s = CreateFile(filename);
        if (!s.IsOK())
            return s;
    }
    if ((fd_ = open(filename.c_str(), O_RDWR)) == -1)
    {
        return Status::IOError(filename, strerror(errno));
    }
    int page_size = getpagesize();

    header_ = static_cast<fm_header_t*>(mmap(0, 
                                            page_size, 
                                            PROT_READ | PROT_WRITE, 
                                            MAP_SHARED, 
                                            fd_, 0));
    if (header_ == MAP_FAILED)
        return Status::IOError("fail to mmap file", strerror(errno));

    size_of_freememory_ = header_->size_file - page_size;
    freememory_ = static_cast<fm_block_t*>(mmap(0,
                                                size_of_freememory_,
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                fd_,
                                                page_size));     

    if (freememory_ == MAP_FAILED)
        return Status::IOError("mmap fail for memory blocks", strerror(errno));

    int num_slots = size_of_freememory_ / sizeof(fm_block_t);
    int num_free = 0;
    int num_used = 0;
    int i;
    for (i = 0; i < num_slots; ++i)
    {
        if (freememory_[i].size == 0)
        {
            free_slots_.push_back(i);
            ++num_free;
        }
        else
        {
            fm_block_t fmb = {i, freememory_[i].offset, freememory_[i].size};
            map_fm_[fmb.offset] = fmb;
            ++num_used;
        }
    }
    
    return Status::OK();
}

Status FMBMgr::CloseFile()
{
    int page_size = getpagesize();
    munmap(header_, page_size);
    munmap(freememory_, size_of_freememory_);
    close(fd_);
    return Status::OK();

}

Status FMBMgr::ExpandFile()
{
    munmap(freememory_, size_of_freememory_);

    // 增长两倍
    uint64_t add_times = 2;
    uint64_t page_size = getpagesize();
    uint64_t fm_size_cur = header_->num_pages * page_size;
    uint64_t num_slots_cur = fm_size_cur / sizeof(fm_block_t);

    uint64_t num_slots_new = num_slots_cur*add_times;
    uint64_t size_of_freememory_new = num_slots_new * page_size;

    if (ftruncate(fd_, page_size + size_of_freememory_new) != 0)
    {
        return Status::IOError("Could not expand file", strerror(errno));
    }

    header_->size_file = page_size + size_of_freememory_new;
    header_->num_pages *= 2;
    header_->num_slots_free += (num_slots_new - num_slots_cur);
    header_->num_slots_total = num_slots_new;
    size_of_freememory_ = size_of_freememory_new;

    //remap freememory
    freememory_ = static_cast<fm_block_t*>(mmap(0,
                                                size_of_freememory_,
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                fd_,
                                                page_size));

    if (freememory_ == MAP_FAILED)
        return Status::IOError("mmap failed for remap freememory", strerror(errno));

    uint64_t i;
    // new slots
    for (i = num_slots_cur; i < num_slots_new; ++i)
        free_slots_.push_back(i);
    return Status::OK();

}

Status FMBMgr::UnLockFile(uint64_t off, uint64_t size)
{
    struct flock fl;
    int rv;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = off;
    fl.l_len = size;

    rv = fcntl(fd_, F_SETLK, &fl);
    if (rv < 0)
        return Status::IOError("Fail to UnRLockFile", strerror(errno));
    return Status::OK();
}

Status FMBMgr::RLockFile(uint64_t off, uint64_t size)
{
    
    struct flock fl;
    int rv;
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = off;
    fl.l_len = size;

    rv = fcntl(fd_, F_SETLK, &fl);
    if (rv < 0)
        return Status::IOError("Fail to RLockFile", strerror(errno));

    return Status::OK();
}

Status FMBMgr::WLockFile(uint64_t off, uint64_t size)
{
    struct flock fl;
    int rv;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = off;
    fl.l_len = size;

    rv = fcntl(fd_, F_SETLK, &fl);
    if (rv < 0)
        return Status::IOError("Fail to WLockFile", strerror(errno));

    return Status::OK();
}

