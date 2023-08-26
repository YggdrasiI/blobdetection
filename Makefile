all: release install_build

# Compile lib and examples into ./build
release: make_build
debug: make_debug
test_coverage: make_debug_with_tests gen_coverage_report_tests

# Differnt build types
build/CMakeCache.txt: mkdir_build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../dest ..

debug/CMakeCache.txt: mkdir_debug
	cd debug && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../dest ..

debug_with_tests/CMakeCache.txt: mkdir_debug_with_tests
	cd debug_with_tests && cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=1 \
		-DCMAKE_C_FLAGS=--coverage -g -O0 -DCMAKE_CXX_FLAGS=--coverage -g -O0 \
		..

# Artifical targets
make_%: mkdir_% %/CMakeCache.txt %/src/libdepthtree.so
	@echo "" # Dummy

mkdir_%:
	@test -d "./$*" || mkdir "./$*"

%/src/libdepthtree.so:
	cd "$*" && make

# Generate test coverage report
gen_coverage_report_%:
	cd "./$*" \
		&& gcovr -r .. -o covr-report.html --html-details \
		--filter "../src/*" --filter "../include/*" \
		--exclude-directories tests

show_coverage_report_%:
	cd "./$*" \
		&& nohup sh -c 'sleep 2; xdg-open "http://127.0.0.1:8001/covr-report.html"' 2>/dev/null \
		&& python -m http.server 8001


# Install files into ./dest
install_%: 
	@test -d "./$*" || ( echo "Folder $* contains nothing to install?!" && false )
	cd "./$*" && make install && echo "Library installed"

clean:
	rm -rf ./build ./debug ./dest ./debug_with_tests

.PHONY: all clean release debug debug_with_tests
