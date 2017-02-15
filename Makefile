PROG = ims
CC = g++
OBJDIR = obj
CFLAGS = -g -D_DEBUG -pedantic -std=c++11
LDFLAGS = -lsimlib -lm
WARNCFLAGS = -Wcast-align -Wall -Wshadow -Wpointer-arith -Wcast-qual
WARNLD = -Wbad-function-cast -Wmissing-declarations -Wstrict-prototypes 
SOURCES = $(notdir $(wildcard *.cpp))
OBJECTS = $(patsubst %.cpp,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all

all: $(PROG)
auto: $(PROG) run

$(PROG): $(OBJECTS)
	$(CC) $(WARNCFLAGS) $(WARNLD) $(CFLAGS) -o $(PROG) $(OBJECTS) $(LDFLAGS)
	
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	@mkdir -p out
	$(CC) $(WARNCFLAGS) $(CFLAGS) -c $< -o $@

clean: 
	rm -f $(OBJECTS) $(PROG)
	rm -r $(OBJDIR)
	rm -rf out

run:
	./ims