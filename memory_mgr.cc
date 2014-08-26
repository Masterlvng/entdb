#include "memory_mgr.h"

#include <iostream>

using namespace entdb;
using namespace std;

MemoryMgr::MemoryMgr()
{

}

MemoryMgr::~MemoryMgr()
{
    delete fb_mgr_;
}

uint64_t MemoryMgr::Size()
{
    cout << set_fm_.size() << endl;
    return 0;
}

Status MemoryMgr::Open(const string& filename, DataPool* dp, Version* v, pthread_cond_t* cond,
                        uint64_t data_size, offset_t off_init)
{
    Status s;
    filename_ = filename;
    dp_ = dp;
    v_ = v;
    cur_v_ = 0;
    cond_ = cond;
    pthread_mutex_init(&mutex_, NULL);
    
    fb_mgr_ = new FMBMgr(filename, data_size, off_init);
    s = fb_mgr_->Open(filename_);
    if (!s.IsOK()) return s;
    std::map<offset_t, fm_block_t>::iterator it_fmb;
    
    flock(fb_mgr_->fd_, LOCK_EX);
    // file lock
    for (it_fmb = fb_mgr_->map_fm_.begin(); it_fmb != fb_mgr_->map_fm_.end(); ++it_fmb)
    {
        set_fm_.insert(it_fmb->second); 
    }
    
    StartLoop();
    flock(fb_mgr_->fd_, LOCK_UN);

    return Status::OK();
}

Status MemoryMgr::Close()
{
    pthread_kill(loop_id_, SIGUSR1);
    pthread_join(loop_id_, NULL);
    return fb_mgr_->Close();
}

Status MemoryMgr::Sync()
{
    return fb_mgr_->Sync();
}

Status MemoryMgr::Allocate(uint64_t req_size, offset_t* off_out, uint64_t* rsp_size)
{
    Status s;
    offset_t off;
    uint64_t page_size = getpagesize();
    *rsp_size = align_size(req_size, ALIGNMENT_DATA);

    flock(fb_mgr_->fd_, LOCK_EX);
    UpdateDS();

    fm_block_t fbt; 
    fbt.offset = UINT64_MAX;
    fbt.size = *rsp_size;
    
    std::set<fm_block_t>::iterator it_set;

    //file lock
    it_set = set_fm_.upper_bound(fbt);
    
    if (it_set == set_fm_.end())
    {
        //没找到空位，需要扩展
        
        int add_times = 2;    
        uint64_t size_data_old = dp_->PoolSize();
        uint64_t size_data_new = size_data_old * add_times;
        if (*rsp_size > size_data_new - dp_->PoolSize())
        {
            size_data_new = align_size(*rsp_size+dp_->PoolSize(), page_size);
        }
        s = dp_->ExpandMemory(size_data_new);
        if (!s.IsOK())
        {
           return s; 
        }
        else
        {
           Free(size_data_old, size_data_new - size_data_old); 
           it_set = set_fm_.upper_bound(fbt);
        }
    }

    if (it_set == set_fm_.end())
    {
        return Status::IOError("Out of Memory"); 
    }
    // 节省空间
    if ( (it_set->size > *rsp_size * 2)
            || (it_set->size > page_size && it_set->size - *rsp_size > 1024))
    {
       cur_v_ = v_->IncVersion(FM);
       s = fb_mgr_->UpdateBlock(it_set->offset, it_set->offset +*rsp_size, it_set->size - *rsp_size, cur_v_); 
       off = it_set->offset;
       fm_block_t fbt;
       fbt.v = cur_v_;
       fbt.offset = it_set->offset + *rsp_size;
       fbt.size = it_set->size - *rsp_size;

       set_fm_.erase(*it_set);
       set_fm_.insert(fbt);
    }
    else
   {
       //拿走整块空闲块
       cur_v_ = v_->IncVersion(FM);
       s = fb_mgr_->DeleteBlock(it_set->offset, cur_v_);
       if (!s.IsOK()) return s;
       *rsp_size = it_set->size;
       off = it_set->offset;
       set_fm_.erase(it_set);
    }
    *off_out = off;

    fb_mgr_->header_->v = cur_v_;
    pthread_cond_broadcast(cond_);

    flock(fb_mgr_->fd_, LOCK_UN);

    return Status::OK();
}

