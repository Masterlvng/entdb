#include "data_pool.h"
#include <iostream>

using namespace std;
using namespace entdb;

DataPool::DataPool()
{

}

DataPool::~DataPool()
{
    
}

Status DataPool::OpenFile(const std::string& filename, uint64_t size)
{
    Status s;
    if (access(filename.c_str(), F_OK) == -1)
    {
       s = CreateFile(filename, size); 
       if (!s.IsOK()) return s;
    }
    if ((fd_ = open(filename.c_str(), O_RDWR)) == -1)
    {
       return Status::IOError("fail to open datafile", strerror(errno)); 
    }

    int page_size = getpagesize();
    header_ = static_cast<header_t*>(mmap(0,
                                          page_size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED,
                                          fd_,
                                          0));

    if (header_ == MAP_FAILED)
        return Status::IOError("fail to map header", strerror(errno));

    data_ = static_cast<char*>(mmap(0,
                                    header_->size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd_,
                                    page_size)); 
    if (data_ == MAP_FAILED)
        return Status::IOError("fail to map databody", strerror(errno));

    return Status::OK();

}

Status DataPool::CreateFile(const std::string& filename, uint64_t size)
{
    int page_size = getpagesize();    
    if ((fd_ = open(filename.c_str(), O_RDWR | O_CREAT, 0644)) == -1)
    {
        return Status::IOError("Could not open data file");
    }
    char buffer[page_size];
    memset(buffer, 0, page_size);

    header_t *h = (header_t*) buffer;
    h->size = size;
    if (write(fd_, buffer, page_size) < 0)
    {
        return Status::IOError("Fail to write header", strerror(errno));
    }
    if(ftruncate(fd_, page_size + size) != 0)
    {
        return Status::IOError("Fail to create body", strerror(errno));
    }
    close(fd_);
    return Status::OK();
}

Status DataPool::CloseFile()
{
    int page_size = getpagesize();
    munmap(header_, page_size);
    munmap(data_, header_->size);
    close(fd_);
    return Status::OK();
}

Status DataPool::ExpandFile(uint64_t new_size)
{
    int page_size = getpagesize();
    munmap(data_, header_->size);

    if (ftruncate(fd_, page_size + new_size) != 0)
    {
        return Status::IOError("fail to truncate file ", strerror(errno));
    }
    // remap
    data_ = static_cast<char*>((mmap(0,
                                     new_size,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     fd_,
                                     page_size)));

    if (data_ == MAP_FAILED)
        return Status::IOError("fail to remap in datapool expandfile ", strerror(errno));
    header_->size = new_size;
    return Status::OK();
}

Status DataPool::Open(const std::string& filename, uint64_t size)
{
    filename_ = filename;
    Status s = OpenFile(filename_, size);
    return s;
}

Status DataPool::Close()
{
    return CloseFile();
}

Status DataPool::Sync()
{
    int page_size = getpagesize();
    if (msync(header_, page_size, MS_SYNC) != 0)
    {
        return Status::IOError("Fail to sync header", strerror(errno));
    }
    if (msync(data_, header_->size, MS_SYNC) != 0)
    {
        return Status::IOError("Fail to sync data body", strerror(errno));
    }
    return Status::OK();
}

Status DataPool::Read(uint64_t off, uint64_t size, std::vector<char>* data)
{
    char *buffer = new char[size+1];
    buffer[size] = '\0';
    memcpy(buffer, data_+off, size);

    //*data = std::string(buffer);
    *data = std::vector<char>(buffer, buffer+size);

    delete[] buffer;
    return Status::OK();
}

Status DataPool::Write(uint64_t off, uint64_t size, const std::vector<char>& data)
{
    memcpy(data_ + off, data.data(), size);
    return Status::OK();
}

/*
int DataPool::CompareData(offset_t off, uint64_t size, const std::string& data)
{
    std::string ori_data;
    Read(off, size, &ori_data);
    return ori_data.compare(data);
}
*/

Status DataPool::ExpandMemory(uint64_t new_size)
{
    return ExpandFile(new_size);
}

