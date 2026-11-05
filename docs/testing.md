# Testing Sutbnet (NovaC-style)

The unit-test framework (`tests/tester.hpp`) is reused from NovaC. Each test
uses `TEST(Suite, Name)`, `CHECK(...)`, `CHECK_EQ(...)` or `CHECK_NEAR(...)`.
The test runner registers cases across translation units, reports their
individual results and exits nonzero if any case fails.

## Test matrix

| Command | What it checks |
| --- | --- |
| `make test` | Static library + 22 registered C++ tests |
| `make test-lan` | Two processes, multicast discovery, TLS mutual authentication and broadcast |
| `make test-asan` | Rebuild library/tests with AddressSanitizer + leak detection |
| `make test-ubsan` | Rebuild library/tests with UndefinedBehaviorSanitizer |
| `make release-check` | All of the above |

`make test-lan` needs Python 3, OpenSSL command-line utilities and working
multicast loopback. Development keys are generated in `certs/` and must never
be committed or used in production.

The sanitizers use **separate CMake build directories**, because linking an
instrumented test binary against a non-instrumented library would leave part
of the code unchecked.

## Coverage and gaps

Current suites cover discovery announcements (including truncated and
malformed inputs), peer registry operations and concurrent inserts, event
callback replacement, reentrant subscription and concurrent dispatch, TLS
certificate input checks, randomized invalid discovery input and the test
framework's floating-point comparisons.

Still to add before claiming production readiness: rejection of rogue CA
certificates at the network boundary, reconnect/shutdown race tests,
slow/malicious TLS peers, cross-machine LAN tests, fuzzing, ThreadSanitizer
where supported, and installation/package-consumer smoke tests.
