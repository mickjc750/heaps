// *************************************
//  Define what is needed by heaps.h

    #include "mcheap.h"

//  mandatory
    #define heaps_platform_free(ptr)            mcheap_free(ptr)

//  only one of these is mandatory
    #define heaps_platform_alloc(size)          mcheap_allocate(size)
    #define heaps_platform_realloc(ptr, size)   mcheap_reallocate(ptr, size)

//  the below are optional
    #define heaps_platform_check()              mcheap_is_intact()
    #define heaps_platform_largest_free()       mcheap_largest_free()

//  as this is a test case the error handler simply passes info to the test, instead of aborting
    extern void test_error_handler(const char* msg, const char* file, int line);
    #define heaps_error_handler(msg,file,line)    test_error_handler(msg,file,line)

//  If you wished to configure heaps to use regular assert.h you would provide it like this:
//    #include <assert.h>
//    #define heaps_error_handler(msg,file,line)    __assert_fail(msg,file,line,__ASSERT_FUNCTION)


//  With HEAPS_IMPLEMENTATION defined heaps.h will provide the implementation (all functions)
    #define HEAPS_IMPLEMENTATION
    #include "../heaps.h"

