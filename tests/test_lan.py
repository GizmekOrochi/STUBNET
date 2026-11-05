"""Optional end-to-end LAN smoke test. Run with: make test-lan.

Requires multicast loopback support, development certificates, and Python 3.
"""
import pathlib
import queue
import subprocess
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
EXEC = ROOT / 'build' / 'stubnet_demo'


def start_peer(name, port):
    process = subprocess.Popen(
        [str(EXEC), name, str(port), f'certs/{name}.pem',
         f'certs/{name}.key', 'certs/ca.pem'],
        cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, bufsize=1,
    )
    output = queue.Queue()

    def reader():
        for line in process.stdout:
            output.put(line.rstrip())

    threading.Thread(target=reader, daemon=True).start()
    return process, output


def wait_for(output, fragment, timeout=12):
    deadline = time.monotonic() + timeout
    lines = []
    while time.monotonic() < deadline:
        try:
            line = output.get(timeout=min(0.3, max(0.01, deadline - time.monotonic())))
        except queue.Empty:
            continue
        lines.append(line)
        if fragment in line:
            return
    raise RuntimeError(f'Missing {fragment!r}; observed: {lines}')


def main():
    a, out_a = start_peer('alpha', 42000)
    b, out_b = start_peer('beta', 42002)
    try:
        wait_for(out_a, 'beta v1 connected')
        wait_for(out_b, 'alpha v1 connected')
        a.stdin.write('send HELLO_TEST\n')
        a.stdin.flush()
        wait_for(out_b, '[build-ready] alpha: HELLO_TEST')
        print('LAN integration test passed (discovery, TLS, callbacks, broadcast)')
    finally:
        for p in (a, b):
            if p.poll() is None:
                try:
                    p.stdin.write('quit\n')
                    p.stdin.flush()
                    p.wait(timeout=4)
                except (BrokenPipeError, subprocess.TimeoutExpired):
                    p.kill()
                    p.wait(timeout=4)


if __name__ == '__main__':
    main()
