/* ========================================================================= *
 * File: libleaks.c, part of sp-libleaks
 *
 * Copyright (C) 2005,2006,2007 by Nokia Corporation
 *
 * Author: Leonid Moiseichuk <leonid.moiseichuk@nokia.com>
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * ========================================================================= */

/* ========================================================================= *
 * Includes
 * ========================================================================= */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "report.h"

/* Function types to be overloaded */
typedef void* (*FCALLOC)(size_t nmemb, size_t size);
typedef void* (*FMALLOC)(size_t size);
typedef void* (*FREALLOC)(void* ptr, size_t size);
typedef void* (*FFREE)(void* ptr);

/* Structure that will be used to point of current functions set */
typedef struct
{
   FCALLOC  calloc;
   FMALLOC  malloc;
   FREALLOC realloc;
   FFREE    free;
} FUNCS;

/* ========================================================================= *
 * Local data.
 * ========================================================================= */

/* Set of pointers to local functions */
static void* ll_calloc_local(size_t nmemb, size_t size);
static void* ll_malloc_local(size_t size);
static void* ll_realloc_local(void* ptr, size_t size);
static void  ll_free_local(void* ptr);

static FUNCS s_funcs_local =
   {
      .calloc  = (FCALLOC) ll_calloc_local,
      .malloc  = (FMALLOC) ll_malloc_local,
      .realloc = (FREALLOC)ll_realloc_local,
      .free    = (FFREE)   ll_free_local
   }; /* s_funcs_local */

/* Set of pointers to runtime functions - shall be initialized during startup */
static FUNCS s_funcs_runtime =
   {
      NULL, NULL, NULL, NULL
   };

/* Set of pointers to standard functions (no tracking, but take into account local heap) */
static void* ll_calloc_standard(size_t nmemb, size_t size);
static void* ll_malloc_standard(size_t size);
static void* ll_realloc_standard(void* ptr, size_t size);
static void  ll_free_standard(void* ptr);

static FUNCS s_funcs_standard =
   {
      .calloc  = (FCALLOC) ll_calloc_standard,
      .malloc  = (FMALLOC) ll_malloc_standard,
      .realloc = (FREALLOC)ll_realloc_standard,
      .free    = (FFREE)   ll_free_standard
   }; /* s_funcs_standard */

/* Set of pointers to functions with logging */
static void* ll_calloc_tracked(size_t nmemb, size_t size);
static void* ll_malloc_tracked(size_t size);
static void* ll_realloc_tracked(void* ptr, size_t size);
static void  ll_free_tracked(void* ptr);

static FUNCS s_funcs_tracked =
   {
      .calloc  = (FCALLOC) ll_calloc_tracked,
      .malloc  = (FMALLOC) ll_malloc_tracked,
      .realloc = (FREALLOC)ll_realloc_tracked,
      .free    = (FFREE)   ll_free_tracked
   }; /* s_funcs_tracked */

/* Pointer to current function set */
static FUNCS* s_funcs_current = &s_funcs_local;

/* Heap bottom, not changed during working time */
static void_t     s_heap_bottom;
/* The lowest and the highest allocated blocks during measure session */
static void_t     s_lowest_block;
static void_t     s_highest_block;

/* Space for temporary data allocation before library initialized */
static char       s_heap[32 * 1024];
static char      *s_prev = NULL;
static unsigned   s_used = 0;
static int        s_heap_overflow = 0;

/* Mutex to block access to sensitive data from the parallel threads */
static pthread_mutex_t s_mutex;

/* Space for logging of allocations */
static INFO    s_infoStore[HEAP_SIZE * 1024];   /* Information about allocations  */
static INFO*   s_infoFree;    /* Pointer to first free item in s_infoStore */
static INFO*   s_infoUsed;    /* Pointer to first used item in s_infoStore */


/* ========================================================================= *
 * Local allocation methods.
 * ========================================================================= */

