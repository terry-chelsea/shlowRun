flags = -Wall -Werror -g
all_target = Heap.o test_heap heap_sort
all : ${all_target}

heap_sort : Heap.h Heap.c heap_sort.c
	gcc -o heap_sort heap_sort.c Heap.c ${flags} -std=c99

test_heap : Heap.h Heap.c
	gcc -o test_heap Heap.c ${flags} -DTEST_HEAP

Heap.o : Heap.h Heap.c
	gcc -c Heap.c ${flags}


clean : 
	rm ${all_target}
