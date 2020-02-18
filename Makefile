CFLAGS = -g -Wall
CC = gcc
OBJS = ush.o expand.o builtin.o # macro objs was made / one line needs to be changed
PROG = ush
DEPS = defn.h

all: $(PROG)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROG): $(OBJS) # link all o files to ush
	$(CC) $(CFLAGS) $(OBJS) -o ush

clean:
	rm -f *~ *.o $(PROG) core a.out