int show_backtrace (void **buffer, int size) {
  unw_cursor_t cursor;
  unw_context_t uc;
  unw_word_t ip;
  int n = 0;

  unw_getcontext (&uc);
  if (unw_init_local (&cursor, &uc) < 0)
    return -1;

  while (unw_step (&cursor) > 0)
    {
      if (n >= size)
      { fprintf(stderr, "n >= size \n");
	      
	return n;
      }

      if (unw_get_reg (&cursor, UNW_REG_IP, &ip) < 0){
	      fprintf(stderr,"unw_get_reg < 0 \n");
		return n;
      }
      buffer[n++] = (void *) (uintptr_t) ip;
    }
  return n;
}

/* ------------------------------------------------------------------------- *
 * ll_mem_take -- Take necessary amount of memory from the temporary data heap.
 * parameters:
 *    size - requested memory size
 * returns: pointer to data or null.
 * ------------------------------------------------------------------------- */

static void* ll_mem_take(size_t size)
{
/* Alignment according ANSI C. Structs, doubles and long long can require this */
#define ALIGNMENT    sizeof(double)

   /* Align size to integer */
   if (size & (ALIGNMENT - 1))
   {
      size = (size + ALIGNMENT) & ~(ALIGNMENT - 1);
   }

   /* Check available memory */
   if (s_used + size <= CAPACITY(s_heap))
   {
      char* ptr;
      s_prev = s_heap;
      ptr = s_heap + s_used;
      s_used += size;
      return ptr;
   }
   else
   {
      fprintf(stderr, "ERROR: internal heap overflow: current size=%u, request=%u!\n", s_used, size);
      s_heap_overflow = 1;
      return NULL;
   }
} /* ll_mem_take */

/* ------------------------------------------------------------------------- *
 * ll_mem_test -- Checks that pointed memory located in temporary data heap.
 * parameters:
 *    ptr - pointer to located area
 * returns: 0 if memory is not located in temporary heap.
 * ------------------------------------------------------------------------- */

static inline int ll_mem_test(void* ptr)
{
   char* mem = (char*)ptr;
   return (mem >= s_heap && mem < s_heap + s_used);
} /* ll_mem_test */

/* ------------------------------------------------------------------------- *
 * methods for calloc/malloc/realloc/free for local memory.
 * ------------------------------------------------------------------------- */

static void* ll_calloc_local(size_t nmemb, size_t size)
{
   const size_t space = nmemb * size;
   void* result = ll_mem_take(space);
   if (result)
      memset(result, 0, space);

   return result;
} /* ll_calloc_local */

static void* ll_malloc_local(size_t size)
{
   return ll_mem_take(size);
} /* ll_malloc_local */

static void* ll_realloc_local(void* ptr, size_t size)
{
   void* result = ll_mem_take(size);

   if (result && ptr)
      memcpy(result, ptr, size);

   return result;
} /* ll_realloc_local */

static void ll_free_local(void* ptr)
{
   /* Can only free last alloc */
   if (ptr == s_prev && s_prev > s_heap)
      s_used = s_prev - s_heap;
} /* ll_free_local */

/* ------------------------------------------------------------------------- *
 * methods for calloc/malloc/realloc/free for runtime library without tracking.
 * ------------------------------------------------------------------------- */

static void* ll_calloc_standard(size_t nmemb, size_t size)
{
   return s_funcs_runtime.calloc(nmemb, size);
}

static void* ll_malloc_standard(size_t size)
{
   return s_funcs_runtime.malloc(size);
}

static void* ll_realloc_standard(void* ptr, size_t size)
{
   /* Check the previously allocated memory */
   if ( !ll_mem_test(ptr) )
   {
      /* The standard approach - call RTL realloc */
      return s_funcs_runtime.realloc(ptr, size);
   }
   else
   {
      /* Make a malloc and copy data */
      void* pnew = s_funcs_runtime.malloc(size);
      if (size && pnew)
      {
         memcpy(pnew, ptr, size);
      }
      return pnew;
   }
} /* ll_realloc_standard */

