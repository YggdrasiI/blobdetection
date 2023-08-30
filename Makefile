TEST_BUILD_DIR=debug_with_tests

help:
	@echo -e "Build targets:\n\n" \
	"\trelease\tCMake build in 'build' folder.\n" \
	"\tdebug\tCMake build in 'debug' folder.\n" \
	"\t\n" \
	"\ttest\tCMake build in 'debug_with_tests' folder.\n" \
	"\tgen_coverage_report\t Generate test coverage report with gcovr\n" \
	"\tshow_coverage_report\t Show test coverage report in browser\n" \
	"\t\n" \
	"\tinstall_{BUILD_FOLDER}\tInstall lib into ./dest\n" \
	"\t\n" \
	"\t\n" \

all: release install_build

# Compile lib and examples into ./build
release: make_build
	@echo ""

debug: make_debug
	@echo ""

# Just an alias
tests: test

test: make_$(TEST_BUILD_DIR) gen_coverage_report
	@echo ""

# Differnt build types
build/CMakeCache.txt: mkdir_build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../dest ..

debug/CMakeCache.txt: mkdir_debug
	cd debug && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../dest ..

$(TEST_BUILD_DIR)/CMakeCache.txt: mkdir_$(TEST_BUILD_DIR)
	cd $(TEST_BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=1 \
		-DCMAKE_C_FLAGS="--coverage -g -O0" -DCMAKE_CXX_FLAGS="--coverage -g -O0" \
		..

# Artifical targets
#cmake_%: mkdir_% %/CMakeCache.txt
#	@echo "" # Dummy

mkdir_%:
	@test -d "./$*" || mkdir "./$*"

make_%: %/CMakeCache.txt
	cd "$*" && make

#%/src/libdepthtree.so:
#	cd "$*" && make

# Generate test coverage report
gen_coverage_report: make_$(TEST_BUILD_DIR)
	cd "./$(TEST_BUILD_DIR)" \
		&& gcovr -r . -o covr-report.html --html-details \
		--filter "../src/*" --filter "../include/*" \
		--exclude-directories tests

show_coverage_report:
	cd "./$(TEST_BUILD_DIR)" \
		&& nohup sh -c 'sleep 2; xdg-open "http://127.0.0.1:8001/covr-report.html"' 2>/dev/null \
		&& python -m http.server 8001


# Install files into ./dest
install_%: 
	@test -d "./$*" || ( echo "Folder $* contains nothing to install?!" && false )
	cd "./$*" && make install && echo "Library installed"

clean:
	rm -rf ./build ./debug ./dest ./$(TEST_BUILD_DIR)

.PHONY: all clean release debug test tests

# This preventing make from delete its intermediate target
# Without, the make is ending with 'rm [...]/src/libdepthtree.so'
.PRECIOUS: %/src/libdepthtree.so
