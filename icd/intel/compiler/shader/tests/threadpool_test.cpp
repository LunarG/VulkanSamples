/*
 * Copyright © 2014 LunarG, Inc.
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
#include <gtest/gtest.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "c11/threads.h"

#include "threadpool.h"

#define NUM_THREADS 10
#define OPS_PER_THREAD 100
#define MAX_TASKS 10

static void
race_cb(void *data)
{
   usleep(1000 * 5);
}

static int
race_random_op(void *data)
{
   struct _mesa_threadpool *pool = (struct _mesa_threadpool *) data;
   struct _mesa_threadpool_task *tasks[MAX_TASKS];
   int num_tasks = 0;
   int num_ops = 0;
   int i;

   while (num_ops < OPS_PER_THREAD) {
      int op = (random() % 1000);

      if (op < 480) { /* 48% */
         if (num_tasks < MAX_TASKS) {
            tasks[num_tasks++] =
               _mesa_threadpool_queue_task(pool, race_cb, NULL);
         }
      }
      else if (op < 980) { /* 50% */
         if (num_tasks)
            _mesa_threadpool_complete_task(pool, tasks[--num_tasks]);
      }
      else if (op < 995) { /* 1.5% */
         for (i = 0; i < num_tasks; i++)
            _mesa_threadpool_complete_task(pool, tasks[i]);
         num_tasks = 0;
      }
      else { /* 0.5% */
         _mesa_threadpool_join(pool, (op < 998));
      }

      num_ops++;
   }

   for (i = 0; i < num_tasks; i++)
      _mesa_threadpool_complete_task(pool, tasks[i]);

   _mesa_threadpool_unref(pool);

   return 0;
}

/**
 * \name Thread safety
 */
/*@{*/
TEST(threadpool_test, race)
{
   struct _mesa_threadpool *pool;
   thrd_t threads[NUM_THREADS];
   int i;

   srandom(time(NULL));
   pool = _mesa_threadpool_create(4);
   for (i = 0; i < NUM_THREADS; i++) {
      thrd_create(&threads[i], race_random_op,
            (void *) _mesa_threadpool_ref(pool));
   }
   _mesa_threadpool_unref(pool);

   for (i = 0; i < NUM_THREADS; i++)
      thrd_join(threads[i], NULL);

   /* this is not really a unit test */
   EXPECT_TRUE(true);
}

static void
basic_cb(void *data)
{
   int *val = (int *) data;

   usleep(1000 * 5);
   *val = 1;
}

TEST(threadpool_test, basic)
{
   struct _mesa_threadpool *pool;
   struct _mesa_threadpool_task *tasks[2];
   int vals[2];

   pool = _mesa_threadpool_create(2);

   vals[0] = vals[1] = 0;
   tasks[0] = _mesa_threadpool_queue_task(pool, basic_cb, (void *) &vals[0]);
   tasks[1] = _mesa_threadpool_queue_task(pool, basic_cb, (void *) &vals[1]);
   _mesa_threadpool_complete_task(pool, tasks[0]);
   _mesa_threadpool_complete_task(pool, tasks[1]);
   EXPECT_TRUE(vals[0] == 1 && vals[1] == 1);

   _mesa_threadpool_unref(pool);
}

/*@}*/