static void  ll_free_standard(void* ptr)
{
   /* Call standard free only if memory is allocated by libc */
   if ( ll_mem_test(ptr) )
      ll_free_local(ptr);
   else
      s_funcs_runtime.free(ptr);
} /* ll_free_standard */

/* ========================================================================= *
 * Local methods for support allocations with tracking.
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * ll_trace_enabled -- Check trace for enabled status.
 * parameters: none
 * returns: 0 if trace not enabled.
 * ------------------------------------------------------------------------- */

static int ll_trace_enabled(void)
{
   return ((&s_funcs_tracked) == s_funcs_current);
} /* ll_trace_enabled */


/* ------------------------------------------------------------------------- *
 * ll_info_setup -- Setup s_infoStore for using.
 *
 * This is called only from the library init function and the signal
 * handler ll_trace_dump() function which is protected by mutex.
 * Therefore this doesn't need locking.
 * ------------------------------------------------------------------------- */

static void ll_info_setup(void)
{
   unsigned index;

   /* Clean-up all items */
   memset(s_infoStore, 0, sizeof(s_infoStore));
   s_infoUsed = NULL;
   s_infoFree = &s_infoStore[0];

   /* Add all items into free chain */
   for (index = 0; index < CAPACITY(s_infoStore) - 1; index++)
   {
      s_infoStore[index].next = &s_infoStore[index + 1];
   }
} /* ll_info_setup */


/* ------------------------------------------------------------------------- *
 * ll_info_create -- Allocate free cell to fill information.
 * ------------------------------------------------------------------------- */

static void ll_info_create(void_t ptr, size_t size)
{
   pthread_mutex_lock(&s_mutex);

   /* Check the pointer location for crossing lower and higher borders */
   if (ptr < s_lowest_block)
      s_lowest_block = ptr;

   if (ptr > s_highest_block)
      s_highest_block = ptr;

   /* Get the item from the free list */
   if ( s_infoFree )
   {
      /* Prepare data if cell is found */

      /*
       * We were called from:
       * - malloc/realloc/calloc
       * - s_funcs_tracked
       * - ll_info_create
       * Thus 3 first call frames will be ignored for save space
       * last 2 call frames can also usually be ignored
       */
#define IGNORE_TOP 2
#define IGNORE_BOTTOM 3
      int depth;
      int    ignore = IGNORE_BOTTOM;
      INFO*  item   = s_infoFree;
      void_t stack[STACK_SIZE + IGNORE_BOTTOM + IGNORE_TOP];

      /* temporarily use standard alloc funcs to avoid infinite recursion */
      s_funcs_current = &s_funcs_standard;
      depth = show_backtrace(stack, CAPACITY(stack));
      s_funcs_current = &s_funcs_tracked;

      /* Move item from free chain to used chain */
      s_infoFree = s_infoFree->next;
      item->next = s_infoUsed;
      s_infoUsed = item;

      /* Is backtrace handled successfully? */
      if ( depth > 0 )
      {
         /* Calculate how much frames may be ignored */
         if ( ignore >= depth )
            ignore = depth - 1;
         depth -= ignore;

         /* Remove first 2 calls that usually executable name and libc.so */
         if ( depth > IGNORE_TOP )
            depth -= IGNORE_TOP;

         /* Calculate how much frames we can store */
         if ( depth > (int)CAPACITY(item->stack) )
            depth = CAPACITY(item->stack);
      }

      item->data = ptr;
      item->size = size;
      if (depth > 0)
         memcpy(item->stack, &stack[ignore], sizeof(*stack) * depth);
   } /* if s_infoFree */

   pthread_mutex_unlock(&s_mutex);
} /* ll_info_create */

