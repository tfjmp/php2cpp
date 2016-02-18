CCC = g++

install:
	mkdir -p build
	cd ./build && git clone https://github.com/uncrustify/uncrustify.git
	cd ./build/uncrustify && ./configure && make
	cp ./build/uncrustify/src/uncrustify ./uncrustify
	
all:
	$(CCC) src/php2cpp.cpp $(LIB) -o php2cpp

clean:
	rm -f php2cpp
