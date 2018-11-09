
THRESH_SOURCES=blob.c threshtree.c tree.c threshtree_old.c threshtree_output.c
DEPTH_SOURCES=blob.c depthtree.c tree.c depthtree_output.c

# -fpic required for shared libs.
# CFLAGS=-std=gnu11 -Wall -fpic -O3 -lm

# Debug
CFLAGS=-std=gnu11 -Wall -g -O0 -fmax-errors=3 -w -fpic


libthreshtree.a: $(THRESH_SOURCES:.c=.o) | *.h
	ar rcs libthreshtree.a $^

libthreshtree.so: $(THRESH_SOURCES:.c=.o) | *.h
	LANG=C gcc -shared -o $@ $(CFLAGS) $^ 

libdepthtree.a: $(DEPTH_SOURCES:.c=.o) | *.h
	ar rcs libdepthtree.a $^

%.o: %.c | *.h
	LANG=C $(CC) $(CFLAGS) -c -o $@ $<


threshtree: libthreshtree.a
	echo "Build $(@)"

depthtree: libdepthtree.a
	echo "Build $(@)"

example1.bin: example1.c example.h | libthreshtree.so
	LANG=C $(CC) -o example1.bin $^ $(CFLAGS) -L. -lthreshtree

example2.bin: example2.c example.h libdepthtree.a
	LANG=C $(CC) -o example2.bin $^ $(CFLAGS)

example1_static.bin: example1.c libthreshtree.a | example.h 
	LANG=C $(CC) -o example1_static.bin $^ $(CFLAGS)

example2_static.bin: example2.c libdepthtree.a example.h 
	LANG=C $(CC) -o example2_static.bin $^ $(CFLAGS)


clean:
	rm *.bin *.a *.so *.o 2>/dev/null
