// Bench 2 (Bun / Node.js): High-frequency compute (fib(38)) + 10M Uint32Array hash reduction
const ARRAY_LEN = 1_000_000;
const ROUNDS = 10;

function fib(n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

const nowNs = typeof Bun !== "undefined"
  ? () => BigInt(Bun.nanoseconds())
  : () => process.hrtime.bigint();

const t0 = nowNs();
const fibRes = fib(38);
const t1 = nowNs();

const buf = new Uint32Array(ARRAY_LEN);
const t2 = nowNs();
for (let i = 0; i < ARRAY_LEN; i++) {
  buf[i] = Math.imul(i, 2654435761) >>> 0;
}

let acc = 2166136261 >>> 0;
for (let r = 0; r < ROUNDS; r++) {
  for (let j = 0; j < ARRAY_LEN; j++) {
    acc = Math.imul((acc ^ buf[j]) >>> 0, 16777619) >>> 0;
  }
}
const t3 = nowNs();

const fib_ms = Number(t1 - t0) / 1e6;
const reduce_ms = Number(t3 - t2) / 1e6;
const total_ms = fib_ms + reduce_ms;

console.log(JSON.stringify({
  fib_ms: Number(fib_ms.toFixed(3)),
  reduce_ms: Number(reduce_ms.toFixed(3)),
  total_ms: Number(total_ms.toFixed(3)),
  fib_res: fibRes,
  reduce_hash: acc,
}));
