all:
	ruby ../prepare.rb example.php _tmp.php
	../php2cpp.out _tmp.php example.cpp _example
	g++ -c -I./ example.cpp -o example.o

clean:
	rm -f example.cpp
	rm -f example.o
