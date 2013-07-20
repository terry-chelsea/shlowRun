flags = -Wall -g
all_target = Heap.o test_heap heap_sort Timer.o test_timer more_timers expire_more
all : ${all_target}

heap_sort : Heap.h Heap.c heap_sort.c
	gcc -o heap_sort heap_sort.c Heap.c ${flags} -std=c99

test_heap : Heap.h Heap.c
	gcc -o test_heap Heap.c ${flags} -DTEST_HEAP

Heap.o : Heap.h Heap.c
	gcc -c Heap.c ${flags}

Timer.o : Heap.h Heap.c Timer.h Timer.c 
	gcc -c Timer.c ${flags}

test_timer : Timer.o timer_test.c
	gcc -o test_timer timer_test.c Timer.o Heap.o ../intmap/intmap.o ../log/Debug.c -I../log ${flags}

more_timers : Timer.o more_timer_test.c 
	gcc -o more_timers more_timer_test.c Timer.o Heap.o ../intmap/intmap.o ../log/Debug.c -I../log ${flags}

expire_more : Timer.o expire_some_test.c
	gcc -o expire_more expire_some_test.c Timer.o Heap.o ../intmap/intmap.o ../log/Debug.c -I../log ${flags} -lm

clean : 
	rm ${all_target}
