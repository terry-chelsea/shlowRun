flags = -g -O0 -Wall -Werror
all = thread_test
list : thread_test.c queue.o
	gcc -o thread_test thread_test.c queue.o ${flags} -lpthread -DTHREADS -DDEBUG

queue.o : queue.h queue.c
	gcc -c queue.c ${flags} -lpthread -DDOUBLE_LOCK_

clean :
	rm ${all}
