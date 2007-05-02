namespace QCA {
/*
Copyright (C) 1999-2004 The Botan Project. All rights reserved.

Redistribution and use in source and binary forms, for any use, with or without
modification, is permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions, and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions, and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTOR(S) BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*************************************************
* Memory Mapping Allocator Source File           *
* (C) 1999-2004 The Botan Project                *
*************************************************/

#ifndef _XOPEN_SOURCE
  #define _XOPEN_SOURCE 500
#endif

#ifndef _XOPEN_SOURCE_EXTENDED
  #define _XOPEN_SOURCE_EXTENDED 1
#endif

}
#include <botan/mmap_mem.h>
namespace QCA {
}
#include <cstring>
namespace QCA {
}
#include <sys/types.h>
namespace QCA {
}
#include <sys/mman.h>
namespace QCA {
}
#include <sys/stat.h>
namespace QCA {
}
#include <unistd.h>
namespace QCA {
}
#include <stdlib.h>
namespace QCA {
}
#include <fcntl.h>
namespace QCA {

#ifndef MAP_FAILED
   #define MAP_FAILED -1
#endif

namespace Botan {

/*************************************************
* Memory Map a File into Memory                  *
*************************************************/
void* MemoryMapping_Allocator::alloc_block(u32bit n) const
   {
   const std::string path = "/tmp/botan_XXXXXX";

   char* filepath = new char[path.length() + 1];
   std::strcpy(filepath, path.c_str());

   mode_t old_umask = umask(077);
   int fd = mkstemp(filepath);
   umask(old_umask);

   if(fd == -1)
      throw Exception("MemoryMapping_Allocator: Could not create file");
   if(unlink(filepath))
      throw Exception("MemoryMapping_Allocator: Could not unlink file " +
                      std::string(filepath));
   delete[] filepath;

   lseek(fd, n-1, SEEK_SET);
   if(write(fd, "\0", 1) != 1)
      throw Exception("MemoryMapping_Allocator: Could not write to file");
   void* ptr = mmap(0, n, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if(ptr == (void*)MAP_FAILED)
      throw Exception("MemoryMapping_Allocator: Could not map file");
   if(close(fd))
      throw Exception("MemoryMapping_Allocator: Could not close file");

   return ptr;
   }

/*************************************************
* Remove a Memory Mapping                        *
*************************************************/
void MemoryMapping_Allocator::dealloc_block(void* ptr, u32bit n) const
   {
   if(ptr == 0) return;

   const u32bit OVERWRITE_PASSES = 12;
   const byte PATTERNS[] = { 0x00, 0xFF, 0xAA, 0x55, 0x73, 0x8C, 0x5F, 0xA0,
                             0x6E, 0x91, 0x30, 0xCF, 0xD3, 0x2C, 0xAC, 0x53 };

   for(u32bit j = 0; j != OVERWRITE_PASSES; j++)
      {
      std::memset(ptr, PATTERNS[j % sizeof(PATTERNS)], n);
      if(msync(ptr, n, MS_SYNC))
         throw Exception("MemoryMapping_Allocator: Sync operation failed");
      }
   std::memset(ptr, 0, n);
   if(msync(ptr, n, MS_SYNC))
      throw Exception("MemoryMapping_Allocator: Sync operation failed");

   if(munmap(ptr, n))
      throw Exception("MemoryMapping_Allocator: Could not unmap file");
   }

}
}
