MAIN := tyrant_optimize
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,obj/%.o,$(SRCS))
INCS := $(wildcard *.h)

CPPFLAGS := -Wall -Werror -std=gnu++11 -O3 -DTYRANT_UNLEASHED
LDFLAGS := -lboost_system -lboost_thread -lboost_filesystem

all: $(MAIN)

obj/%.o: %.cpp $(INCS)
	$(CXX) $(CPPFLAGS) -o $@ -c $<

$(MAIN): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(MAIN) obj/*.o
