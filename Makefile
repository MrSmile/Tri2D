
HEADER = vec2d.h
SOURCE = main.cpp
FLAGS = -std=c++11 -INTL/include -Wall -Wno-parentheses
LIBS =
PROGRAM = tri2d


debug: $(SOURCE) $(HEADER)
	g++ -g -O0 -DDEBUG $(FLAGS) $(SOURCE) $(LIBS) -o $(PROGRAM)

release: $(SOURCE) $(HEADER)
	g++ -Ofast -flto -mtune=native $(FLAGS) $(SOURCE) $(LIBS) -o $(PROGRAM)

clean:
	rm $(PROGRAM)
