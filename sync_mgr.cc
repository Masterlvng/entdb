#include "sync_mgr.h"
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

using namespace std;
using namespace entdb;

SyncMgr::SyncMgr(string location)
{
    location_ = location;
    OpenFile();
}

SyncMgr::~SyncMgr()
{
    CloseFile();
}

pthread_mutex_t* SyncMgr::mutexr(int module)
{
    pthread_mutex_t* mutex;
    data_ += 3 * sizeof(pthread_cond_t);
    switch(module)
    {
        case INDEX:
            mutex = (pthread_mutex_t*)data_;
            break;
        case FM:
            mutex = (pthread_mutex_t*)data_ + 1;
            break;
        case DATA:
            mutex = (pthread_mutex_t*)data_ + 2;
            break;
    }
    return mutex;
}

pthread_cond_t* SyncMgr::condr(int module)
{
    pthread_cond_t* cond;
    switch(module)
    {
        case INDEX:
            cond = (pthread_cond_t*)data_;
            break;
        case FM:
            cond = (pthread_cond_t*)data_ + 1;
            break;
        case DATA:
            cond = (pthread_cond_t*)data_ + 2;
            break;
    }
    return cond;
}

int SyncMgr::FileSize()
{
    return 3 * (sizeof(pthread_mutex_t) + sizeof(pthread_cond_t));

}

Status SyncMgr::OpenFile()
{
    string f = location_ + "/sync";
    if (access(f.c_str(), F_OK) != 0)
    {
        return CreateFile();
    }
    fd_ = open(f.c_str(), O_RDWR);
    if (fd_ < 0)
        return Status::IOError("Fail to open file");
    int fz = FileSize();
    data_ = (char*)mmap(0, fz, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0); 
    close(fd_);

    return Status::OK();
}

Status SyncMgr::CreateFile()
{
    string f = location_ + "/sync";
    fd_ = open(f.c_str(), O_RDWR|O_CREAT, 0644);
    if (fd_ < 0)
        return Status::IOError("Fail to create file");
    int fz = FileSize();
    if(ftruncate(fd_, fz) < 0 )
    {
        return Status::IOError("fail to truncate");
    }

    data_ = static_cast<char*>(mmap(0, fz, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0)); 
    
    if (data_ == MAP_FAILED)
        return Status::IOError("Map failed");
    InitCond(data_);
    InitMutex(data_ + 3*sizeof(pthread_mutex_t));

    close(fd_);
    return Status::OK();
}

Status SyncMgr::CloseFile()
{
    munmap(data_, FileSize());
    close(fd_);
    return Status::OK();
}

//将来考虑用来替代文件锁
void SyncMgr::InitMutex(char* addr)
{
    pthread_mutexattr_t attr;    
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    // index
    pthread_mutex_init((pthread_mutex_t*)addr, &attr);
    addr += sizeof(pthread_mutex_t);
    
    //fm
    pthread_mutex_init((pthread_mutex_t*)addr, &attr);
    addr += sizeof(pthread_mutex_t);
    
    //data
    pthread_mutex_init((pthread_mutex_t*)addr, &attr);

}

void SyncMgr::InitCond(char* addr)
{
    pthread_condattr_t attr;    
    pthread_condattr_init(&attr);
    pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    
    // index
    pthread_cond_init((pthread_cond_t*)addr, &attr);
    addr += sizeof(pthread_cond_t);
    
    //fm
    pthread_cond_init((pthread_cond_t*)addr, &attr);
    addr += sizeof(pthread_cond_t);

    //data
    pthread_cond_init((pthread_cond_t*)addr, &attr);

}