Status MemoryMgr::Free(uint64_t off, uint64_t size)
{
    Status s;
    // 合并空闲块
    bool merged_prev = false;
    bool merged_next = false;
    fm_block_t fbt;
    fbt.offset = off;
    fbt.size = size;
    
    flock(fb_mgr_->fd_, LOCK_EX);
    UpdateDS();

    cur_v_ = v_->IncVersion(FM);
    
    fb_mgr_->AddBlock(off, size, cur_v_);
    set_fm_.insert(fbt);
    std::map<uint64_t, fm_block_t>::iterator cur_it, next_it, prev_it;

    //mutex lock
    cur_it = fb_mgr_->map_fm_.find(fbt.offset);
    next_it = fb_mgr_->map_fm_.find(fbt.offset);
    prev_it = fb_mgr_->map_fm_.find(fbt.offset);
    // 向前合并
    if (cur_it != fb_mgr_->map_fm_.begin())
    {
       --prev_it;
       if (prev_it->second.offset + prev_it->second.size == 
                            cur_it->second.offset)
       {
            cout << "merge prev" << endl;
            merged_prev = true; 
            
            fm_block_t fbt_deleted, fbt_add; 
            fbt_deleted.offset = cur_it->second.offset;
            fbt_deleted.size = cur_it->second.size;
            set_fm_.erase(set_fm_.find(fbt_deleted));
            fb_mgr_->DeleteBlock(cur_it->second.offset, cur_v_);

            fb_mgr_->UpdateBlock(prev_it->second.offset, prev_it->second.offset, 
                                            prev_it->second.size + cur_it->second.size, cur_v_);
            fbt_add.offset = prev_it->second.offset;
            fbt_add.size = prev_it->second.size + cur_it->second.size;
            set_fm_.insert(fbt_add);

            cur_it = fb_mgr_->map_fm_.find(fbt_add.offset);
            next_it = fb_mgr_->map_fm_.find(fbt_add.offset);
       }
    }
    // 向后合并
    ++next_it;
    if (cur_it != fb_mgr_->map_fm_.end() &&
            next_it != fb_mgr_->map_fm_.end())
    {

        fm_block_t fbt_cur;
        fbt_cur.offset = cur_it->second.offset;
        fbt_cur.size = cur_it->second.size;
       if (cur_it->second.offset + cur_it->second.size == 
                            next_it->second.offset) 
       {
           cout << "merge next" << endl;
            merged_next = true;
            fb_mgr_->DeleteBlock(next_it->second.offset, cur_v_);
            fb_mgr_->UpdateBlock(cur_it->second.offset, cur_it->second.offset,
                                            cur_it->second.size + next_it->second.size, cur_v_);
            fm_block_t fbt_deleted; 
            fbt_deleted.offset = next_it->second.offset;
            fbt_deleted.size = next_it->second.size;
            set<fm_block_t>::iterator sfm_it = set_fm_.find(fbt_deleted);
            if(sfm_it != set_fm_.end())
                set_fm_.erase(set_fm_.find(fbt_deleted));

            //todo bug
            fm_block_t fbt_new;
            sfm_it = set_fm_.find(fbt_cur);
            if(sfm_it != set_fm_.end())
                set_fm_.erase(set_fm_.find(fbt_cur));
            fbt_new.offset = cur_it->second.offset;
            fbt_new.size = cur_it->second.size;
            set_fm_.insert(fbt_new);

       }
    }
    
    fb_mgr_->header_->v = cur_v_;

    pthread_cond_broadcast(cond_);
    if (merged_next || merged_prev)
    {
       flock(fb_mgr_->fd_, LOCK_UN);
       return Status::OK(); 
    }
    //fb_mgr_->AddBlock(off, size, cur_v_);
    //set_fm_.insert(fbt);

    flock(fb_mgr_->fd_, LOCK_UN);

    return Status::OK();
    
}

