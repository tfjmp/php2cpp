all:
	../php2cpp.out example.php example.cpp _example
	g++ -c -I./ example.cpp -o example.o

clean:
	rm -f example.cpp
	rm -f example.o
