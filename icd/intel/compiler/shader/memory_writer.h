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
#ifndef MEMORY_WRITER_H
#define MEMORY_WRITER_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "main/hash_table.h"

#ifdef __cplusplus
/**
 * Helper class for writing data to memory
 *
 * This class maintains a dynamically-sized memory buffer and allows
 * for data to be efficiently appended to it with automatic resizing.
 */
class memory_writer
{
public:
   memory_writer() :
      memory(NULL),
      curr_size(0),
      pos(0),
      unique_id_counter(0)
   {
      data_hash = _mesa_hash_table_create(0, int_equal);
      hash_value = _mesa_hash_data(this, sizeof(memory_writer));
   }

   ~memory_writer()
   {
      free(memory);
      _mesa_hash_table_destroy(data_hash, NULL);
   }

   /* user wants to claim the memory */
   char *release_memory(size_t *size)
   {
      /* final realloc to free allocated but unused memory */
      char *result = (char *) realloc(memory, pos);
      *size = pos;
      memory = NULL;
      curr_size = 0;
      pos = 0;
      return result;
   }

/**
 * write functions per type
 */
#define DECL_WRITER(type) void write_ ##type (const type data) {\
   write(&data, sizeof(type));\
}

   DECL_WRITER(int32_t);
   DECL_WRITER(int64_t);
   DECL_WRITER(uint8_t);
   DECL_WRITER(uint32_t);

   void write_bool(bool data)
   {
      uint8_t val = data;
      write_uint8_t(val);
   }

   /* write function that reallocates more memory if required */
   void write(const void *data, int size)
   {
      if (!memory || pos > (curr_size - size))
         if (!grow(size)) {
            assert(!"Out of memory while serializing a shader");
            return;
         }

      memcpy(memory + pos, data, size);

      pos += size;
   }

   void overwrite(const void *data, int size, int offset)
   {
      if (offset < 0 || offset + size > pos) {
         assert(!"Attempt to write out of bounds while serializing a shader");
         return;
      }

      memcpy(memory + offset, data, size);
   }

   /* length is written to make reading safe, we write len + 1 to be
    * able to make distinction between "" and NULL
    */
   void write_string(const char *str)
   {
      uint32_t len = str ? strlen(str) + 1 : 0;
      write_uint32_t(len);

      /* serialize string + terminator for more convinient parsing. */
      if (str)
         write(str, len);
   }

   unsigned position()
   {
      return pos;
   }

   /**
    * Convert the given pointer into a small integer unique ID.  In other
    * words, if make_unique_id() has previously been called with this pointer,
    * return the same ID that was returned last time.  If this is the first
    * call to make_unique_id() with this pointer, return a fresh ID.
    *
    * Return value is true if the pointer has been seen before, false
    * otherwise.
    */
   bool make_unique_id(const void *ptr, uint32_t *id_out)
   {
      hash_entry *entry =
         _mesa_hash_table_search(data_hash, _mesa_hash_pointer(ptr), ptr);
      if (entry != NULL) {
         *id_out = (uint32_t) (intptr_t) entry->data;
         return true;
      } else {
         /* Note: hashtable uses 0 to represent "entry not found" so our
          * unique ID's need to start at 1.  Hence, preincrement
          * unique_id_counter.
          */
         *id_out = ++this->unique_id_counter;
         _mesa_hash_table_insert(data_hash, _mesa_hash_pointer(ptr), ptr,
                                 (void *) (intptr_t) *id_out);
         return false;
      }
   }

private:

   /* reallocate more memory */
   bool grow(int size)
   {
      unsigned new_size = 2 * (curr_size + size);
      char *more_mem = (char *) realloc(memory, new_size);
      if (more_mem == NULL) {
         free(memory);
         memory = NULL;
         return false;
      } else {
         memory = more_mem;
         curr_size = new_size;
         return true;
      }
   }

   /* allocated memory */
   char *memory;

   /* current size of the whole allocation */
   int curr_size;

   /* write position / size of the data written */
   int pos;

   /* this hash can be used to refer to data already written
    * to skip sequential writes of the same data
    */
   struct hash_table *data_hash;
   uint32_t hash_value;
   unsigned unique_id_counter;

   static bool int_equal(const void *a, const void *b)
   {
      return a == b;
   }

};

#endif /* ifdef __cplusplus */

#endif /* MEMORY_WRITER_H */
