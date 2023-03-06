MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

clean:
	rm -f *.out pa3 x y

pa3: pa3.cpp my_utils.cpp my_utils.h my_server.cpp my_utils.h my_client.cpp my_client.h my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h my_timestamp.cpp my_timestamp.h my_connection.cpp my_connection.h
	g++ ${MYDEFS} -o pa3 pa3.cpp my_utils.cpp my_server.cpp my_client.cpp my_socket.cpp my_readwrite.cpp my_timestamp.cpp my_connection.cpp -lcrypto -pthread
