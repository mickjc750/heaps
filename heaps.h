#ifndef _HEAPS_H_
#define _HEAPS_H_
/*

Heaps.
 A layer which can be added to any allocator, to track allocations, find leaks, etc..

-----------------------------------
Adding heaps.h to your application.
-----------------------------------

In ONE .c file:
	#define HEAPS_IMPLEMENTATION 

Then provide macros or functions for some selection of the following:

		heaps_platform_alloc(size_t size)		
		heaps_platform_free(void* ptr)			
		heaps_platform_realloc(size_t size)

Note that there is no heaps_platform_calloc, as the platforms calloc function will not be used.
heaps_calloc() WILL be available, but will allocate using heaps_platform_alloc(), or heaps_platform_realloc()

An error handler if available:
		heaps_error_handler(const char* message, const char* file, int line)

If available, a heap integrity test which returns true if the heap is ok:
		bool heaps_platform_check(void)

If available, a function or macro which gives the size of the largest allocation which can currently be made:
		size_t heaps_platform_largest_free()


The behaviour of realloc(ptr, 0) is implementation defined.
The default behaviour expected (and that of glibc) is that realloc(ptr,0) will free ptr and return NULL.
If the realloc() in use doesn't free memory when given a size of 0, then define this symbol:
		#define HEAPS_REALLOC_ZERO_DOESNT_FREE.


By default heaps will walk the list of existing allocations before each alloc/free operation as a partial integrity test.
If you do not want this to happen (due to the performance hit), then define the symbol:
	#define HEAPS_NO_PRE_OPERATION_WALK_CHECK
	Note that even if you define this, heaps_platform_check() will still be called if it has been provided.

Then:
	#include "heaps.h"


-----------------------------------
 Usage
-----------------------------------

 Use heaps_alloc(), heaps_free(), heaps_realloc(), and heaps_calloc() just as you would malloc(), free(), realloc() and calloc().
 Instead of returning NULL on allocation/reallocation failure, the error handler (if provided) will be called.

 Do NOT pass pointers returned from heaps_alloc() directly to free() or you will break the heap.

 Heaps will link together every allocation together with some meta data of the callers source location (file+line) and size.
 This linked list of heaps_t structures is available to the application by calling heaps_get_allocation_list().
 
 Any call to heaps_free() will check the linked list of allocations to verify the address was previously returned by heaps_alloc().
 The error handler will be called if a heaps_free() or heaps_realloc() operation is attempted on an invalid address.  

 Various statistics are available, including the peak allocation count, headroom (if heaps_platform_largest_free is provided),
  details of the largest allocation made, and a report detailing the number of allocations and size used by each source location.

*/

	#include <stdbool.h>
	#include <stdint.h>
	#include <stddef.h>


//********************************************************************************************************
// Public defines
//********************************************************************************************************
	#ifdef HEAPS_SANDBOX
		#define STATIC_IF_SANDBOXED static
	#else
		#define STATIC_IF_SANDBOXED
	#endif

	#define heaps_alloc(size) 		heaps_alloc_(size, __FILE__, __LINE__)
	#define heaps_free(ptr)			heaps_free_(ptr, __FILE__, __LINE__)
	#define heaps_realloc(ptr,size)	heaps_realloc_(ptr, size, __FILE__, __LINE__)
	#define heaps_calloc(qty,size)	heaps_calloc_(qty, size, __FILE__, __LINE__)

	typedef struct heaps_t
	{
		size_t			size;
		const char* 	file;
		int 			line;
		struct heaps_t* next;
		uint8_t		content[0] __attribute__((aligned));
	} heaps_t;

	typedef struct heaps_report_t
	{
		const char* 	file;
		int 			line;
		int 			count;
		size_t 			size;
	} heaps_report_t;

//********************************************************************************************************
// Public variables
//********************************************************************************************************

