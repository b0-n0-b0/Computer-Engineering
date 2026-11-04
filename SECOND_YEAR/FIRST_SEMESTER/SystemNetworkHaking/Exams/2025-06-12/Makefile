server: server.cc libflag.so
	g++ -no-pie -g -o server -fstack-protector -z relro -z now -z noexecstack server.cc libflag.so

libflag.so: libflag.c
	gcc -fPIC -pie -z relro -z now -z noexecstack -fstack-protector -shared -o libflag.so libflag.c
