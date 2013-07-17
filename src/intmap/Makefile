all : intmap_test intmap.o

intmap_test : intmap.h intmap.c ../log/Debug.c
	gcc -o intmap_test intmap.c ../log/Debug.c -I../log -Wall  -DTEST_INTMAP -Wall -g

intmap.o : intmap.h intmap.c
	gcc -c intmap.c ../log/Debug.c -I../log -Wall -g

clean :
	rm intmap_test intmap.o
