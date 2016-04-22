CC=g++

default:
	rm -rf sat
	$(CC) -o sat sat.cpp

gen:
	rm -rf gen
	$(CC) -o gen gen.cpp

clean:
	rm -rf gen sat

all:
	rm -rf gen sat
	$(CC) -o gen gen.cpp
	$(CC) -o sat sat.cpp
