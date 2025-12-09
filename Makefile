appname := hearts

CXX := clang++
CXXFLAGS := -std=c++11 -g -O3

srcfiles := $(shell find . -name "*.cpp")
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

# Library objects (excluding main.o for tests)
libobjects := $(filter-out ./main.o, $(objects))

LDLIBS := -lpthread

all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS)

# Test targets
thread_pool_test: thread_pool_test.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

thread_pool_test.o: thread_pool_test.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

resource_leak_test: resource_leak_test.o $(libobjects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

resource_leak_test.o: resource_leak_test.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

tests: thread_pool_test resource_leak_test

run-tests: tests
	./thread_pool_test
	./resource_leak_test

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(objects) thread_pool_test thread_pool_test.o resource_leak_test resource_leak_test.o

dist-clean: clean
	rm -f *~ .depend

include .depend
