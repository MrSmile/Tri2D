
HEADER = vec2d.h delaunay.h
SOURCE = delaunay.cpp main.cpp
FLAGS = -std=c++11 -INTL/include -fno-exceptions -DGL_GLEXT_PROTOTYPES -Wall
LIBS = -lSDL -lGL
PROGRAM = tri2d


debug: $(SOURCE) $(HEADER)
	g++ -g -O0 -DDEBUG $(FLAGS) $(SOURCE) $(LIBS) -o $(PROGRAM)

release: $(SOURCE) $(HEADER)
	g++ -Ofast -flto -mtune=native $(FLAGS) $(SOURCE) $(LIBS) -o $(PROGRAM)

clean:
	rm $(PROGRAM)
