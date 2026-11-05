.PHONY: all run run-alpha run-beta test test-lan clean

all:
	cmake -S . -B build
	cmake --build build -j2

test: all
	ctest --test-dir build --output-on-failure

test-lan: all
	bash scripts/dev-certs.sh
	python3 tests/test_lan.py

run: run-alpha

run-alpha: all
	bash scripts/dev-certs.sh
	./build/stubnet_demo alpha 42000 certs/alpha.pem certs/alpha.key certs/ca.pem

run-beta: all
	bash scripts/dev-certs.sh
	./build/stubnet_demo beta 42002 certs/beta.pem certs/beta.key certs/ca.pem

clean:
	rm -rf build
