CC=g++
CL=clang++

all: *.cpp *.h ../lib/*.cpp ../lib/*.h
	${CC} *.cpp ../lib/*.cpp -o client -std=c++11 -lpthread

clang: *.cpp *.h ../lib/*.cpp ../lib/*.h
	${CL} *.cpp ../lib/*.cpp -o client -std=c++11 -lpthread

clean:
	rm client
