/* -*- c -*- */
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

#include <sys/stat.h>
#include "shader_cache.h"
#include "prog_diskcache.h"
#include "glsl_parser_extras.h"

#ifndef _WIN32
#include <dirent.h>

struct dir_entry_t
{
   char *path;
   struct stat info;
};


static int
sort_by_access_time(const void *_a, const void *_b)
{
   /* Compare access time of 2 entries */
   struct dir_entry_t *a = (struct dir_entry_t *) _a;
   struct dir_entry_t *b = (struct dir_entry_t *) _b;

   if (a->info.st_atime > b->info.st_atime)
      return 1;
   return -1;
}


static int
valid_cache_entry(const struct dirent *entry)
{
   /* Only regular files are possible valid cache entries. */
   if (entry->d_type == DT_REG)
      return 1;
   return 0;
}


/**
 * Cache size management. If cache size exceeds max_cache_size,
 * entries are sorted by access time and oldest entries deleted
 * until we fit.
 */
static void
manage_cache_size(const char *path, const unsigned max_cache_size)
{
   if (max_cache_size == 0xFFFFFFFF) {
      /* cache size of -1 means don't limit the size */
      return;
   }

   struct dirent **entries;
   int n = scandir(path, &entries, valid_cache_entry, NULL);

   if (n <= 0)
      return;

   struct dir_entry_t *cache = NULL;
   unsigned cache_size = 0;
   unsigned cache_entries = 0;

   void *mem_ctx = ralloc_context(NULL);

   cache = ralloc_array(mem_ctx, struct dir_entry_t, n);

   /* Construct entries with path and access information + calculate
    * total size used by entries.
    */
   while (n--) {
      cache[cache_entries].path =
         ralloc_asprintf(mem_ctx, "%s/%s", path, entries[n]->d_name);
      stat(cache[cache_entries].path, &cache[cache_entries].info);

      cache_size += cache[cache_entries].info.st_size;

      cache_entries++;
      free(entries[n]);
   }
   free(entries);

   /* No need to manage if we fit the max size. */
   if (cache_size < max_cache_size)
      goto free_allocated_memory;

   /* Sort oldest first so we can 'delete until cache size less than max'. */
   qsort(cache, cache_entries, sizeof(struct dir_entry_t), sort_by_access_time);

   unsigned i = 0;
   while (cache_size > max_cache_size && i < cache_entries) {
      unlink(cache[i].path);
      cache_size -= cache[i].info.st_size;
      i++;
   }

free_allocated_memory:

   ralloc_free(mem_ctx);
}
#endif


static int
mesa_mkdir_cache(const char *path)
{
   char *copy = _mesa_strdup(path);
   char *dir = strtok(copy, "/");
   char *current = ralloc_strdup(NULL, "/");
   int result = 0;

   /* As example loop iterates and calls mkdir for each token
    * separated by '/' in "/home/yogsothoth/.cache/mesa".
    */
   while (dir) {
      ralloc_strcat(&current, dir);

      result = _mesa_mkdir(current);

      if (result != 0 && result != EEXIST)
         return -1;

      ralloc_strcat(&current, "/");
      dir = strtok(NULL, "/");
   }

   ralloc_free(current);
   free(copy);

   return 0;
}


int
mesa_program_diskcache_init(struct gl_context *ctx)
{
   const char *tmp = "/tmp", *cache_root = NULL;
   int result = 0;

   if (ctx->Const.MaxShaderCacheSize == 0) {
      // if 0 (default) then no cache will be active
      ctx->BinaryProgramCacheActive = false;
      return -1;
   }

   cache_root = _mesa_getenv("XDG_CACHE_DIR");
   if (!cache_root)
      cache_root = _mesa_getenv("HOME");
   if (!cache_root)
      cache_root = tmp;

   asprintf(&ctx->BinaryProgramCachePath, "%s/.cache/mesa/programs", cache_root);

   struct stat stat_info;
   if (stat(ctx->BinaryProgramCachePath, &stat_info) != 0)
      result = mesa_mkdir_cache(ctx->BinaryProgramCachePath);
#ifndef _WIN32
   else
      manage_cache_size(ctx->BinaryProgramCachePath, ctx->Const.MaxShaderCacheSize);
#endif

   if (result == 0)
      ctx->BinaryProgramCacheActive = true;
   else
      ctx->BinaryProgramCacheActive = false;

   return result;
}


