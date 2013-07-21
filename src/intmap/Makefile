all : intmap_test intmap.o test_map intmap_inside_test

intmap_test : intmap_test.c intmap.h intmap.c ../log/Debug.c
	gcc -o intmap_test intmap_test.c intmap.c ../log/Debug.c -I../log -Wall -g -O3

intmap_inside_test : intmap.h intmap.c ../log/Debug.c
	gcc -o inside_test intmap.c ../log/Debug.c -I../log -DTEST_INTMAP -Wall -g

intmap.o : intmap.h intmap.c
	gcc -c intmap.c ../log/Debug.c -I../log -Wall -g

test_map : map_test.cpp 
	g++ -o map_test map_test.cpp ../log/Debug.c -I../log -g -O3

clean :
	rm intmap_test intmap.o map_test inside_test
