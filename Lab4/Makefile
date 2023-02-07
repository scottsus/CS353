MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

all: lab4a

lab4a: lab4a.cpp my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h
	g++ ${MYDEFS} -o lab4a lab4a.cpp my_socket.cpp my_readwrite.cpp

clean:
	rm -f *.o lab4a

