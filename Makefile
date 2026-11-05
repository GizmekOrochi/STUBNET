CMAKE ?= cmake
PYTHON ?= python3

.PHONY: all test tests test-lan test-asan test-ubsan test-all release-check run run-alpha run-beta clean

all:
	$(CMAKE) -S . -B build
	$(CMAKE) --build build -j2

tests: all

test: tests
	./build/stubnet_tests

test-lan: all
	bash scripts/dev-certs.sh
	$(PYTHON) tests/test_lan.py

# Separate instrumented builds: the library AND the tests must be instrumented.
test-asan:
	$(CMAKE) -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O0 -g -fno-omit-frame-pointer -fsanitize=address" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
	$(CMAKE) --build build-asan --target stubnet_tests -j2
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./build-asan/stubnet_tests

test-ubsan:
	$(CMAKE) -S . -B build-ubsan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=undefined" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined"
	$(CMAKE) --build build-ubsan --target stubnet_tests -j2
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./build-ubsan/stubnet_tests

test-all: test test-asan test-ubsan test-lan
release-check: test-all

run: run-alpha
run-alpha: all
	bash scripts/dev-certs.sh
	./build/stubnet_demo alpha 42000 certs/alpha.pem certs/alpha.key certs/ca.pem
run-beta: all
	bash scripts/dev-certs.sh
	./build/stubnet_demo beta 42002 certs/beta.pem certs/beta.key certs/ca.pem
clean:
	rm -rf build build-asan build-ubsan