static uint32_t
checksum(const char *src)
{
   uint32_t sum = _mesa_str_checksum(src);
   unsigned i;

   /* Add some sugar on top (borrowed from brw_state_cache). This is meant
    * to catch cache collisions when there are only small changes in the
    * source such as mat3 -> mat4 in a type for example.
    */
   for (i = 0; i < strlen(src); i++) {
      sum ^= (uint32_t) src[i];
      sum = (sum << 5) | (sum >> 27);
   }

   return sum;
}


/**
 * Attempt to generate unique key for a gl_shader_program.
 * TODO - this should be stronger and be based on some of the
 * gl_shader_program content, not just sources.
 */
static char *
generate_key(struct gl_shader_program *prog)
{
   char *key = ralloc_strdup(prog, "");
   for (unsigned i = 0; i < prog->NumShaders; i++) {

      /* No source, no key. */
      if (!prog->Shaders[i]->Source)
         return NULL;

      /* At least some content required. */
      if (strcmp(prog->Shaders[i]->Source, "") == 0)
         return NULL;

      uint64_t sum = checksum(prog->Shaders[i]->Source);

      char tmp[32];
      _mesa_snprintf(tmp, 32, "%lu", sum);

      ralloc_strcat(&key, tmp);
   }

   /* Key needs to have enough content. */
   if (strlen(key) < 7) {
      ralloc_free(key);
      key = NULL;
   }

   return key;
}


/**
 * Cache gl_shader_program to disk
 */
int
mesa_program_diskcache_cache(struct gl_context *ctx, struct gl_shader_program *prog)
{
   assert(ctx);
   int result = -1;
   struct stat stat_info;
   char *key;

   key = generate_key(prog);

   if (!key)
      return -1;

   /* Glassy vs. Opaque compiled shaders */
   if (_mesa_use_glass(ctx))
       ralloc_strcat(&key, ".g");

   char *shader_path =
      ralloc_asprintf(NULL, "%s/%s.bin", ctx->BinaryProgramCachePath, key);

   /* Collision, do not attempt to overwrite. */
   if (stat(shader_path, &stat_info) == 0)
      goto cache_epilogue;

   size_t size = 0;
   char *data = mesa_program_serialize(ctx, prog, &size);

   if (!data)
      goto cache_epilogue;

   FILE *out = fopen(shader_path, "w+");

   if (!out)
      goto cache_epilogue;

   fwrite(data, size, 1, out);
   fclose(out);
   free(data);
   result = 0;

cache_epilogue:
   ralloc_free(shader_path);
   ralloc_free(key);
   return result;
}

bool
supported_by_program_cache(struct gl_shader_program *prog, bool is_write)
{
   /* No geometry shader support. */
   if (prog->_LinkedShaders[MESA_SHADER_GEOMETRY])
      return false;

   /* No uniform block support. */
   if (prog->NumUniformBlocks > 0)
      return false;

   /* No transform feedback support. */
   if (prog->TransformFeedback.NumVarying > 0)
           return false;

   /* These more expensive checks should only be run when generating
    * the cache entry
    */
   if (is_write)
   {
      /* Uniform structs are not working */
      if (prog->UniformStorage) {
         for (unsigned i = 0; i < prog->NumUserUniformStorage; i++) {
            if (strchr(prog->UniformStorage[i].name, '.')) {
               /* The uniforms struct fields have already been broken
                * down into unique variables, we have to inspect their
                * names and kick back since these aren't working.
                */
               return false;
            }
         }
      }

      /* This is nasty!  Short term solution for correctness! */
      for (unsigned i = 0; i < prog->NumShaders; i++) {
         if (prog->Shaders[i] && prog->Shaders[i]->Source) {
            /* This opcode is not supported by MesaIR (yet?) */
            if (strstr(prog->Shaders[i]->Source, "textureQueryLevels"))
               return false;
         }
      }
   }

   return true;
}

