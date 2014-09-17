/* -*- c++ -*- */
/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once
#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <fcntl.h>
#include <unistd.h>

#ifdef _POSIX_MAPPED_FILES
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <stdint.h>
#include <string.h>
#include "ralloc.h"

#ifdef __cplusplus

/**
 * Helper class to read data
 *
 * Class can read either from user given memory or from a file. On Linux
 * file reading wraps around the Posix functions for mapping a file into
 * the process's address space. Other OS may need different implementation.
 */
class memory_map
{
public:
   memory_map() :
      error(false),
      mode(memory_map::READ_MEM),
      cache_size(0),
      cache_mmap(NULL),
      cache_mmap_p(NULL)
   {
      mem_ctx = ralloc_context(NULL);
   }

   /* read from disk */
   int map(const char *path)
   {
#ifdef _POSIX_MAPPED_FILES
      struct stat stat_info;
      if (stat(path, &stat_info) != 0)
         return -1;

      mode = memory_map::READ_MAP;
      cache_size = stat_info.st_size;

      int fd = open(path, O_RDONLY);
      if (fd) {
         cache_mmap_p = cache_mmap = (char *)
            mmap(NULL, cache_size, PROT_READ, MAP_PRIVATE, fd, 0);
         close(fd);
         return (cache_mmap == MAP_FAILED) ? -1 : 0;
      }
#else
      /* Implementation for systems without mmap(). */
      FILE *in = fopen(path, "r");
      if (in) {
         fseek(in, 0, SEEK_END);
         cache_size = ftell(in);
         rewind(in);

         cache_mmap = ralloc_array(mem_ctx, char, cache_size);

         if (!cache_mmap)
            return -1;

         if (fread(cache_mmap, cache_size, 1, in) != 1) {
            ralloc_free(cache_mmap);
            cache_mmap = NULL;
         }
         cache_mmap_p = cache_mmap;
         fclose(in);

         return (cache_mmap == NULL) ? -1 : 0;
      }
#endif
      return -1;
   }

   /* read from memory */
   void map(const void *memory, size_t size)
   {
      cache_mmap_p = cache_mmap = (char *) memory;
      cache_size = size;
   }

   ~memory_map() {
#ifdef _POSIX_MAPPED_FILES
      if (cache_mmap && mode == READ_MAP) {
         munmap(cache_mmap, cache_size);
      }
#endif
      ralloc_free(mem_ctx);
   }

   /* move read pointer forward */
   inline void ffwd(int len)
   {
      cache_mmap_p += len;
   }

   inline void jump(unsigned pos)
   {
      cache_mmap_p = cache_mmap + pos;
   }

   /**
    * safety check to avoid reading over cache_size,
    * returns bool if it is safe to continue reading
    */
   bool safe_read(unsigned size)
   {
      if (position() + size > cache_size)
         error = true;
      return !error;
   }

   /* position of read pointer */
   inline uint32_t position()
   {
      return cache_mmap_p - cache_mmap;
   }

   inline char *read_string()
   {
      uint32_t len = read_uint32_t();

      /* NULL pointer is supported */
      if (len == 0)
         return NULL;

      /* don't read off the end of cache */
      /* TODO: Understand how this can happen and fix */
      if (len + position() > cache_size) {
         error = true;
         return NULL;
      }

      /* verify that last character is terminator */
      if (*(cache_mmap_p + len - 1) != '\0') {
         error = true;
         return NULL;
      }

      char *str = ralloc_array(mem_ctx, char, len);
      memcpy(str, cache_mmap_p, len);
      ffwd(len);
      return str;
   }

/**
 * read functions per type
 */
#define DECL_READER(type) type read_ ##type () {\
   if (!safe_read(sizeof(type)))\
      return 0;\
   ffwd(sizeof(type));\
   return *(type *) (cache_mmap_p - sizeof(type));\
}

   DECL_READER(int32_t);
   DECL_READER(int64_t);
   DECL_READER(uint8_t);
   DECL_READER(uint32_t);

   inline uint8_t read_bool()
   {
      return read_uint8_t();
   }

   inline void read(void *dst, size_t size)
   {
      if (!safe_read(size))
         return;
      memcpy(dst, cache_mmap_p, size);
      ffwd(size);
   }

   /* total size of mapped memory */
   inline int32_t size()
   {
      return cache_size;
   }

   inline bool errors()
   {
      return error;
   }

private:

   void *mem_ctx;

   /* if errors have occured during reading */
   bool error;

   /* specifies if we are reading mapped memory or user passed mem */
   enum read_mode {
      READ_MEM = 0,
      READ_MAP
   };

   int32_t mode;
   unsigned cache_size;
   char *cache_mmap;
   char *cache_mmap_p;
};
#endif /* ifdef __cplusplus */

#endif /* MEMORY_MAP_H */