/* ------------------------------------------------------------------------- *
 * ll_info_delete -- Find and remove specific allocation information.
 * parameters: pointer to data.
 * returns: nothing.
 * ------------------------------------------------------------------------- */

static void ll_info_delete(void_t ptr)
{
   pthread_mutex_lock(&s_mutex);

   /* Have we used items? */
   if ( s_infoUsed )
   {
      INFO* cursor = s_infoUsed;

      /* Check for first item is a goal */
      if (ptr == s_infoUsed->data)
      {
         s_infoUsed = s_infoUsed->next;
      }
      else
      {
         INFO* next = cursor->next;

         /* Find the item and setup cursor to it */
         while (next)
         {
            /* Is next item to be removed? */
            if (ptr == next->data)
            {
               cursor->next = next->next;
               cursor = next;
               break;
            }
            else
            {
               cursor = next;
               next   = next->next;
            }
         }
      }

      /* Is item correctly found? */
      if (cursor && ptr == cursor->data)
      {
         /* Clean item and insert it to free chain */
         memset(cursor, 0, sizeof(*cursor));
         cursor->next = s_infoFree;
         s_infoFree   = cursor;
      }
   }

   pthread_mutex_unlock(&s_mutex);
} /* ll_info_delete */


/* ------------------------------------------------------------------------- *
 * ll_stack_depth -- Calculate the depth of stack in INFO structure.
 * parameters: stack pointers and maximal depth of stack.
 * returns: stack depth.
 * ------------------------------------------------------------------------- */

static unsigned ll_stack_depth(const void_t* stack, unsigned depth)
{
   while (depth > 0)
   {
      depth--;
      if ( stack[depth] )
         return depth + 1;
   }

   return (NULL != *stack);
} /* ll_stack_depth */

/* ------------------------------------------------------------------------- *
 * ll_stack_compare -- Compare 2 stacks for qsort.
 * parameters: stack pointers.
 * returns: comparison result.
 * ------------------------------------------------------------------------- */

typedef int (*QSORTFUNC)(const void* item1, const void* item2);

static int ll_stack_compare(const INFO* item1, const INFO* item2)
{
   unsigned index;

   for (index = 0; index < CAPACITY(item1->stack); index++)
   {
      const unsigned a = (unsigned)(item1->stack[index]);
      const unsigned b = (unsigned)(item2->stack[index]);

      /* Some sopthisticated operations */
      if (a < b)
         return -1;
      if (a > b)
         return 1;
      if ( !a )
         break;
   }

   /* Well, they are equal - so return 0 */
   return 0;
} /* ll_stack_compare */

/* ------------------------------------------------------------------------- *
 * ll_copyfile -- Copy specified text file to destination.
 * parameters: source and destination file.
 * returns: nothing.
 * ------------------------------------------------------------------------- */
static void ll_copyfile(const char* src, const char* dst)
{
   char line[256];
   FILE *fi, *fo;

   fi = fopen(src, "rt");
   if (!fi) {
      fprintf (stderr, "\n%s: unable to open '%s' for reading\n", TOOL_NAME, src);
      return;
   }
   fo = fopen(dst, "wt");
   if (!fo) {
      fclose(fi);
      fprintf (stderr, "\n%s: unable to open '%s' for writing\n", TOOL_NAME, dst);
      return;
   }

   while (fgets(line, sizeof(line), fi)) {
      fputs(line, fo);
   }
   fclose(fo);
   fclose(fi);
} /* ll_copyfile */

/* ------------------------------------------------------------------------- *
 * ll_trace_dump -- Create the file and dump trace information into it.
 * parameters: none
 * returns: none.
 * ------------------------------------------------------------------------- */

