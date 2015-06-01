CC=g++
CL=clang++

all: *.cpp *.h ../lib/*
	${CC} *.cpp ../lib/*.cpp -o client -std=c++11 -lpthread

clang: *.cpp *.h ../lib/*
	${CL} *.cpp ../lib/*.cpp -o client -std=c++11 -lpthread

clean:
	rm client
