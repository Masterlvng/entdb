#include "version.h"
#include <iostream>

using namespace std;
using namespace entdb;

Version::Version()
{

}

Version::~Version()
{

}

Status Version::Open(const std::string& filename)
{
    return OpenFile(filename);
}

version_t Version::CurVersion(int target)
{
    version_t v;
    uint64_t off = target * sizeof(version_t);
    
    Sync();
    v = *(uint64_t*)(data_ + off); 
    Sync();

    return v;
}

version_t Version::IncVersion(int target)
{
    version_t v;
    uint64_t off = target * sizeof(version_t);
   

    //flock(fd_, LOCK_EX);

    v = *(uint64_t*)(data_ + off); 
    ++v;
    *(uint64_t*)(data_ + off) = v;
    Sync();

    //flock(fd_, LOCK_UN);

    return v;
}

Status Version::OpenFile(const std::string& filename)
{
    Status s;
    if (access(filename.c_str(), F_OK) == -1)
    {
       s = CreateFile(filename.c_str()); 
       return s;
    }
    if ((fd_ = open(filename.c_str(), O_RDWR)) == -1)
    {
        return Status::IOError("Fail to open file");
    }
    data_ = static_cast<char*>(mmap(
                                    0,
                                    3*sizeof(version_t),
                                    PROT_WRITE | PROT_READ,
                                    MAP_SHARED,
                                    fd_,
                                    0));

    if (data_ == MAP_FAILED)
    {
        return Status::IOError("Fail to mmap file", strerror(errno));
    }
    return Status::OK();
}

Status Version::CreateFile(const std::string& filename)
{
    if((fd_ = open(filename.c_str(), O_RDWR | O_CREAT, 0644)) == -1)
    {
        return Status::IOError("Fail to create file", strerror(errno));
    }
    if (ftruncate(fd_, 3*sizeof(version_t)) == -1)
    {
        close(fd_);
        return Status::IOError("Fail to truncate file", strerror(errno));
    }
    int buf_size = 3*sizeof(version_t);
    char buf[buf_size];
    memset(buf, 0, buf_size);

    data_ = static_cast<char*>(mmap(
                                    0,
                                    3*sizeof(version_t),
                                    PROT_WRITE | PROT_READ,
                                    MAP_SHARED,
                                    fd_,
                                    0));

    memcpy(data_, buf, buf_size);
    if (data_ == MAP_FAILED)
    {
        return Status::IOError("Fail to mmap file", strerror(errno));
    }
    return Status::OK();
}

Status Version::CloseFile()
{
    munmap(data_, 3*sizeof(version_t));
    close(fd_);
    return Status::OK();
}


Status Version::Sync()
{
    if (msync(data_, 3*sizeof(version_t), MS_SYNC) == -1)
        return Status::IOError("Fail to sync");
    return Status::OK();
}
