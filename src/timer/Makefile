flags = -Wall -Werror -g
all_target = Heap.o test_heap
all : ${all_target}

test_heap : Heap.h Heap.c
	gcc -o test_heap Heap.c ${flags} -DTEST_HEAP

Heap.o : Heap.h Heap.c
	gcc -c Heap.c ${flags}


clean : 
	rm ${all_target}
