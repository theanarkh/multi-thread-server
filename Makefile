build: main.cc http-parser.cc
	g++ main.cc http-parser.cc -lllhttp -Lbuild -std=c++11 -o multi-thread-server
	