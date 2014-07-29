#ifndef ENTDB_VERSION_H
#define ENTDB_VERSION_H

#include <stdint.h>
#include <string>
#include "status.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>

typedef uint64_t version_t;

enum
{
    INDEX,
    FM,
    DATA
};

namespace entdb
{
    class Version
    {
        public:
            Version();
            ~Version();
            Status Open(const std::string& filename);
            version_t CurVersion(int target);
            Status IncVersion(int target);

        private:
            int fd_;
            char* data_;

            Status OpenFile(const std::string& filename);
            Status CreateFile(const std::string& filename);
            Status CloseFile();
            Status Sync();
    
   };
};

#endif
