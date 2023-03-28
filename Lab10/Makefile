MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"
LAB10A = lab10a
SOURCES = $(wildcard my_*.cpp)
HEADERS = $(wildcard my*.h)

.PHONY: clean

clean:
	rm -f *.out $(LAB10A) x y

$(LAB10A): lab10a.cpp $(SOURCES) $(HEADERS)
	g++ ${MYDEFS} -o $(LAB10A) $^ -lcrypto -pthread