uint64_t MemoryMgr::AlignSize(uint64_t req_size)
{
    return align_size(req_size, ALIGNMENT_DATA);
}

void MemoryMgr::StartLoop()
{
    //创建线程，运行sniffingloop
    pthread_create(&loop_id_, NULL, loop_wrapper, this);
}

void MemoryMgr::SniffingLoop()
{
    signal(SIGUSR1, exit_thread); 
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while(1)
    {
        pthread_mutex_lock(&mutex_);
        pthread_cond_wait(cond_, &mutex_);
        onFileChange();
        pthread_mutex_unlock(&mutex_);
    }
}

void MemoryMgr::onFileChange()
{
    //cout << "On file change" << endl;

    flock(fb_mgr_->fd_, LOCK_EX);
    UpdateDS();
    flock(fb_mgr_->fd_, LOCK_UN);
}

void MemoryMgr::UpdateDS()
{
    /*
    fb_mgr_->map_fm_;
    fb_mgr_->free_slots_;
    set_fm_;
    */
    //fb_mgr_->freememory_;
    //fb_mgr_->header_->num_slots_total;
    if (fb_mgr_->header_->v <= cur_v_)
        return;
    uint64_t i;
    version_t new_v = cur_v_;
    for(i = 0; i < fb_mgr_->header_->num_slots_total; ++i)
    {
        if (fb_mgr_->freememory_[i].v > cur_v_)
        {
            if (new_v < fb_mgr_->freememory_[i].v)
                new_v = fb_mgr_->freememory_[i].v;

            if (fb_mgr_->freememory_[i].del == 1)
            {
                fb_mgr_->free_slots_.insert(i);
                //fb_mgr_->map_fm_[fb_mgr_->freememory_[i].offset] = fb_mgr_->freememory_[i];
                //set_fm_.insert(fb_mgr_->freememory_[i]);
                map<uint64_t, fm_block_t>::iterator mit = 
                        fb_mgr_->map_fm_.find(fb_mgr_->freememory_[i].offset);
                if(mit != fb_mgr_->map_fm_.end() && mit->second.size == fb_mgr_->freememory_[i].size)
                {
                    fb_mgr_->pos2fm_.erase(mit->second.pos);
                    fb_mgr_->map_fm_.erase(mit);
                    set<fm_block_t>::iterator s_it = set_fm_.find(fb_mgr_->freememory_[i]);
                    if (s_it != set_fm_.end())
                        set_fm_.erase(s_it);
                }

            }
            else
            {
               // 清除过期无效的free slot
                set<uint64_t>::iterator fs_it = fb_mgr_->free_slots_.find(i);
                if(fs_it != fb_mgr_->free_slots_.end())
                    fb_mgr_->free_slots_.erase(fs_it);
                
                //清除无效数据
                map<uint64_t, fm_block_t>::iterator m_it;
                if(fb_mgr_->pos2fm_.find(i) != fb_mgr_->pos2fm_.end())
                {
                    fm_block_t fmb = fb_mgr_->pos2fm_[i];
                    fb_mgr_->map_fm_.erase(fmb.offset);
                    set_fm_.erase(fmb);
                    fb_mgr_->pos2fm_.erase(i);
                }
                fb_mgr_->map_fm_[fb_mgr_->freememory_[i].offset] = fb_mgr_->freememory_[i];
                fb_mgr_->pos2fm_[i] = fb_mgr_->freememory_[i];
                set_fm_.insert(fb_mgr_->freememory_[i]);
            }
        }
    }
    cur_v_ = new_v;
}
