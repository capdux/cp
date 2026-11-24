// Bench 4 (Bun / Node.js): JSON.parse — 10,000 parses of medium JSON payload
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const payload = fs.readFileSync(path.join(__dirname, "payload.json"), "utf8");
const ITERATIONS = 10_000;

const nowNs = typeof Bun !== "undefined"
  ? () => BigInt(Bun.nanoseconds())
  : () => process.hrtime.bigint();

let checksum = 0;
const t0 = nowNs();
for (let i = 0; i < ITERATIONS; i++) {
  const root = JSON.parse(payload);
  checksum += root.meta.request_id.length;
  const items = root.items;
  for (let j = 0; j < items.length; j++) {
    checksum += items[j].score;
  }
}
const t1 = nowNs();

const parse_ms = Number(t1 - t0) / 1e6;
console.log(JSON.stringify({
  parse_ms: Number(parse_ms.toFixed(3)),
  iterations: ITERATIONS,
  payload_bytes: Buffer.byteLength(payload, "utf8"),
  checksum,
}));