//********************************************************************************************************
// Public prototypes
//********************************************************************************************************

	STATIC_IF_SANDBOXED void* heaps_alloc_(size_t size, const char* file, int line);
	STATIC_IF_SANDBOXED void* heaps_free_(void* ptr, const char* file, int line);
	STATIC_IF_SANDBOXED void* heaps_realloc_(void* ptr, size_t size, const char* file, int line);
	STATIC_IF_SANDBOXED void* heaps_calloc_(size_t qty, size_t size, const char* file, int line);

	STATIC_IF_SANDBOXED int heaps_get_allocation_count(void);					// The current number of allocations
	STATIC_IF_SANDBOXED int heaps_get_allocation_count_peak(void);				// The highest number of allocations that has ever occurred.
	STATIC_IF_SANDBOXED size_t heaps_get_headroom(void);						// The minimum free space that has occurred since reset.
	STATIC_IF_SANDBOXED heaps_report_t heaps_get_largest_allocation(void);		// Return details (file/line/size) of the largest allocation ever made.

//	Get the head of a linked list of allocations
	STATIC_IF_SANDBOXED heaps_t* heaps_get_allocation_list(void);

//	This feature is used for finding leaks, it is only provided if heaps_platform_realloc is available.
//	Returns an array that for each source location, shows the number of current allocations, and total size used.
//	One of these allocations will be the array itself, and it must be passed to heaps_free() when no longer needed.
//	The number of elements in the array is written to *arr_size. If this is 0, then the return value will be NULL and does not need to be freed.
	STATIC_IF_SANDBOXED heaps_report_t* heaps_report(int* arr_size);

// 	The report may be sorted using qsort, and the comparator functions are provided
//	Example:	qsort(arr, arr_size, sizeof(heaps_report_t), heaps_report_sorter_descending_count);
	STATIC_IF_SANDBOXED int heaps_report_sorter_descending_size(const void* a, const void* b);
	STATIC_IF_SANDBOXED int heaps_report_sorter_descending_count(const void* a, const void* b);

#endif


#ifdef HEAPS_IMPLEMENTATION
	#include <stdlib.h>
	#include <string.h>

//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************
//********************************************************************************************************


//********************************************************************************************************
// Private defines
//********************************************************************************************************

	#ifndef heaps_platform_check
		#define heaps_platform_check() (true)
	#endif
	#ifndef heaps_error_handler
		#define heaps_error_handler(msg,file,line)	((void)0)
	#endif
	#ifndef heaps_platform_largest_free
		#define heaps_platform_largest_free() (0)
	#endif

//********************************************************************************************************
// Private variables
//********************************************************************************************************

	static heaps_t* head = NULL;
	static int allocation_count = 0;
	static int allocation_count_peak = 0;
	static size_t headroom = (size_t)-1;
	static heaps_report_t largest_allocation = {0};

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	static void* alloc_(size_t size, const char* file, int line);
	static void* realloc_(void* ptr, size_t size, const char* file, int line);
	static void* free_(void* ptr, const char* file, int line);
	static void* calloc_(size_t qty, size_t size, const char* file, int line);


	static void check_heap(const char* file, int line);

//	Given a pointer to a heaps_t, fill out the heaps_t members, link it, and return it's content
	static void* link_allocation(heaps_t* meta, size_t size, const char* file, int line);

//	Given a void* to be freed, find it's containing heaps_t, unlink it, and return the address to free.
//	Returns NULL if the given ptr is not a value previously returned by heaps_alloc().
	static void* unlink_allocation(void* ptr);

	static void track_headroom(void);

	static heaps_report_t* report(int* arr_size);
	static bool add_to_report(heaps_report_t** dst, int* dst_size, heaps_t* src);

//********************************************************************************************************
// Public functions
//********************************************************************************************************

#ifdef heaps_platform_alloc
STATIC_IF_SANDBOXED void* heaps_alloc_(size_t size, const char* file, int line)
{
	return alloc_(size, file, line);
}
#endif

#ifdef heaps_platform_realloc
STATIC_IF_SANDBOXED void* heaps_realloc_(void* ptr, size_t size, const char* file, int line)
{
	return realloc_(ptr, size, file, line);
}
#endif

