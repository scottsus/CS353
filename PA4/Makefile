MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"
LAB = lab11a
SOURCES = $(wildcard my_*.cpp)
HEADERS = $(wildcard my*.h)

all: $(LAB)

clean:
	rm -f *.out $(LAB) x y

$(LAB): $(LAB).cpp $(SOURCES) $(HEADERS)
	g++ ${MYDEFS} -o $(LAB) $(LAB).cpp $(SOURCES) -lcrypto -pthread

