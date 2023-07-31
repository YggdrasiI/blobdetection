all: compile install

# Compile lib and examples into ./build
compile: build/Makefile

build/Makefile:
	@test -d build || mkdir build 
	cd build && cmake -DCMAKE_INSTALL_PREFIX=../dest ..
	cd build && make

# Install files into ./dest
install: compile
	@test -d dest || mkdir dest 
	cd build && make install && echo "Library can be found in ./dest"

clean:
	rm -rf ./build ./dest
