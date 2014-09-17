/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014  LunarG, Inc.   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdbool.h>
#include "c11/threads.h"
#include "main/compiler.h"
#include "main/simple_list.h"
#include "threadpool.h"

enum _mesa_threadpool_control {
   MESA_THREADPOOL_NORMAL,    /* threads wait when there is no task */
   MESA_THREADPOOL_QUIT,      /* threads quit when there is no task */
   MESA_THREADPOOL_QUIT_NOW   /* threads quit as soon as possible */
};

enum _mesa_threadpool_task_state {
   MESA_THREADPOOL_TASK_PENDING,    /* task is on the pending list */
   MESA_THREADPOOL_TASK_ACTIVE,     /* task is being worked on */
   MESA_THREADPOOL_TASK_COMPLETED,  /* task has been completed */
   MESA_THREADPOOL_TASK_CANCELLED   /* task is cancelled */
};

struct _mesa_threadpool_task {
   /* these are protected by the pool's mutex */
   struct simple_node link; /* must be the first */
   enum _mesa_threadpool_task_state state;
   cnd_t completed;

   void (*func)(void *);
   void *data;
};

struct _mesa_threadpool {
   mtx_t mutex;
   int refcnt;
   bool shutdown;

   enum _mesa_threadpool_control thread_control;
   thrd_t *threads;
   int num_threads, max_threads;
   int idle_threads; /* number of threads that are idle */
   cnd_t thread_wakeup;
   cnd_t thread_joined;

   struct simple_node pending_tasks;
   int num_pending_tasks;
   int num_tasks;
};

static struct _mesa_threadpool_task *
task_create(void)
{
   struct _mesa_threadpool_task *task;

   task = malloc(sizeof(*task));
   if (!task)
      return NULL;

   if (cnd_init(&task->completed)) {
      free(task);
      return NULL;
   }

   task->state = MESA_THREADPOOL_TASK_PENDING;

   return task;
}

static void
task_destroy(struct _mesa_threadpool_task *task)
{
   cnd_destroy(&task->completed);
   free(task);
}

static void
pool_exec_task(struct _mesa_threadpool *pool,
               struct _mesa_threadpool_task *task)
{
   assert(task->state == MESA_THREADPOOL_TASK_PENDING);

   remove_from_list(&task->link);
   pool->num_pending_tasks--;

   task->state = MESA_THREADPOOL_TASK_ACTIVE;

   /* do the work! */
   mtx_unlock(&pool->mutex);
   task->func(task->data);
   mtx_lock(&pool->mutex);

   task->state = MESA_THREADPOOL_TASK_COMPLETED;
}

static int
_mesa_threadpool_worker(void *arg)
{
   struct _mesa_threadpool *pool = (struct _mesa_threadpool *) arg;

   mtx_lock(&pool->mutex);

   while (true) {
      struct _mesa_threadpool_task *task;

      /* wait until there are tasks */
      while (is_empty_list(&pool->pending_tasks) &&
             pool->thread_control == MESA_THREADPOOL_NORMAL) {
         pool->idle_threads++;
         cnd_wait(&pool->thread_wakeup, &pool->mutex);
         pool->idle_threads--;
      }

      if (pool->thread_control != MESA_THREADPOOL_NORMAL) {
         if (pool->thread_control == MESA_THREADPOOL_QUIT_NOW ||
             is_empty_list(&pool->pending_tasks))
            break;
      }

      assert(!is_empty_list(&pool->pending_tasks));
      task = (struct _mesa_threadpool_task *)
         first_elem(&pool->pending_tasks);

      pool_exec_task(pool, task);
      cnd_signal(&task->completed);
   }

   mtx_unlock(&pool->mutex);

   return 0;
}

/**
 * Queue a new task.
 */
struct _mesa_threadpool_task *
_mesa_threadpool_queue_task(struct _mesa_threadpool *pool,
                            void (*func)(void *), void *data)
{
   struct _mesa_threadpool_task *task;

   task = task_create();
   if (!task)
      return NULL;

   task->func = func;
   task->data = data;

   mtx_lock(&pool->mutex);

   if (unlikely(pool->shutdown)) {
      mtx_unlock(&pool->mutex);
      free(task);
      return NULL;
   }

   /* someone is joining with the threads */
   while (unlikely(pool->thread_control != MESA_THREADPOOL_NORMAL))
      cnd_wait(&pool->thread_joined, &pool->mutex);

   /* spawn threads as needed */
   if (pool->idle_threads <= pool->num_pending_tasks &&
       pool->num_threads < pool->max_threads) {
      int err;

      err = thrd_create(&pool->threads[pool->num_threads],
                        _mesa_threadpool_worker, (void *) pool);
      if (!err)
         pool->num_threads++;

      if (!pool->num_threads) {
         mtx_unlock(&pool->mutex);
         task_destroy(task);
         return NULL;
      }
   }

   insert_at_tail(&pool->pending_tasks, &task->link);
   pool->num_tasks++;
   pool->num_pending_tasks++;
   cnd_signal(&pool->thread_wakeup);

   mtx_unlock(&pool->mutex);

   return task;
}

