// Bench 1 (CP): SQLite :memory: 100,000 batch inserts + 100,000 primary-key point lookups
import { Database } from "cap:sqlite";
import { HwTimer } from "cap:bench";
import std from "std";

const ROWS: i64 = 100000;
const LOOKUPS: i64 = 100000;

fn runSqliteBench() {
    let db = Database.openMemory() catch return;
    defer db.close();

    db.exec("CREATE TABLE items (id INTEGER PRIMARY KEY, name TEXT, score INTEGER);") catch return;

    const payload: []const u8 = "agent_item_payload";

    // Phase 1: 100,000 batch inserts in a single transaction
    const t0 = HwTimer.nowNs();
    db.begin() catch return;
    {
        let ins = db.prepare("INSERT INTO items (id, name, score) VALUES (?, ?, ?);") catch return;
        defer ins.close();

        let i: i64 = 1;
        while (i <= ROWS) {
            ins.bindInt(1, i) catch return;
            ins.bindText(2, payload) catch return;
            ins.bindInt(3, i * 7) catch return;
            ins.run() catch return;
            i += 1;
        }
    }
    db.commit() catch return;
    const t1 = HwTimer.nowNs();

    // Phase 2: 100,000 deterministic pseudo-random primary-key point lookups
    let checksum: i64 = 0;
    {
        let sel = db.prepare("SELECT score, name FROM items WHERE id = ?;") catch return;
        defer sel.close();

        let k: i64 = 0;
        let target_id: i64 = 0;
        while (k < LOOKUPS) {
            target_id = @mod(k * 7919, ROWS) + 1;
            sel.bindInt(1, target_id) catch return;
            if (sel.step() catch false) {
                const score = sel.columnInt(0);
                const name = sel.columnText(1);
                checksum += score + @as(i64, @intCast(name.len));
            }
            sel.reset() catch return;
            k += 1;
        }
    }
    const t2 = HwTimer.nowNs();

    const write_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const read_ms: f64 = @as(f64, @floatFromInt(t2 - t1)) / 1_000_000.0;
    const total_ms: f64 = write_ms + read_ms;

    std.debug.print("{{\"write_ms\":{d:.3},\"read_ms\":{d:.3},\"total_ms\":{d:.3},\"checksum\":{d}}}\n", .{
        write_ms,
        read_ms,
        total_ms,
        checksum,
    });
}

fn main() {
    runSqliteBench();
}
