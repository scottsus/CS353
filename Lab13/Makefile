MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"
LAB1 = lab13a
LAB2 = lab13b
SOURCES = $(wildcard my_*.cpp)
HEADERS = $(wildcard my*.h)

all: $(LAB1) $(LAB2)

clean:
	rm -f *.out $(LAB1) $(LAB2) x y

$(LAB1): $(LAB1).cpp $(SOURCES) $(HEADERS)
	g++ ${MYDEFS} -o $(LAB1) $(LAB1).cpp $(SOURCES) -lcrypto -pthread

$(LAB2): $(LAB2).cpp $(SOURCES) $(HEADERS)
	g++ ${MYDEFS} -o $(LAB2) $(LAB2).cpp $(SOURCES) -lcrypto -pthread

