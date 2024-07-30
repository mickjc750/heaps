HEAPS

A memory safety layer that can be added to an existing memory allocator.
Intended for use on embedded platforms.

 * Single header (stb style).
 * Links all allocations with meta data containing the file:line which made them, and the size.
 * Generate reports (array of structs), indicating the size used and allocation count of each file:line source location.
 * Provides comparator functions for sorting reports with qsort().
 * Tracks the allocation count, peak allocation count, and largest allocation made.
 * If the allocator can report it's free space, Heaps can track the minimum free space which has ocurred (headroom).
 * Test suite using https://github.com/silentbicycle/greatest (there's really not much to test... but it works). 


Heaps may be configured with an error handler, and will catch the following:

 * Failed attempt to allocate.
 * Failed attempt to reallocate.
 * Attempt to free an invalid address.
 * Heaps meta data broken, and if the allocator offers a test for it, heap integrity broken.

 An example is provided which demonstrates using Heaps on top of stdlib's malloc/free, and using regular assert.h as an error handler.


 Heaps came about after creating a bloated allocator which implemented all of the above features badly. It was realised that the role of error checking and statics tracking belongs in a layer above the allocator. So the allocator was refined and Heaps was created. That allocator is mcheap, and is used in the test suite.

