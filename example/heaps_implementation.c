// *************************************
//  Define what is needed by heaps.h

    #include <stdlib.h>
    #include <assert.h>

//  mandatory
    #define heaps_platform_free(ptr)            free(ptr)

//  only one of these is mandatory
    #define heaps_platform_alloc(size)          malloc(size)
    #define heaps_platform_realloc(ptr, size)   realloc(ptr, size)

//  abort on error
    #define heaps_error_handler(msg,file,line)    __assert_fail(msg,file,line,__ASSERT_FUNCTION)

//  With HEAPS_IMPLEMENTATION defined heaps.h will provide the implementation (all functions)
    #define HEAPS_IMPLEMENTATION
    #include "../heaps.h"