void ll_trace_dump(void)
{
   static unsigned step = 0;
   struct mallinfo mi;

   char  path[256];
   FILE* file = NULL;
   pid_t pid;
   int count;

   pthread_mutex_lock(&s_mutex);

   /* Switch off tracing first */
#if DUMP_USING_INTERNAL_HEAP
   /* It would be good to use internal allocator here i.e. within
    * signal handler, but that would need to first track all allocs
    * so that it can free them!
    */
   s_funcs_current = &s_funcs_local;
#else
   s_funcs_current = &s_funcs_standard;
#endif
   mi = mallinfo();

   /* Open the file */
   ++step;
   pid = getpid();
   count = snprintf(path, sizeof(path), TOOL_FILE, getenv("HOME"), pid, step);
   if (count > 0)
      file = fopen(path, "wt");
   if ( !file )
      file = stderr;

   /* Dump the number of allocated blocks */
   fprintf (file, "file %s: memory information for pid %d\n", path, pid);

   fprintf (file, "heap status information:\n");
   fprintf (file, "- heap bottom 0x%08x\n", (unsigned)s_heap_bottom);
   fprintf (file, "- heap top 0x%08x\n", (unsigned)sbrk(0));
   fprintf (file, "- the lowest block 0x%08x\n", (unsigned)s_lowest_block);
   fprintf (file, "- the highest block 0x%08x\n", (unsigned)s_highest_block);
   fprintf (file, "- non-mmapped space allocated from system %d\n", mi.arena);
   fprintf (file, "- number of free chunks %d\n", mi.ordblks);
   fprintf (file, "- number of fastbin blocks %d\n", mi.smblks);
   fprintf (file, "- number of mmapped regions %d\n", mi.hblks);
   fprintf (file, "- space in mmapped regions %d\n", mi.hblkhd);
   fprintf (file, "- maximum total allocated space %d\n", mi.usmblks);
   fprintf (file, "- space available in freed fastbin blocks %d\n", mi.fsmblks);
   fprintf (file, "- total allocated space %d\n", mi.uordblks);
   fprintf (file, "- total free space %d\n", mi.fordblks);
   fprintf (file, "- top-most, releasable (via malloc_trim) space %d\n", mi.keepcost);

   /* Dumping the memory leaks */
   if ( s_infoUsed )
   {
      static INFO *s_tempStore[CAPACITY(s_infoStore)];
      const INFO* cursor;
      unsigned    index;
      unsigned counter;
      INFO**   leaks;
      unsigned garbaged;

      leaks = s_tempStore;

      /* Re-pack leaked blocks and sort them  */
      garbaged = 0;
      for (counter = 0, cursor = s_infoUsed; cursor; cursor = cursor->next, counter++)
      {
	 leaks[counter] = (INFO*)cursor;
	 garbaged += cursor->size;
	 
	 /* Now sort all leaked blocks by call stacks */
	 for (index = counter; index > 1; index--)
	 {
	    if ( ll_stack_compare(leaks[index - 1], leaks[index]) > 0 )
	    {
	       INFO* swap = leaks[index - 1];
	       
	       leaks[index - 1] = leaks[index];
	       leaks[index] = swap;
	    }
	    else
	    {
	       break;
	    }
	 }
      }
      
      fprintf (file, "information about %u memory leaks\n", counter);
      for (index = 0; index < counter; index++)
      {
	 /* Print leak general information */
	 cursor = leaks[index];
	 
	 fprintf (file, "%u. block at 0x%08x with size %u\n", index, (unsigned)cursor->data, cursor->size);
	 /* Print leak stack trace if that is the last or next item has another stack trace */
	 if (index + 1 == counter || ll_stack_compare(cursor, leaks[index + 1]))
	 {
	    const unsigned depth  = ll_stack_depth(cursor->stack, CAPACITY(cursor->stack));
	    
	    if (depth)
	    {
	       unsigned idx;
	       char** symbols = backtrace_symbols(cursor->stack, depth);

	       if (symbols)
	       {
		  for (idx = 0; idx < depth; idx++)
		  {
                     fprintf (file, "   %s\n", symbols[idx]);
		  }
		  free(symbols);
	       }
	       else
	       {
		  fprintf (file, "   ERR: backtrace_symbols() returned NULL\n");
	       }
	    }
	    else
	    {
	       fprintf (file, "   NO STACK INFORMATION AVAILABLE\n");
	    }
	 }
      } /* for all leaks */
      
      fprintf (file, "%u blocks leaked with size %u bytes\n", counter, garbaged);
      
      /* Clean up memory for next tracking */
      ll_info_setup();
   }
   else
   {
      fprintf (file, "no memory leaks detected\n");
   }

#if DUMP_USING_INTERNAL_HEAP
   if (s_heap_overflow)
   {
      fprintf(file, "ERROR %s: internal heap overflow -> terminate\n", TOOL_NAME);
      fprintf(stderr, "ERROR %s: internal heap overflow -> terminate\n", TOOL_NAME);
      _exit(-1);	/* skip exit handlers which may try allocs */
   }
   else
   {
      fprintf(stderr, "%u/%u bytes are used from the internal heap\n", s_used, sizeof(s_heap));
   }
#endif
   /* Close file if not stderr */
   if ( file != stderr )
      fclose(file);

   /* Report the /proc/self/maps file */
   snprintf(path, sizeof(path), TOOL_FILE_MAP, getenv("HOME"), pid, step);
   ll_copyfile("/proc/self/maps", path);

#if DUMP_USING_INTERNAL_HEAP
   /* Now that both tracing and dumping is done, switch to standard allocs */
   s_funcs_current = &s_funcs_standard;
#endif
   pthread_mutex_unlock(&s_mutex);
} /* ll_trace_dump */