#ifdef heaps_platform_free
STATIC_IF_SANDBOXED void* heaps_free_(void* ptr, const char* file, int line)
{
	return free_(ptr, file, line);
}
#endif

#if (defined heaps_platform_alloc || defined heaps_platform_realloc)
STATIC_IF_SANDBOXED void* heaps_calloc_(size_t qty, size_t size, const char* file, int line)
{
	return calloc_(qty, size, file, line);
}
#endif

#ifdef heaps_platform_realloc
STATIC_IF_SANDBOXED heaps_report_t* heaps_report(int* arr_size)
{
	return report(arr_size);
}

STATIC_IF_SANDBOXED int heaps_report_sorter_descending_count(const void* a, const void* b)
{
	return ((heaps_report_t*)b)->count - ((heaps_report_t*)a)->count;
}

STATIC_IF_SANDBOXED int heaps_report_sorter_descending_size(const void* a, const void* b)
{
	return ((heaps_report_t*)b)->size - ((heaps_report_t*)a)->size;
}
#endif

STATIC_IF_SANDBOXED int heaps_get_allocation_count(void)
{
	return allocation_count;
}

STATIC_IF_SANDBOXED int heaps_get_allocation_count_peak(void)
{
	return allocation_count_peak;
}

STATIC_IF_SANDBOXED size_t heaps_get_headroom(void)
{
	return headroom;
}

STATIC_IF_SANDBOXED heaps_report_t heaps_get_largest_allocation(void)
{
	return largest_allocation;
}

