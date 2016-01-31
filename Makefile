CCC = g++
CCFLAGS = -Werror

all:
	$(CCC) src/php2cpp.cpp $(LIB) -o php2cpp.out $(CCFLAGS)

clean:
	rm -f *.o