/* ------------------------------------------------------------------------- *
 * ll_trace_signal -- Create the file and dump trace information into it.
 * parameters: none
 * returns: none.
 *
 * Yikes, locks etc. in signal handlers...
 * ------------------------------------------------------------------------- */

void ll_trace_signal(int signo)
{
   int tmp = errno;
   if ( ll_trace_enabled() )
   {
      /* Stop the tracing and dump the trace information */
      ll_trace_dump();
      fprintf (stderr, "\n%s: memory leaks tracing finished\n", TOOL_NAME);
   }
   else
   {
      /* Start tracing of memory operations */
      pthread_mutex_lock(&s_mutex);
      s_funcs_current = &s_funcs_local;
      fprintf (stderr, "\n%s: memory leaks tracing started\n", TOOL_NAME);
      s_lowest_block  = (void_t)(-1 >> 1); /* Maximal positive value */
      s_highest_block = NULL;
      s_funcs_current = &s_funcs_tracked;
      pthread_mutex_unlock(&s_mutex);
   }
   errno = tmp;

   /* Makes compiler happy */
   signo = signo;
} /* ll_trace_signal*/

/* ------------------------------------------------------------------------- *
 * ll_trace_stop -- Finish tracing if it enabled.
 * parameters: none
 * returns: none.
 * ------------------------------------------------------------------------- */

static void ll_trace_stop(int signo)
{
   if ( ll_trace_enabled() )
   {
      int tmp = errno;
      /* Stop the tracing and dump the trace information */
      ll_trace_dump();
      fprintf (stderr, "\n%s: memory leaks tracing finished\n", TOOL_NAME);
      errno = tmp;
   }
   /*
    * This is one-shot handler, raise the signal again so that
    * original handler catches this too
    */
   raise(signo);
} /* ll_trace_stop */

/* ------------------------------------------------------------------------- *
 * ll_signal_attach -- Enabled trace storing if specified signals are received.
 * parameters: none
 * returns: none.
 * ------------------------------------------------------------------------- */

