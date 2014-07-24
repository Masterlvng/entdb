#ifndef SKIP_LIST
#define SKIP_LIST

#include <stdlib.h>
#include <assert.h>
#include "random.h"

namespace util{
    typedef int level_t;
    const level_t MAX_LEVEL = 16;

    template <typename TKey, typename TVal>
    class SkipList 
    {
        public:
            SkipList(): rand_(0x01bdfe8a)
            {
                init_();
            }

            ~SkipList()
            {
                Release();
            }

            bool Search(const TKey& key, TVal& val)
            {
                node_t* prev[MAX_LEVEL];
                node_t* n;
                return hasNode_(key, val, &n, prev);
            }

            bool Insert(const TKey& key, TVal& val)
            {
                node_t* prev[MAX_LEVEL];
                node_t* n;
                if(!hasNode_(key, val, &n, prev))
                {
                    level_t level = randomLevel_();
                    if(level > list_->level_)
                    {
                        //level = ++list_->level_;
                        int i;
                        for(i = list_->level_; i<= level; ++i)
                            prev[i] = list_->header_;
                        list_->level_ = level;
                    }
                    node_t* newNode = newNode_(level);
                    newNode->key_ = key;
                    newNode->val_ = val;
                    int i = level;
                    for(;i >= 0; --i)
                    {
                        node_t* tempN = prev[i];
                        newNode->forward_[i] = tempN->forward_[i];
                        tempN->forward_[i] = newNode;
                    }
                    return true;
                }
                // update Node
                n->val_ = val;
                return false;
            }

            bool Delete(const TKey& key, TVal& val)
            {
                node_t* prev[MAX_LEVEL];
                node_t* n;
                if (hasNode_(key, val, &n, prev))
                {
                    int i;
                    for(i = 0; i <= list_->level_; ++i)
                    {
                        if(prev[i]->forward_[i] != n)
                        {
                            break;
                        }
                        prev[i]->forward_[i] = n->forward_[i];
                    }

                    //free(n->mem);
                    //delete n;
                    freeNode_(n);
                    while(list_->level_ > 0
                            && list_->header_->forward_[list_->level_] == nil_)
                    {
                        --list_->level_;
                    }
                }
                return true;
            }

            void Release()
            {
                node_t* n = list_->header_;
                while(n != nil_)
                {
                    node_t* tempN = n->forward_[0];
                    /*
                    char *mem = n->mem;
                    n->~node_t();
                    free(mem);
                    */
                    freeNode_(n);
                    n = tempN;
                }
                /*
                char *mem = n->mem;
                n->~node_t();
                free(mem);
                */
                freeNode_(n);
                free(list_);
            }
            
        private:
            class node_t
            {
                public:
                node_t(){}
                ~node_t(){}
                TKey key_;
                TVal val_;
                level_t level_;
                char* mem;
                node_t* forward_[1];
            };

            struct list_t
            {
                level_t level_;
                node_t* header_;
            };

            list_t* list_;
            node_t* nil_;
            Random rand_;
        private:
            void init_()
            {
                
                nil_ = newNode_(1);
                nil_->level_ = 0;
                nil_->forward_[0] = NULL;
                list_ = (list_t*)malloc(sizeof(list_t));
                list_->level_ = 0;
                list_->header_ = newNode_(MAX_LEVEL);
                list_->header_->level_ = 0;
                int i;
                for(i = 0; i < MAX_LEVEL; ++i)
                {
                    list_->header_->forward_[i] = nil_;
                }
            }

            node_t* newNode_(const level_t level)
            {
                char * mem = (char*)malloc(sizeof(node_t) + level*sizeof(node_t*));
                //将来替换为更牛B的内存分配器

                node_t* node = new (mem) node_t();
                node->level_ = level;
                node->mem = mem;
                return node;
            }

            void freeNode_(node_t* n)
            {
                char* mem = n->mem;
                n->~node_t();
                free(mem);
            }

            level_t randomLevel_()
            {
                static const unsigned int kBranching = 4;
                level_t height = 1;
                while(height < MAX_LEVEL && ((rand_.Next() % kBranching) == 0))
                {
                   height++; 
                }
                assert(height > 0);
                assert(height <= MAX_LEVEL);
                return height;
            }

            bool hasNode_(const TKey& key, TVal& val, node_t** node, node_t* prev[MAX_LEVEL])
            {
                node_t* n = list_->header_;
                int i = list_->level_;
                for(; i>=0; --i)
                {
                    while(n->forward_[i]->forward_[0] != NULL && n->forward_[i]->key_ < key)
                    {
                        n = n->forward_[i];
                    }
                    prev[i] = n;
                }
                if (n->forward_[0]->key_ != key)
                {
                    return false;
                }
                else
                {
                    val = n->forward_[0]->val_;
                    *node = n->forward_[0];
                    return true;
                }
            }
    };
};

#endif
