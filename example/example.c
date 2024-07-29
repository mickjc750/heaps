
	#include <stdio.h>
    #include <stdlib.h>
    #include <time.h>

    #include "../heaps.h"

    #define MAX_ALLOC  1024

int main(int argc, const char* argv[])
{
    (void)argc;(void)argv;
    heaps_report_t *report;
    heaps_t* info;

    int report_size;
    int i;

    srand(time(NULL)); // randomize seed
    printf("\n");
    printf("Making some allocations\n");

    //make some allocations
    heaps_alloc(rand() % MAX_ALLOC);
    heaps_alloc(rand() % MAX_ALLOC);heaps_alloc(rand() % MAX_ALLOC);
    heaps_alloc(rand() % MAX_ALLOC);heaps_alloc(rand() % MAX_ALLOC);heaps_alloc(rand() % MAX_ALLOC);heaps_alloc(rand() % MAX_ALLOC);
    heaps_alloc(rand() % MAX_ALLOC);
    heaps_alloc(rand() % MAX_ALLOC);heaps_alloc(rand() % MAX_ALLOC);heaps_alloc(rand() % MAX_ALLOC);

    printf("Generating Report\n");
    report = heaps_report(&report_size);

    printf("Sorting report by descending size\n");
    qsort(report, report_size, sizeof(*report), heaps_report_sorter_descending_size);
    i = 0;
    
    printf("\n\
---------------------------------------\n\
        file:line      count     size\n\
---------------------------------------\n\
");
    while(i != report_size)
    {
        printf("%12s:%-6i %8i %8zu\n", report[i].file, report[i].line, report[i].count, report[i].size);
        i++;
    };
    printf("\n\n");

    printf("Freeing all allocations\n\n");
    do
    {
        info = heaps_get_allocation_list();
        if(info)
        {
            printf("Freeing %zu bytes made at %s:%i\n", info->size, info->file, info->line);
            heaps_free(info->content);
        };
    }while (info);
    
	return 0;
}