static void ll_signal_attach(void)
{
  /*
   * These signals shall be tracked by default, application
   * can override them as these are set at library init
   */
   static const int signals[] =
   {
      SIGHUP,  SIGQUIT, SIGILL,  SIGTRAP, SIGABRT, SIGBUS, SIGFPE,
      SIGSEGV, SIGPIPE, SIGXCPU, SIGXFSZ, SIGSYS
   }; /* signals */
   struct sigaction sa;
   unsigned index;

   /* Set signal handler for toggling the reporting */
#if TOOL_LOGO
   fprintf(stderr, "signal %d (%s) is used to switching to/from tracking mode\n", TOOL_SIGNAL, strsignal(TOOL_SIGNAL));
#endif
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sa.sa_handler = ll_trace_signal;
   sigaction(TOOL_SIGNAL, &sa, NULL);

   /*
    * Set handlers for signals which terminate the process,
    * handle just the first signal
    */
   sa.sa_flags = SA_ONESHOT;
   for (index = 0; index < CAPACITY(signals); index++)
   {
      sa.sa_handler = ll_trace_stop;
      sigaction(signals[index], &sa, NULL);
#if TOOL_LOGO
   fprintf(stderr, "signal %d (%s) is used to finish tracing mode\n", signals[index], strsignal(signals[index]));
#endif
   }
} /* ll_signal_attach */


/* ------------------------------------------------------------------------- *
 * methods for calloc/malloc/realloc/free with tracking allocations.
 * ------------------------------------------------------------------------- */

static void* ll_calloc_tracked(size_t nmemb, size_t size)
{
   void* ptr = s_funcs_runtime.calloc(nmemb, size);
   ll_info_create(ptr, nmemb * size);
   return ptr;
} /* ll_calloc_tracked */

static void* ll_malloc_tracked(size_t size)
{
   void* ptr = s_funcs_runtime.malloc(size);
   ll_info_create(ptr, size);
   return ptr;
} /* ll_malloc_tracked */

static void* ll_realloc_tracked(void* ptr, size_t size)
{
   if ( !ll_mem_test(ptr) )
   {
      ll_info_delete(ptr);
   }
   else
   {
      ptr = NULL;
   }

   ptr = s_funcs_runtime.realloc(ptr, size);
   if ( ptr )
   {
      ll_info_create(ptr, size);
   }

   return ptr;
} /* ll_realloc_tracked */

static void  ll_free_tracked(void* ptr)
{
   /* Call standard free only if memory is allocated by libc */
   if ( !ll_mem_test(ptr) )
   {
      /* Really free memory */
      s_funcs_runtime.free(ptr);
      ll_info_delete(ptr);
   }
} /* ll_free_tracked */

/* ------------------------------------------------------------------------- *
 * ll_libc -- Setup standard libc functions.
 * parameters: nothing.
 * returns: see libc_XXX.
 * ------------------------------------------------------------------------- */
static unsigned ll_libc(void)
{
   /* LIBC shall be initialized before we put pointers to internal store */
   /* If we don't do it on ARM with pthreads we will have recusion!      */
   FCALLOC  cfunc  = (FCALLOC)dlsym(RTLD_NEXT, "calloc");
   FMALLOC  mfunc  = (FMALLOC)dlsym(RTLD_NEXT, "malloc");
   FREALLOC rfunc = (FREALLOC)dlsym(RTLD_NEXT, "realloc");
   FFREE    ffunc = (FFREE)dlsym(RTLD_NEXT, "free");

   /* Check function initialization */
   if (cfunc && mfunc && rfunc && ffunc)
   {
      void* ptr;

      /* Initialize libc, that is important */
      ptr = cfunc(sizeof(int), sizeof(int));
      ffunc(ptr);

      ptr = mfunc(sizeof(int));
      ptr = rfunc(ptr, sizeof(int) << 1);
      ffunc(ptr);

      /* Copy pointers to local variables */
      s_funcs_runtime.calloc  = cfunc;
      s_funcs_runtime.malloc  = mfunc;
      s_funcs_runtime.realloc = rfunc;
      s_funcs_runtime.free    = ffunc;

      return 1;
   }
   else
   {
      return 0;
   }
} /* ll_libc */


