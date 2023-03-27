MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

clean:
	rm -f *.out lab10a x y && rm -rf 127.0.0.1:* dir2 dir3

lab10a: lab10a.cpp my_utils.cpp my_utils.h my_server.cpp my_server.h my_client.cpp my_client.h my_reaper.cpp my_reaper.h my_socket.cpp my_socket.h my_readwrite.cpp my_readwrite.h my_timestamp.cpp my_timestamp.h my_connection.cpp my_connection.h my_message.cpp my_message.h
	g++ ${MYDEFS} -o lab10a lab10a.cpp my_utils.cpp my_server.cpp my_client.cpp my_reaper.cpp my_socket.cpp my_readwrite.cpp my_timestamp.cpp my_connection.cpp my_message.cpp -lcrypto -pthread
