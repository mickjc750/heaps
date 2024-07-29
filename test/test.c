
	#include <stdio.h>
    #include <stdlib.h>

    #include "greatest.h"
    #include "../heaps.h"

    #ifdef HEAPS_REALLOC_ZERO_DOESNT_FREE
        #error "Sorry but HEAPS_REALLOC_ZERO_DOESNT_FREE isn't supported by the tests" 
    #endif

//********************************************************************************************************
// Local defines
//********************************************************************************************************

	GREATEST_MAIN_DEFS();

    typedef struct err_info_t
    {
        const char* msg;
        const char* file;
        int line;
    } err_info_t;

//********************************************************************************************************
// Private variables
//********************************************************************************************************

    static err_info_t err_info;

//********************************************************************************************************
// Private prototypes
//********************************************************************************************************

	SUITE(suite_all_tests);
	TEST test_gen_linked_list(void);
	TEST test_err_on_alloc_fail(void);
	TEST test_err_on_realloc_fail(void);
	TEST test_err_on_bad_free(void);
    TEST test_track_headroom(void);
    TEST test_track_peak_allocation_count(void);
    TEST test_calloc(void);
    TEST test_reports(void);


//********************************************************************************************************
// Public functions
//********************************************************************************************************

void test_error_handler(const char* msg, const char* file, int line)
{
    err_info.msg = msg;
    err_info.file = file;
    err_info.line = line;
}

int main(int argc, const char* argv[])
{
	GREATEST_MAIN_BEGIN();
	RUN_SUITE(suite_all_tests);
	GREATEST_MAIN_END();

	return 0;
}

//********************************************************************************************************
// Private functions
//********************************************************************************************************

SUITE(suite_all_tests)
{
	RUN_TEST(test_gen_linked_list);
	RUN_TEST(test_err_on_alloc_fail);
	RUN_TEST(test_err_on_realloc_fail);
    RUN_TEST(test_err_on_bad_free);
    RUN_TEST(test_track_headroom);
    RUN_TEST(test_track_peak_allocation_count);
    RUN_TEST(test_calloc);
    RUN_TEST(test_reports);
}

TEST test_gen_linked_list(void)
{
    heaps_t* ptr = NULL;
    heaps_t* old_head = heaps_get_allocation_list();
    void *a,*b,*c;
    a = heaps_alloc_(101, "file-one", 1);
    b = heaps_alloc_(102, "file-two", 2);
    c = heaps_alloc_(103, "file-three", 3);
    ptr = heaps_get_allocation_list();
    ASSERT(ptr);
    ASSERT_STR_EQ("file-three", ptr->file);
    ASSERT_EQ(3, ptr->line);
    ASSERT_EQ(103, ptr->size);
    ptr = ptr->next;
    ASSERT(ptr);
    ASSERT_STR_EQ("file-two", ptr->file);
    ASSERT_EQ(2, ptr->line);
    ASSERT_EQ(102, ptr->size);
    ptr = ptr->next;
    ASSERT(ptr);
    ASSERT_STR_EQ("file-one", ptr->file);
    ASSERT_EQ(1, ptr->line);
    ASSERT_EQ(101, ptr->size);
    ptr = ptr->next;
    ASSERT_EQ(old_head, ptr);

//  remove the middle allcoation, and re-test the list is what it should be
    heaps_free(b);
    ptr = heaps_get_allocation_list();
    ASSERT(ptr);
    ASSERT_STR_EQ("file-three", ptr->file);
    ASSERT_EQ(3, ptr->line);
    ASSERT_EQ(103, ptr->size);
    ptr = ptr->next;
    ASSERT(ptr);
    ASSERT_STR_EQ("file-one", ptr->file);
    ASSERT_EQ(1, ptr->line);
    ASSERT_EQ(101, ptr->size);
    ptr = ptr->next;
    ASSERT_EQ(old_head, ptr);
 
//  remove the last allocation, and re-test the list is what it should be
    heaps_free(c);
    ptr = heaps_get_allocation_list();
    ASSERT(ptr);
    ASSERT_STR_EQ("file-one", ptr->file);
    ASSERT_EQ(1, ptr->line);
    ASSERT_EQ(101, ptr->size);
    ptr = ptr->next;
    ASSERT_EQ(old_head, ptr);

//  remove the first allocation, and re-test the list is what it should be
    heaps_free(a);
    ptr = heaps_get_allocation_list();
    ASSERT_EQ(old_head, ptr);

    PASS();
}

TEST test_err_on_alloc_fail(void)
{
    void* a;
    a = heaps_alloc_(MCHEAP_SIZE+1, "fred likes dogs", 1975);
    ASSERT_EQ(NULL, a);
    ASSERT_STR_EQ("fred likes dogs", err_info.file);
    ASSERT_STR_EQ("allocation failed", err_info.msg);
    ASSERT_EQ(1975, err_info.line);
    err_info = (err_info_t){.file ="", .line=0, .msg=""};
    PASS();
}

TEST test_err_on_realloc_fail(void)
{
    void* a = NULL;
    void* b;
 
    a = heaps_realloc_(a, MCHEAP_SIZE+1, "bob eats chickens", 1984);
    ASSERT_EQ(NULL, a);
    ASSERT_STR_EQ("bob eats chickens", err_info.file);
    ASSERT_STR_EQ("allocation via heaps_realloc() failed", err_info.msg);
    ASSERT_EQ(1984, err_info.line);
    err_info = (err_info_t){.file ="", .line=0, .msg=""};

    a = heaps_alloc(50);
    ASSERT_NEQ(NULL, a);
    b = heaps_realloc_(a, MCHEAP_SIZE, "turtle broth", 2001);
    ASSERT_EQ(NULL, b);
    ASSERT_STR_EQ("turtle broth", err_info.file);
    ASSERT_STR_EQ("heaps_realloc() failed", err_info.msg);
    ASSERT_EQ(2001, err_info.line);
    err_info = (err_info_t){.file ="", .line=0, .msg=""};

    b = heaps_realloc_(a + 1, 0, "trying to false free", 2019);
    ASSERT_EQ(NULL, b);
    ASSERT_STR_EQ("trying to false free", err_info.file);
    ASSERT_STR_EQ("false free via heaps_realloc()", err_info.msg);
    ASSERT_EQ(2019, err_info.line);
    err_info = (err_info_t){.file ="", .line=0, .msg=""};

    heaps_free(a);
    PASS();
}