/**
 * Fill gl_shader_program from cache if found
 */
int
mesa_program_diskcache_find(struct gl_context *ctx, struct gl_shader_program *prog)
{
   assert(ctx);
   int result = 0;
   char *key;

   /* Do not use diskcache when program relinks. Relinking is not
    * currently supported due to the way how cache key gets generated.
    * We would need to modify key generation to take account hashtables
    * and possible other data in gl_shader_program to catch changes in
    * the data used as input for linker (resulting in a different program
    * with same sources).
    */
   if (prog->_Linked)
      return -1;

   /* This is heavier than we'd like, but string compares are
    * insufficient for this caching.  It depends on state as well.
    */
   if (!supported_by_program_cache(prog, false /* is_write */))
      return -1;

   key = generate_key(prog);

   if (!key)
      return -1;

   /* Glassy vs. Opaque compiled shaders */
   if (_mesa_use_glass(ctx))
       ralloc_strcat(&key, ".g");


   char *shader_path =
      ralloc_asprintf(NULL, "%s/%s.bin", ctx->BinaryProgramCachePath, key);

   result = mesa_program_load(ctx, prog, shader_path);

   // TODO:  ensure we did not get a false cache hit by comparing the
   // incoming strings with what we just deserialized

   ralloc_free(shader_path);
   ralloc_free(key);

   return result;
}



/* The following functions are shader verions of program caching functions
 * They could be moved to another file, or merged with the program version
 * since several have line of code in common.  This hasn't been done yet to
 * prevent premature optimization.
 */

bool
supported_by_shader_cache(struct gl_shader *shader, bool is_write)
{
   /* No geometry shader support. */
   // how hard to add?
   if (shader->Stage == MESA_SHADER_GEOMETRY)
      return false;

   /* No uniform block support. */
   if (shader->NumUniformBlocks > 0)
      return false;

   return true;
}


static int
mesa_mkdir_shader_cache(const char *path)
{
   char *copy = _mesa_strdup(path);
   char *dir = strtok(copy, "/");
   char *current = ralloc_strdup(NULL, "/");
   int result = 0;

   /* As example loop iterates and calls mkdir for each token
    * separated by '/' in "/home/yogsothoth/.cache/mesa".
    */
   while (dir) {
      ralloc_strcat(&current, dir);

      result = _mesa_mkdir(current);

      if (result != 0 && result != EEXIST)
         return -1;

      ralloc_strcat(&current, "/");
      dir = strtok(NULL, "/");
   }

   ralloc_free(current);
   free(copy);

   return 0;
}



/* This is based on mesa_program_diskcache_init,
 * would be good to merge them at some point.
 */

int
mesa_shader_diskcache_init(struct gl_context *ctx)
{
   const char *tmp = "/tmp", *cache_root = NULL;
   int result = 0;

   if (ctx->Const.MaxShaderCacheSize == 0) {
      // if 0 (default) then no cache will be active
      ctx->BinaryShaderCacheActive = false;
      return -1;
   }

   cache_root = _mesa_getenv("XDG_CACHE_DIR");
   if (!cache_root)
      cache_root = _mesa_getenv("HOME");
   if (!cache_root)
      cache_root = tmp;

   asprintf(&ctx->BinaryShaderCachePath, "%s/.cache/mesa/shaders", cache_root);

   struct stat stat_info;
   if (stat(ctx->BinaryShaderCachePath, &stat_info) != 0)
      result = mesa_mkdir_shader_cache(ctx->BinaryShaderCachePath);
#ifndef _WIN32
   else
      manage_cache_size(ctx->BinaryShaderCachePath, ctx->Const.MaxShaderCacheSize);
#endif

   if (result == 0)
      ctx->BinaryShaderCacheActive = true;
   else
      ctx->BinaryShaderCacheActive = false;

   return result;
}