STATIC_IF_SANDBOXED heaps_t* heaps_get_allocation_list(void)
{
	return head;
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************


#ifdef heaps_platform_alloc
static void* alloc_(size_t size, const char* file, int line)
{
	void* retval = NULL;
	heaps_t* meta;
	size_t size_with_meta = size + sizeof(heaps_t);

	check_heap(file, line);
	meta = heaps_platform_alloc(size_with_meta);
	if(meta == NULL)
		heaps_error_handler("allocation failed", file, line);
	else
	{
		retval = link_allocation(meta, size, file, line);
		track_headroom();
	};

	return retval;
}
#endif

#ifdef heaps_platform_realloc
static void* realloc_(void* ptr, size_t size, const char* file, int line)
{
	void* to_free;
	void* to_realloc;
	void* retval = NULL;
	heaps_t* meta;
	size_t size_with_meta = size + sizeof(heaps_t);
	bool allocating = (ptr == NULL);
#ifdef HEAPS_REALLOC_ZERO_DOESNT_FREE
	bool reallocating = (ptr != NULL);
	bool freeing = false;
#else
	bool reallocating = (ptr != NULL && size > 0);
	bool freeing = (ptr != NULL && size == 0);
#endif

	check_heap(file, line);
	if(allocating)
	{
		meta = heaps_platform_realloc(NULL, size_with_meta);
		if(meta == NULL)
			heaps_error_handler("allocation via heaps_realloc() failed", file, line);
		else
			retval = link_allocation(meta, size, file, line);			
	}
	else if(freeing)
	{
		to_free = unlink_allocation(ptr);
		if(to_free == NULL)
			heaps_error_handler("false free via heaps_realloc()", file, line);
		else
			heaps_realloc(to_free, 0);
	}
	else if(reallocating)
	{
		to_realloc = unlink_allocation(ptr);
		meta = heaps_platform_realloc(to_realloc, size_with_meta);
		if(meta == NULL)
			heaps_error_handler("heaps_realloc() failed", file, line);
		else
			retval = link_allocation(meta, size, file, line);
	};

	if(retval != NULL)
		track_headroom();

	return retval;
}
#endif

#ifdef heaps_platform_free
static void* free_(void* ptr, const char* file, int line)
{
	void* to_free;
	check_heap(file, line);
	if(ptr)
	{
		to_free = unlink_allocation(ptr);
		if(to_free == NULL)
			heaps_error_handler("false free", file, line);
		else
			heaps_platform_free(to_free);
	};
	return NULL;
}
#endif

#if (defined heaps_platform_alloc || defined heaps_platform_realloc)
static void* calloc_(size_t qty, size_t size, const char* file, int line)
{
	void* retval = NULL;
	heaps_t* meta;
	size_t size_with_meta = (size * qty) + sizeof(heaps_t);

	check_heap(file, line);
	size *= qty;
	#ifdef heaps_platform_alloc
		meta = heaps_platform_alloc(size_with_meta);
	#else
		meta = heaps_platform_realloc(NULL, size_with_meta);
	#endif
	if(meta == NULL)
		heaps_error_handler("calloc failed", file, line);
	else
	{
		retval = link_allocation(meta, size, file, line);
		memset(retval, 0, size);
		track_headroom();
	};
	return retval;
}
#endif

static void check_heap(const char* file, int line)
{
#ifndef HEAPS_NO_PRE_OPERATION_WALK_CHECK
	heaps_t *link = head;
	int count = 0;
	while(link)
	{
		count++;
		link = link->next;
	};
	if(count != allocation_count)
		heaps_error_handler("heap broken", file, line);
#endif
	if(!heaps_platform_check())
		heaps_error_handler("heap broken", file, line);
}

static void* link_allocation(heaps_t* meta, size_t size, const char* file, int line)
{
	meta->size = size;
	meta->file = file;
	meta->line = line;
	meta->next = head;
	head = meta;
	allocation_count++;
	if(allocation_count > allocation_count_peak)
		allocation_count_peak = allocation_count;
  	if(size > largest_allocation.size)
	{
		largest_allocation.size = size;
		largest_allocation.file = file;
		largest_allocation.line = line;
	};
	return meta->content;
}

static void* unlink_allocation(void* ptr)
{
	heaps_t **link = &head;
	void* to_free;

	while(*link && (*link)->content != ptr)
		link = &(*link)->next;

	to_free = *link;
	if(to_free != NULL)
	{
		*link = (*link)->next;
		allocation_count--;
	};

	return to_free;
}

static void track_headroom(void)
{
	size_t largest_free = heaps_platform_largest_free();
	if(largest_free < headroom)
		headroom = largest_free;
}

#ifdef heaps_platform_realloc

static heaps_report_t* report(int* arr_size)
{
	heaps_report_t* arr = NULL;
	int size = 0;
	heaps_t* link = head;
	int i;
	bool found = false;
	bool failed = false;	// allows us to fail gracefully without leaking memory if realloc fails for the report and no error handler is provided

	while(link && !failed)
	{
		i = size;
		found = false;
		while(!found && i--)
			found = (!strcmp(arr[i].file, link->file) && (arr[i].line == link->line));

		if(found)
		{
			arr[i].count++;
			arr[i].size += link->size;
		}
		else
			failed = add_to_report(&arr, &size, link);

		link = link->next;
	};

	// add the reports allocation (which must be at the head) to the report.
	// the array is already oversized by 1 to allow for this 
	if(size & !failed)
	{
		arr[size].count = 1;
		arr[size].size = head->size;
		arr[size].file = head->file;
		arr[size].line = head->line;
		size++;
	};
	
	if(arr_size)
		*arr_size = size;
	return arr;
}

static bool add_to_report(heaps_report_t** dst, int* dst_size, heaps_t* src)
{
	heaps_report_t* arr = *dst;
	heaps_report_t* new_arr;
	int i = *dst_size;
	bool failed;
	(*dst_size)++;

	new_arr = realloc_(arr, ((*dst_size)+1) * sizeof(heaps_report_t), __FILE__, __LINE__);	// maintain array oversized by one, so we can add the head at the end
	failed = (new_arr == NULL);
	if(failed)
	{
		arr = free_(arr, __FILE__, __LINE__);
		*dst_size = 0;
	}
	else
	{
		arr = new_arr;
		arr[i].count = 1;
		arr[i].size = src->size;
		arr[i].file = src->file;
		arr[i].line = src->line;
	}
	*dst = arr;
	return failed;
}

#endif
#endif
