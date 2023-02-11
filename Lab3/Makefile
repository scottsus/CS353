MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

all: echo-server echo-client lab3c lab3d

echo-server: echo-server.cpp my_socket.cpp my_socket.h
	g++ ${MYDEFS} -o echo-server echo-server.cpp my_socket.cpp

echo-client: echo-client.cpp my_socket.cpp my_socket.h
	g++ ${MYDEFS} -o echo-client echo-client.cpp my_socket.cpp

lab3c: lab3c.cpp my_socket.cpp my_socket.h
	g++ ${MYDEFS} -o lab3c lab3c.cpp my_socket.cpp

lab3d: lab3d.cpp my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h
	g++ ${MYDEFS} -o lab3d lab3d.cpp my_socket.cpp my_readwrite.cpp

run-server:
	./echo-server 12345

run-client:
	./echo-client 12345

clean:
	rm -f *.o echo-server echo-client lab3c lab3d