TEST test_err_on_bad_free(void)
{
    void* a = heaps_alloc(1);
    ASSERT_NEQ(NULL, a);
    heaps_free_(a-1, "trying false free", 1989);
    ASSERT_STR_EQ("trying false free", err_info.file);
    ASSERT_STR_EQ("false free", err_info.msg);
    ASSERT_EQ(1989, err_info.line);
    err_info = (err_info_t){.file ="", .line=0, .msg=""};
    heaps_free(a);
    PASS();
}

TEST test_track_headroom(void)
{
    void* a;
    size_t s;

    ASSERT_LT((MCHEAP_SIZE/2), heaps_get_headroom());   //expect current headroom to be more than 1/2 the heap size
    a = heaps_alloc(MCHEAP_SIZE+1);
    s = heaps_get_headroom();
    ASSERT_LT((MCHEAP_SIZE/2), s);     // expect headroom to be < 1/2 the heap size
    heaps_free(a);
    ASSERT_EQ(s, heaps_get_headroom()); // expect headroom not to change on free() 

    PASS();
}

TEST test_track_peak_allocation_count(void)
{
    int a = heaps_get_allocation_count_peak();

    void* b;
    void* c;
    void* d;
    void* e;
    void* f;
    void* g;
    void* h;

    b = heaps_alloc(100);
    c = heaps_alloc(100);
    d = heaps_alloc(100);
    e = heaps_alloc(100);
    f = heaps_alloc(100);
    g = heaps_alloc(100);
    h = heaps_alloc(100);

    heaps_free(b);
    heaps_free(c);
    heaps_free(d);
    heaps_free(e);
    heaps_free(f);
    heaps_free(g);
    heaps_free(h);

    ASSERT(a < 7);
    ASSERT_EQ(7, heaps_get_allocation_count_peak());
    PASS();
}

TEST test_calloc(void)
{
    uint8_t zeros[200] = {0};
    uint8_t* buf = heaps_calloc(100, 2);
    ASSERT(!memcmp(buf, zeros, 200));
    heaps_free(buf);
    PASS();
}

TEST test_reports(void)
{
    void* a1;
    void* b1;
    void* b2;
    void* c1;
    void* c2;
    void* c3;

    heaps_report_t* arr;
    int arr_size = -1;
    arr = heaps_report(&arr_size);  //call with NO allocations made
    ASSERT_EQ(NULL, arr);
    ASSERT_EQ(0, arr_size);
    a1 = heaps_alloc_(3000, "fileA", 2001);
    b1 = heaps_alloc_(1000, "fileB", 2002);
    b2 = heaps_alloc_(1000, "fileB", 2002);
    c1 = heaps_alloc_(500, "fileC", 2003);
    c2 = heaps_alloc_(500, "fileC", 2003);
    c3 = heaps_alloc_(500, "fileC", 2003);

    arr = heaps_report(&arr_size);  //call with 6 allocations made from 3 sources
    
    ASSERT_EQ(4, arr_size);

    ASSERT_STR_EQ("fileA", arr[2].file);
    ASSERT(arr[2].count == 1);
    ASSERT(arr[2].line == 2001);
    ASSERT(arr[2].size == 3000);

    ASSERT_STR_EQ("fileB", arr[1].file);
    ASSERT(arr[1].count == 2);
    ASSERT(arr[1].line == 2002);
    ASSERT(arr[1].size == 2000);

    ASSERT_STR_EQ("fileC", arr[0].file);
    ASSERT(arr[0].count == 3);
    ASSERT(arr[0].line == 2003);
    ASSERT(arr[0].size == 1500);

    ASSERT_STR_EQ("../heaps.h", arr[3].file);

    qsort(arr, arr_size, sizeof(*arr), heaps_report_sorter_descending_size);

    ASSERT_STR_EQ("fileA", arr[0].file);
    ASSERT(arr[0].count == 1);
    ASSERT(arr[0].line == 2001);
    ASSERT(arr[0].size == 3000);

    ASSERT_STR_EQ("fileB", arr[1].file);
    ASSERT(arr[1].count == 2);
    ASSERT(arr[1].line == 2002);
    ASSERT(arr[1].size == 2000);

    ASSERT_STR_EQ("fileC", arr[2].file);
    ASSERT(arr[2].count == 3);
    ASSERT(arr[2].line == 2003);
    ASSERT(arr[2].size == 1500);

    ASSERT_STR_EQ("../heaps.h", arr[3].file);

    qsort(arr, arr_size, sizeof(*arr), heaps_report_sorter_descending_count);

    ASSERT_STR_EQ("fileC", arr[0].file);
    ASSERT(arr[0].count == 3);
    ASSERT(arr[0].line == 2003);
    ASSERT(arr[0].size == 1500);

    ASSERT_STR_EQ("fileB", arr[1].file);
    ASSERT(arr[1].count == 2);
    ASSERT(arr[1].line == 2002);
    ASSERT(arr[1].size == 2000);

    ASSERT(arr[2].count == 1);  // fileA and the report itself could be in either order
    ASSERT(arr[3].count == 1);

    heaps_free(arr);

    PASS();
}