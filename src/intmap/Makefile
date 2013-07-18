all : intmap_test intmap.o test_map

intmap_test : intmap.h intmap.c ../log/Debug.c
	gcc -o intmap_test intmap.c ../log/Debug.c -I../log -Wall  -DPRO_TEST -Wall -g

intmap.o : intmap.h intmap.c
	gcc -c intmap.c ../log/Debug.c -I../log -Wall -g

test_map : map_test.cpp 
	g++ -o map_test map_test.cpp ../log/Debug.c -I../log -g

clean :
	rm intmap_test intmap.o map_test
