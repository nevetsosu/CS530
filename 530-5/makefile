OBJS= one two three four

all: $(OBJS)

%: %.c
	gcc -O3 -o $@ $<

clean:
	rm -f $(OBJS)
