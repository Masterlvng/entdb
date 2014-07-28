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

Status MemoryMgr::Open(const string& filename, DataPool* dp,
                        uint64_t data_size, offset_t off_init)
{
    Status s;
    filename_ = filename;
    dp_ = dp;
    fb_mgr_ = new FMBMgr(filename, data_size, off_init);
    s = fb_mgr_->Open(filename_);
    if (!s.IsOK()) return s;
    std::map<offset_t, fm_block_t>::iterator it_fmb;
    for (it_fmb = fb_mgr_->map_fm_.begin(); it_fmb != fb_mgr_->map_fm_.end(); ++it_fmb)
    {
        //
        set_fm_.insert(it_fmb->second); 
    }
    return Status::OK();
}

Status MemoryMgr::Close()
{
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

    fm_block_t fbt; 
    fbt.offset = UINT64_MAX;
    fbt.size = *rsp_size;
    
    std::set<fm_block_t>::iterator it_set;
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
       s = fb_mgr_->UpdateBlock(it_set->offset, it_set->offset +*rsp_size, it_set->size - *rsp_size); 
       off = it_set->offset;
       fm_block_t fbt;
       fbt.offset = it_set->offset + *rsp_size;
       fbt.size = it_set->size - *rsp_size;

       set_fm_.erase(*it_set);
       set_fm_.insert(fbt);
    }
    else
    {
        //拿走整块空闲块
       s = fb_mgr_->DeleteBlock(it_set->offset);
       if (!s.IsOK()) return s;
       *rsp_size = it_set->size;
       off = it_set->offset;
       set_fm_.erase(it_set);
    }
    *off_out = off;
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
    
    fb_mgr_->AddBlock(off, size);
    std::map<uint64_t, fm_block_t>::iterator cur_it, next_it, prev_it;
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
            merged_prev = true; 
            
            fm_block_t fbt_deleted; 
            fbt_deleted.offset = prev_it->second.offset;
            fbt_deleted.size = prev_it->second.size;
            set_fm_.erase(set_fm_.find(fbt_deleted));

            fb_mgr_->DeleteBlock(cur_it->second.offset);
            fb_mgr_->UpdateBlock(prev_it->second.offset, prev_it->second.offset, 
                                            prev_it->second.size + cur_it->second.size);

            fbt_deleted.size += cur_it->second.size;
            set_fm_.insert(fbt_deleted);
       }
    }
    // 向后合并
    if (cur_it != fb_mgr_->map_fm_.end() &&
            ++next_it != fb_mgr_->map_fm_.end())
    {
       if (cur_it->second.offset + cur_it->second.size == 
                            next_it->second.offset) 
       {
            merged_next = true;
            fb_mgr_->DeleteBlock(next_it->second.offset);
            fb_mgr_->UpdateBlock(cur_it->second.offset, cur_it->second.offset,
                                            cur_it->second.size + next_it->second.size);
            
            fm_block_t fbt_deleted;
            fbt_deleted.offset = next_it->second.offset;
            fbt_deleted.size = next_it->second.size;
            set_fm_.erase(set_fm_.find(fbt_deleted));

            fbt_deleted.offset = cur_it->second.offset;
            fbt_deleted.size = cur_it->second.size;
            set_fm_.erase(set_fm_.find(fbt_deleted));

            fbt_deleted.size += next_it->second.size;
            set_fm_.insert(fbt_deleted);

       }
    }

    if (merged_next || merged_prev)
    {
       return Status::OK(); 
    }
    fb_mgr_->AddBlock(off, size);
    set_fm_.insert(fbt);
    return Status::OK();
    
}

uint64_t MemoryMgr::AlignSize(uint64_t req_size)
{
    return align_size(req_size, ALIGNMENT_DATA);
}