static uint32_t
shader_checksum(const char *src)
{
   uint32_t sum = _mesa_str_checksum(src);
   unsigned i;

   /* Add some sugar on top (borrowed from brw_state_cache). This is meant
    * to catch cache collisions when there are only small changes in the
    * source such as mat3 -> mat4 in a type for example.
    */
   for (i = 0; i < strlen(src); i++) {
      sum ^= (uint32_t) src[i];
      sum = (sum << 5) | (sum >> 27);
   }

   return sum;
}


/* This is based on generate_key(prog), would
 * be good to merge them at some point.
 */
static char *
generate_shader_key(struct gl_shader *shader)
{
   char *key = ralloc_strdup(shader, "");

   /* No source, no key. */
   if (shader->Source == NULL)
      return NULL;

   /* At least some content required. */
   if (strcmp(shader->Source, "") == 0)
      return NULL;

   uint64_t sum = shader_checksum(shader->Source);

   char tmp[32];
   _mesa_snprintf(tmp, 32, "%lu", sum);

   ralloc_strcat(&key, tmp);

   /* Key needs to have enough content. */
   if (strlen(key) < 7) {
      ralloc_free(key);
      key = NULL;
   }

   return key;
}

/**
 * Deserialize gl_shader structure
 */
struct gl_shader *
mesa_shader_deserialize(struct gl_context *ctx, struct gl_shader *shader,
                        const char* path)
{
   return read_single_shader(ctx, shader, path);
}


int
mesa_shader_load(struct gl_context *ctx, struct gl_shader *shader,
                 const char *path)
{

   struct gl_shader *result = mesa_shader_deserialize(ctx, shader, path);

   if (result)
     return 0;
   else
     return MESA_SHADER_DESERIALIZE_READ_ERROR;
}

/*
 * This is based on mesa_program_diskcache_cache, would be good to
 * merge them at some point.
 */
int
mesa_shader_diskcache_cache(struct gl_context *ctx, struct gl_shader *shader)
{
   assert(ctx);
   int result = -1;
   struct stat stat_info;
   char *key;
   size_t size = 0;

   key = generate_shader_key(shader);

   if (!key)
      return -1;

   /* Glassy vs. Opaque compiled shaders */
   if (_mesa_use_glass(ctx))
      ralloc_strcat(&key, ".g");

   char *shader_path =
      ralloc_asprintf(NULL, "%s/%s.bin", ctx->BinaryShaderCachePath, key);

   char *data = NULL;
   FILE *out = NULL;

   /* Collision, do not attempt to overwrite. */
   if (stat(shader_path, &stat_info) == 0)
      goto cache_epilogue;

   data = mesa_shader_serialize(ctx, shader, &size, true /* shader_only */);

   if (!data)
      goto cache_epilogue;

   out = fopen(shader_path, "w+");

   if (!out)
      goto cache_epilogue;

   fwrite(data, size, 1, out);
   fclose(out);
   free(data);
   result = 0;

cache_epilogue:
   ralloc_free(shader_path);
   ralloc_free(key);
   return result;
}



/* This is based on mesa_program_diskcache_find, would be good to
 * merge them at some point.
 */
int
mesa_shader_diskcache_find(struct gl_context *ctx, struct gl_shader *shader)
{
   int result = 0;
   char *key;

   /* Don't lookup if already compiled. */
   if (shader->CompileStatus)
      return -1;

   /* This is heavier than we'd like, but string compares are
    * insufficient for this caching.  It depends on state as well.
    */
   if (!supported_by_shader_cache(shader, false /* is_write */))
      return -1;

   key = generate_shader_key(shader);

   if (!key)
      return -1;

   /* Glassy vs. Opaque compiled shaders */
   if (_mesa_use_glass(ctx))
      ralloc_strcat(&key, ".g");

   char *shader_path =
      ralloc_asprintf(NULL, "%s/%s.bin", ctx->BinaryShaderCachePath, key);

   result = mesa_shader_load(ctx, shader, shader_path);

   // TODO:  ensure we did not get a false cache hit by comparing the
   // incoming string with what we just deserialized

   ralloc_free(shader_path);
   ralloc_free(key);

   return result;
}
