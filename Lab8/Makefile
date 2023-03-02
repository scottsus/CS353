MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

clean:
	rm -f a.out lab8a lab8b x y

lab8a: lab8a.cpp my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h my_timestamp.cpp my_timestamp.h my_connection.cpp my_connection.h
	g++ ${MYDEFS} -o lab8a lab8a.cpp my_socket.cpp my_readwrite.cpp my_timestamp.cpp my_connection.cpp -lcrypto -pthread

lab8b: lab8b.cpp my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h my_timestamp.cpp my_timestamp.h my_connection.cpp my_connection.h
	g++ ${MYDEFS} -o lab8b lab8b.cpp my_socket.cpp my_readwrite.cpp my_timestamp.cpp my_connection.cpp -lcrypto -pthread
