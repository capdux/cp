// Bench 1 (Bun): SQLite :memory: 100,000 batch inserts + 100,000 primary-key point lookups
import { Database } from "bun:sqlite";

const ROWS = 100000;
const LOOKUPS = 100000;

const db = new Database(":memory:");
db.run("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, score INTEGER);");

const payload = "agent_item_payload";

const t0 = Bun.nanoseconds();
const ins = db.prepare("INSERT INTO items (id, name, score) VALUES (?, ?, ?);");
const insertAll = db.transaction(() => {
  for (let i = 1; i <= ROWS; i++) {
    ins.run(i, payload, i * 7);
  }
});
insertAll();
const t1 = Bun.nanoseconds();

const sel = db.prepare("SELECT score, name FROM items WHERE id = ?;");
let checksum = 0;
for (let k = 0; k < LOOKUPS; k++) {
  const targetId = ((k * 7919) % ROWS) + 1;
  const row = sel.get(targetId);
  if (row) {
    checksum += row.score + row.name.length;
  }
}
const t2 = Bun.nanoseconds();
db.close();

const write_ms = (t1 - t0) / 1e6;
const read_ms = (t2 - t1) / 1e6;
const total_ms = write_ms + read_ms;

console.log(JSON.stringify({
  write_ms: Number(write_ms.toFixed(3)),
  read_ms: Number(read_ms.toFixed(3)),
  total_ms: Number(total_ms.toFixed(3)),
  checksum,
}));
