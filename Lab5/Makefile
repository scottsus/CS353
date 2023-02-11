MYDEFS = -g -Wall -std=c++11 -DLOCALHOST=\"127.0.0.1\"

all: split lab5a lab5b test lab5c lab5d

split: split.cpp
	g++ ${MYDEFS} -o split split.cpp 

lab5a: lab5a.cpp
	g++ ${MYDEFS} -o lab5a lab5a.cpp

lab5b: logging.cpp
	g++ ${MYDEFS} -o lab5b logging.cpp

test:
	rm -f xyzzy xyzzy.log
	./lab5b > xyzzy
	./lab5b xyzzy.log
	diff xyzzy xyzzy.log
	cat xyzzy.log
	xxd -g 1 xyzzy.log

lab5c: md5-calc.cpp
	g++ ${MYDEFS} md5-calc.cpp -o lab5c -lcrypto

lab5d: progress.cpp my_timestamp.cpp my_timestamp.h
	g++ ${MYDEFS} -o lab5d progress.cpp my_timestamp.cpp

clean:
	rm -f xyzzy xyzzy.log split lab5a lab5b lab5c lab5d

