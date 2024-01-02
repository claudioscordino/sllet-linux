CPP=g++
CPPFLAGS= -Iperiodic -Inet --std=c++17 -Wall -Wextra -pthread

main: app/main.o net/Proxy.o net/Skeleton.o
	$(CPP) $(CPPFLAGS) app/main.o net/Proxy.o net/Skeleton.o -o main

app/main.o: app/main.cpp
	$(CPP) $(CPPFLAGS) -c app/main.cpp -o app/main.o

net/Proxy.o: net/Proxy.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -c net/Proxy.cpp -o net/Proxy.o

net/Skeleton.o: net/Skeleton.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -c net/Skeleton.cpp -o net/Skeleton.o

.PHONY: clean

clean:
	rm -f main a.out app/*.o net/*.o
