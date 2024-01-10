CPP=g++
## CPP=clang++
CPPFLAGS= -Iperiodic -Inet -Isdll -Iutils --std=c++17 -Wall -Wextra -pthread 

all: main-std main-time main-tasks

main-std: app/main-std.o net/Proxy-std.o net/Skeleton-std.o
	$(CPP) $(CPPFLAGS) app/main-std.o net/Proxy-std.o net/Skeleton-std.o -o main-std

app/main-std.o: app/main.cpp periodic/*
	$(CPP) $(CPPFLAGS) -c app/main.cpp -o app/main-std.o


net/Proxy-std.o: net/Proxy.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -c net/Proxy.cpp -o net/Proxy-std.o

net/Skeleton-std.o: net/Skeleton.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -c net/Skeleton.cpp -o net/Skeleton-std.o

###################################################

main-time: app/main-time.o net/Proxy-time.o net/Skeleton-time.o
	$(CPP) $(CPPFLAGS) -DSLLET_TS app/main-time.o net/Proxy-time.o net/Skeleton-time.o -o main-time

app/main-time.o: app/main.cpp periodic/*
	$(CPP) $(CPPFLAGS) -DSLLET_TS -c app/main.cpp -o app/main-time.o


net/Proxy-time.o: net/Proxy.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -DSLLET_TS -c net/Proxy.cpp -o net/Proxy-time.o

net/Skeleton-time.o: net/Skeleton.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -DSLLET_TS -c net/Skeleton.cpp -o net/Skeleton-time.o

###################################################

main-tasks: app/main-tasks.o net/Proxy-tasks.o net/Skeleton-tasks.o
	$(CPP) $(CPPFLAGS) -DSLLET app/main-tasks.o net/Proxy-tasks.o net/Skeleton-tasks.o -o main-tasks

app/main-tasks.o: app/main.cpp periodic/*
	$(CPP) $(CPPFLAGS) -DSLLET -c app/main.cpp -o app/main-tasks.o


net/Proxy-tasks.o: net/Proxy.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -DSLLET -c net/Proxy.cpp -o net/Proxy-tasks.o

net/Skeleton-tasks.o: net/Skeleton.cpp net/*.hpp
	$(CPP) $(CPPFLAGS) -DSLLET -c net/Skeleton.cpp -o net/Skeleton-tasks.o

.PHONY: clean

clean:
	rm -f app/*.o net/*.o main-std main-time main-tasks
