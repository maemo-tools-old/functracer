/* ========================================================================= *
 * General settings.
 * ========================================================================= */

#define TOOL_NAME       "functracker"
#define TOOL_VERS       "0.7"
#define TOOL_FILE       "%s/func-%d.%u.trace"
#define TOOL_FILE_MAP   "%s/func-%d.%u.map"
#define TOOL_SIGNAL     SIGUSR1
#define TOOL_START      "PROFILE_START"

#define TOOL_LOGO       1

/* Define STACK_SIZE as 7 by default */
#ifndef STACK_SIZE
#define STACK_SIZE      7
#endif

/* Define HEAP_SIZE as 32K simultaneous allocations */
#ifndef HEAP_SIZE
#define HEAP_SIZE       32
#endif

//#define DUMP_USING_INTERNAL_HEAP 1	/* doesn't work yet */

/* ========================================================================= *
 * Definitions.
 * ========================================================================= */

#define CAPACITY(a)        (sizeof(a) / sizeof(*a))

/* Pointer to void* to use in allocations and call stack */
typedef void* void_t;

/* Structure for storing memory additional information about allocation point */
struct SINFO;
typedef struct SINFO INFO;

struct SINFO
{
   INFO*    next;       /* Pointer to next struct in chunk   */
   void_t   data;       /* Here is pointer to allocated data */
   size_t   size;       /* Allocated block size in bytes     */
   void_t   stack[STACK_SIZE]; /* Stack trace information    */
}; /* struct SINFO */

extern void ll_init(void);
extern void ll_trace_signal(int signo);
extern void* ll_calloc(size_t nmemb, size_t size);
extern void* ll_malloc(size_t size);
extern void* ll_realloc(void* ptr, size_t size);
extern void ll_free(void* ptr);
