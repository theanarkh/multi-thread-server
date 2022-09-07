build: main.cc http-parser.cc
	g++ main.cc http-parser.cc -lllhttp -Lhttp -std=c++11 -o multi-thread-server
	