/* ========================================================================= *
 * Public methods.
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * calloc -- The calloc method replacement.
 * parameters: see man.
 * returns: requested area.
 * ------------------------------------------------------------------------- */

void* ll_calloc(size_t nmemb, size_t size)
{
   return s_funcs_current->calloc(nmemb, size);
} /* calloc */

/* ------------------------------------------------------------------------- *
 * malloc -- The malloc method replacement.
 * parameters: see man.
 * returns: requested area.
 * ------------------------------------------------------------------------- */

void* ll_malloc(size_t size)
{
   return s_funcs_current->malloc(size);
} /* malloc */

/* ------------------------------------------------------------------------- *
 * realloc -- The realloc method replacement.
 * parameters: see man.
 * returns: requested area.
 * ------------------------------------------------------------------------- */

void* ll_realloc(void* ptr, size_t size)
{
   return s_funcs_current->realloc(ptr, size);
} /* realloc */

/* ------------------------------------------------------------------------- *
 * free -- The free method replacement.
 * parameters: see man.
 * returns: nothing.
 * ------------------------------------------------------------------------- */

void ll_free(void* ptr)
{
   s_funcs_current->free(ptr);
} /* free */

 /* ------------------------------------------------------------------------- *
 * ll_init -- this function shall be called by Loader when library is loaded.
 * parameters: none.
 * returns: none.
 * ------------------------------------------------------------------------- */

void ll_init(void)
{
   const char* start = getenv(TOOL_START);

   /* Store the heap bottom */
   s_heap_bottom = sbrk(0);

   /* Check for validity of initialization by malloc function */
   if ( !ll_libc() )
   {
      fprintf(stderr, "%s: unable to obtain reference to LIBC functions calloc, malloc, realloc, free\n", TOOL_NAME);
      exit(-1);
   }

   /* Setup functions as working */
   s_funcs_current = &s_funcs_standard;
   if (s_heap_overflow)
   {
      fprintf(stderr, "%s: internal heap overflow\n", TOOL_NAME);
      exit(-1);
   }
   pthread_mutex_init (&s_mutex, NULL);

#if TOOL_LOGO
   fprintf(stderr, "%s version %s build %s %s\n", TOOL_NAME, TOOL_VERS, __DATE__, __TIME__);
   fprintf(stderr, "(c) 2005 Nokia\n\n");

   fprintf(stderr, "internal heap with capacity %u bytes allocated from 0x%08x to 0x%08x\n", CAPACITY(s_heap), (unsigned)s_heap, (unsigned)s_heap + CAPACITY(s_heap));
   fprintf(stderr, "%u bytes are used on launch in internal heap\n", s_used);
   fprintf(stderr, "s_infoStore[%u] occupied %u bytes at 0x%08x\n", CAPACITY(s_infoStore), sizeof(s_infoStore), (unsigned)s_infoStore);
   fprintf(stderr, "  = max. stackdepth of %d for %d allocs\n", STACK_SIZE, HEAP_SIZE*1024);
   fprintf(stderr, "s_mutex created at 0x%08x from thread %lu\n", (unsigned)(&s_mutex), pthread_self());
   fprintf(stderr, "%s is %s\n", TOOL_START, start);
#endif

   ll_info_setup();
   ll_signal_attach();

   if (start && ('1' == *start || 'y' == *start))
      ll_trace_signal(-1);
} /* ll_init */

/* ------------------------------------------------------------------------- *
 * ll_fini -- this function shall be called by Loader when library is unloaded.
 * parameters: none.
 * returns: none.
 * ------------------------------------------------------------------------- */

void ll_fini(void)
{
   ll_trace_stop(-1);
#if TOOL_LOGO
   fprintf(stderr, "\n%s finalization completed\n", TOOL_NAME);
#endif
} /* ll_fini */

/* ========================================================================= *
 *                    No more code in file libleaks.c                        *
 * ========================================================================= */