/**
 * Wait and destroy the tasks.
 */
static bool
pool_wait_tasks(struct _mesa_threadpool *pool,
                struct _mesa_threadpool_task **tasks,
                int num_tasks)
{
   bool all_completed = true;
   int i;

   for (i = 0; i < num_tasks; i++) {
      struct _mesa_threadpool_task *task = tasks[i];

      while (task->state != MESA_THREADPOOL_TASK_COMPLETED &&
             task->state != MESA_THREADPOOL_TASK_CANCELLED)
         cnd_wait(&task->completed, &pool->mutex);

      if (task->state != MESA_THREADPOOL_TASK_COMPLETED)
         all_completed = false;

      task_destroy(task);
   }

   pool->num_tasks -= num_tasks;

   return all_completed;
}

/**
 * Wait for \p tasks to complete, and destroy them.  If some of \p tasks
 * cannot not be completed, return false.
 *
 * This function can be called from within the worker threads.
 */
bool
_mesa_threadpool_complete_tasks(struct _mesa_threadpool *pool,
                                struct _mesa_threadpool_task **tasks,
                                int num_tasks)
{
   bool prioritized = false, completed;
   int i;

   mtx_lock(&pool->mutex);

   /* we need to do something about tasks that are pending */
   for (i = 0; i < num_tasks; i++) {
      struct _mesa_threadpool_task *task = tasks[i];

      if (task->state != MESA_THREADPOOL_TASK_PENDING)
         continue;

      /* move them to the head so that they are executed next */
      if (!prioritized) {
         int j;

         for (j = i + 1; j < num_tasks; j++) {
            if (task->state == MESA_THREADPOOL_TASK_PENDING)
               move_to_head(&pool->pending_tasks, &task->link);
         }
         prioritized = true;
      }

      /*
       * Execute right away for we would have to wait for the worker threads
       * otherwise, which is no faster.  More importantly, when this is called
       * from within worker threads, there may be no idle thread available to
       * execute them.
       */
      pool_exec_task(pool, task);
   }

   completed = pool_wait_tasks(pool, tasks, num_tasks);

   mtx_unlock(&pool->mutex);

   return completed;
}

/**
 * This is equivalent to calling \p _mesa_threadpool_complete_tasks with one
 * task.
 */
bool
_mesa_threadpool_complete_task(struct _mesa_threadpool *pool,
                               struct _mesa_threadpool_task *task)
{
   bool completed;

   mtx_lock(&pool->mutex);

   if (task->state == MESA_THREADPOOL_TASK_PENDING)
      pool_exec_task(pool, task);

   completed = pool_wait_tasks(pool, &task, 1);

   mtx_unlock(&pool->mutex);

   return completed;
}

static void
pool_cancel_pending_tasks(struct _mesa_threadpool *pool)
{
   struct simple_node *node, *temp;

   if (is_empty_list(&pool->pending_tasks))
      return;

   foreach_s(node, temp, &pool->pending_tasks) {
      struct _mesa_threadpool_task *task =
         (struct _mesa_threadpool_task *) node;

      remove_from_list(&task->link);
      task->state = MESA_THREADPOOL_TASK_CANCELLED;

      /* in case some thread is already waiting */
      cnd_signal(&task->completed);
   }

   pool->num_pending_tasks = 0;
}

static void
pool_join_threads(struct _mesa_threadpool *pool, bool graceful)
{
   int joined_threads = 0;

   if (!pool->num_threads)
      return;

   pool->thread_control = (graceful) ?
      MESA_THREADPOOL_QUIT : MESA_THREADPOOL_QUIT_NOW;

   while (joined_threads < pool->num_threads) {
      int i = joined_threads, num_threads = pool->num_threads;

      cnd_broadcast(&pool->thread_wakeup);
      mtx_unlock(&pool->mutex);
      while (i < num_threads)
         thrd_join(pool->threads[i++], NULL);
      mtx_lock(&pool->mutex);

      joined_threads = num_threads;
   }

   pool->thread_control = MESA_THREADPOOL_NORMAL;
   pool->num_threads = 0;
   assert(!pool->idle_threads);
}

/**
 * Join with all pool threads.  When \p graceful is true, wait for the pending
 * tasks to be completed.
 */
