# Bench 2 (Python3): High-frequency compute (fib(38)) + 10M u32 array hash reduction
import array
import json
import time

ARRAY_LEN = 1_000_000
ROUNDS = 10
MASK32 = 0xFFFFFFFF


def fib(n: int) -> int:
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)


t0 = time.perf_counter_ns()
fib_res = fib(38)
t1 = time.perf_counter_ns()

t2 = time.perf_counter_ns()
buf = array.array("I", ((i * 2654435761) & MASK32 for i in range(ARRAY_LEN)))

acc = 2166136261
for _ in range(ROUNDS):
    for v in buf:
        acc = ((acc ^ v) * 16777619) & MASK32
t3 = time.perf_counter_ns()

fib_ms = (t1 - t0) / 1e6
reduce_ms = (t3 - t2) / 1e6
total_ms = fib_ms + reduce_ms

print(json.dumps({
    "fib_ms": round(fib_ms, 3),
    "reduce_ms": round(reduce_ms, 3),
    "total_ms": round(total_ms, 3),
    "fib_res": fib_res,
    "reduce_hash": acc,
}))
