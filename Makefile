
THRESH_SOURCES=blob.c threshtree.c tree.c threshtree_old.c output.c output_threshtree.c
DEPTH_SOURCES=blob.c depthtree.c tree.c output.c output_depthtree.c

# -fpic required for shared libs.
# CFLAGS=-std=gnu11 -Wall -fpic -O3 -lm

# Debug
CFLAGS=-std=gnu11 -Wall -g -O0 -fmax-errors=3 -w -fpic

all: example1_static.bin example2_static.bin

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
	rm -vf *.bin *.a *.so *.o