void
_mesa_threadpool_join(struct _mesa_threadpool *pool, bool graceful)
{
   mtx_lock(&pool->mutex);

   /* someone is already joining with the threads */
   while (unlikely(pool->thread_control != MESA_THREADPOOL_NORMAL))
      cnd_wait(&pool->thread_joined, &pool->mutex);

   if (pool->num_threads) {
      pool_join_threads(pool, graceful);
      /* wake up whoever is waiting */
      cnd_broadcast(&pool->thread_joined);
   }

   if (!graceful)
      pool_cancel_pending_tasks(pool);

   assert(pool->num_threads == 0);
   assert(is_empty_list(&pool->pending_tasks) && !pool->num_pending_tasks);

   mtx_unlock(&pool->mutex);
}

/**
 * After this call, no task can be queued.
 */
static void
_mesa_threadpool_set_shutdown(struct _mesa_threadpool *pool)
{
   mtx_lock(&pool->mutex);
   pool->shutdown = true;
   mtx_unlock(&pool->mutex);
}

/**
 * Decrease the reference count.  Destroy \p pool when the reference count
 * reaches zero.
 */
void
_mesa_threadpool_unref(struct _mesa_threadpool *pool)
{
   bool destroy = false;

   mtx_lock(&pool->mutex);
   pool->refcnt--;
   destroy = (pool->refcnt == 0);
   mtx_unlock(&pool->mutex);

   if (destroy) {
      _mesa_threadpool_join(pool, false);

      if (pool->num_tasks) {
         fprintf(stderr, "thread pool destroyed with %d tasks\n",
               pool->num_tasks);
      }

      free(pool->threads);
      cnd_destroy(&pool->thread_joined);
      cnd_destroy(&pool->thread_wakeup);
      mtx_destroy(&pool->mutex);
      free(pool);
   }
}

/**
 * Increase the reference count.
 */
struct _mesa_threadpool *
_mesa_threadpool_ref(struct _mesa_threadpool *pool)
{
   mtx_lock(&pool->mutex);
   pool->refcnt++;
   mtx_unlock(&pool->mutex);

   return pool;
}

/**
 * Create a thread pool.  As threads are spawned as needed, this is
 * inexpensive.
 */
struct _mesa_threadpool *
_mesa_threadpool_create(int max_threads)
{
   struct _mesa_threadpool *pool;

   if (max_threads < 1)
      return NULL;

   pool = calloc(1, sizeof(*pool));
   if (!pool)
      return NULL;

   if (mtx_init(&pool->mutex, mtx_plain)) {
      free(pool);
      return NULL;
   }

   pool->refcnt = 1;

   if (cnd_init(&pool->thread_wakeup)) {
      mtx_destroy(&pool->mutex);
      free(pool);
      return NULL;
   }

   if (cnd_init(&pool->thread_joined)) {
      cnd_destroy(&pool->thread_wakeup);
      mtx_destroy(&pool->mutex);
      free(pool);
      return NULL;
   }

   pool->thread_control = MESA_THREADPOOL_NORMAL;

   pool->threads = malloc(sizeof(pool->threads[0]) * max_threads);
   if (!pool->threads) {
      cnd_destroy(&pool->thread_joined);
      cnd_destroy(&pool->thread_wakeup);
      mtx_destroy(&pool->mutex);
      free(pool);
      return NULL;
   }

   pool->max_threads = max_threads;

   make_empty_list(&pool->pending_tasks);

   return pool;
}

static mtx_t threadpool_lock = _MTX_INITIALIZER_NP;
static struct _mesa_threadpool *threadpool;

/**
 * Get the singleton GLSL thread pool.  \p max_threads is honored only by the
 * first call to this function.
 */
struct _mesa_threadpool *
_mesa_glsl_get_threadpool(int max_threads)
{
   mtx_lock(&threadpool_lock);
   if (!threadpool)
      threadpool = _mesa_threadpool_create(max_threads);
   if (threadpool)
      _mesa_threadpool_ref(threadpool);
   mtx_unlock(&threadpool_lock);

   return threadpool;
}

/**
 * Wait until all tasks are completed and threads are joined.
 */
void
_mesa_glsl_wait_threadpool(void)
{
   mtx_lock(&threadpool_lock);
   if (threadpool)
      _mesa_threadpool_join(threadpool, true);
   mtx_unlock(&threadpool_lock);
}

/**
 * Destroy the GLSL thread pool.
 */
void
_mesa_glsl_destroy_threadpool(void)
{
   mtx_lock(&threadpool_lock);
   if (threadpool) {
      /*
       * This is called from _mesa_destroy_shader_compiler().  No new task is
       * allowed since this point.  But contexts, who also own references to
       * the pool, can still complete tasks that have been queued.
       */
      _mesa_threadpool_set_shutdown(threadpool);

      _mesa_threadpool_join(threadpool, false);
      _mesa_threadpool_unref(threadpool);
      threadpool = NULL;
   }
   mtx_unlock(&threadpool_lock);
}
