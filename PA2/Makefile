MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

all: pa2

pa2: pa2.cpp my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h my_timestamp.cpp my_timestamp.h
	g++ ${MYDEFS} -o pa2 pa2.cpp my_socket.cpp my_readwrite.cpp my_timestamp.cpp

clean:
	rm -f *.o pa2