// SQLite Extreme 1c / 8c / 16c Multi-Thread WAL Benchmark (`cap:sqlite`)
// Statically linked against source-compiled `temp/sqlite3_extreme.o` with:
//   -O3 -march=native -DSQLITE_THREADSAFE=2 -DSQLITE_DEFAULT_MEMSTATUS=0
//   -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_DQS=0 -DSQLITE_MAX_EXPR_DEPTH=0 -DSQLITE_OMIT_DEPRECATED
//
// Modes via env `SQLITE_BENCH_MODE`:
//   - "write"             : 1c 100,000 order inserts in 1 transaction with prepared statement reuse (EXCLUSIVE)
//   - "point_read"        : 1c 200,000 indexed point lookups (`SELECT amount FROM orders WHERE id = ?`) (EXCLUSIVE)
//   - "wal_read_8t"       : 8c 8 pthread workers x 200,000 = 1,600,000 concurrent WAL point reads (NORMAL)
//   - "wal_read_16t"      : 16c 16 pthread workers x 200,000 = 3,200,000 concurrent WAL point reads (NORMAL)
//   - "batch_materialize" : 1c 500 queries x 1,000 rows = 500,000 multi-column rows into `undefined` stack Arena

import {
    Database,
    SQLITE_OPEN_READONLY,
    SQLITE_OPEN_READWRITE,
    SQLITE_OPEN_CREATE,
    SQLITE_OPEN_NOMUTEX,
} from "cap:sqlite";
import { HwTimer } from "cap:bench";
import std from "std";

extern fn getenv(name: [*:0]const u8): [*:0]const u8?;
extern fn atoi(s: [*:0]const u8): i32;
extern fn sched_yield(): i32;
extern fn pthread_create(
    thread: *usize,
    attr: *const u8?,
    start_routine: *const fn (*u8?) callconv(.c) *u8?,
    arg: *u8?,
): i32;
extern fn pthread_join(thread: usize, retval: *u8?): i32;

const ORDERS: i64 = 100000;
const POINT_QUERIES: i64 = 200000;
const BATCH_QUERIES: i64 = 500;
const BATCH_ROWS_PER_QUERY: usize = 1000;
const BATCH_CATS: i64 = 100;
const ARENA_CAP: usize = 131072;
const MAX_THREADS: usize = 32;
const ORDER_NOTE: []const u8 = "order_payload_item";

const PRAGMA_WRITE_SQL: []const u8 =
    "PRAGMA journal_mode=WAL; PRAGMA locking_mode=EXCLUSIVE; PRAGMA synchronous=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY;";

const PRAGMA_POINT_READ_SQL: []const u8 =
    "PRAGMA busy_timeout=5000; PRAGMA locking_mode=EXCLUSIVE; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;";

const PRAGMA_READ_SQL: []const u8 =
    "PRAGMA busy_timeout=5000; PRAGMA locking_mode=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;";

const PRAGMA_LOCKING_NORMAL_SQL: []const u8 = "PRAGMA locking_mode=NORMAL;";

const SELECT_SQL: []const u8 = "SELECT amount FROM orders WHERE id = ?;";
const SELECT_BATCH_SQL: []const u8 =
    "SELECT id, amount, status, customer, sku FROM orders_batch WHERE cat_id = ?;";

fn envStr(name: [*:0]const u8, default_val: []const u8): []const u8 {
    if (v := getenv(name)) {
        return std.mem.span(v);
    }
    return default_val;
}

fn envInt(name: [*:0]const u8, default_val: i32): i32 {
    if (v := getenv(name)) {
        const n = atoi(v);
        if (n > 0) return n;
    }
    return default_val;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn readVmHwmKb(): u64 {
    let buf: [4096]u8 = undefined;
    let kb: u64 = 0;
    zig {
        const fp = std.c.fopen("/proc/self/status", "r") orelse return 0;
        defer _ = std.c.fclose(fp);
        const n = std.c.fread(&buf, 1, buf.len, fp);
        const text = buf[0..n];
        if (std.mem.indexOf(u8, text, "VmHWM:")) |pos| {
            var p = pos + 6;
            while (p < text.len and (text[p] == ' ' or text[p] == '\t')) : (p += 1) {}
            while (p < text.len and text[p] >= '0' and text[p] <= '9') : (p += 1) {
                kb = kb * 10 + @as(u64, text[p] - '0');
            }
        }
    }
    return kb;
}

fn runWrite(db_path: []const u8) {
    let db = Database.open(
        db_path,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
    ) catch {
        std.debug.print("{{\"error\":\"open_write_failed\"}}\n", .{});
        return;
    };
    defer db.close();

    db.exec(PRAGMA_WRITE_SQL) catch {
        std.debug.print("{{\"error\":\"pragma_write_failed\"}}\n", .{});
        return;
    };

    db.exec("DROP TABLE IF EXISTS orders;") catch return;
    db.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL, amount INTEGER NOT NULL, status INTEGER NOT NULL, note TEXT NOT NULL);") catch return;

    const t0 = HwTimer.nowNs();
    db.begin() catch return;
    {
        let ins = db.prepare("INSERT INTO orders (id, customer_id, amount, status, note) VALUES (?, ?, ?, ?, ?);") catch return;
        defer ins.close();
        let i: i64 = 1;
        while (i <= ORDERS) : (i += 1) {
            ins.bindInt(1, i) catch return;
            ins.bindInt(2, @mod(i * 37, 10000) + 1) catch return;
            ins.bindInt(3, @mod(i * 131, 50000) + 100) catch return;
            ins.bindInt(4, @mod(i, 4)) catch return;
            ins.bindText(5, ORDER_NOTE) catch return;
            ins.run() catch return;
        }
    }
    db.commit() catch return;
    const t1 = HwTimer.nowNs();

    db.exec(PRAGMA_LOCKING_NORMAL_SQL) catch {};

    let checksum: i64 = 0;
    {
        let chk = db.prepare("SELECT COUNT(*), SUM(amount) FROM orders;") catch return;
        defer chk.close();
        if (chk.next()) {
            checksum = chk.columnInt(0) + chk.columnInt(1);
        }
    }

    const elapsed_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const ops_per_sec: f64 = @as(f64, @floatFromInt(ORDERS)) / (elapsed_ms / 1000.0);
    const peak_rss_mb: f64 = @as(f64, @floatFromInt(readVmHwmKb())) / 1024.0;

    std.debug.print(
        "{{\"mode\":\"write\",\"rows\":{d},\"elapsed_ms\":{d:.3},\"ops_per_sec\":{d:.1},\"peak_rss_mb\":{d:.2},\"checksum\":{d}}}\n",
        .{ ORDERS, elapsed_ms, ops_per_sec, peak_rss_mb, checksum },
    );
}

fn runPointRead(db_path: []const u8) {
    let db = Database.open(db_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX) catch {
        std.debug.print("{{\"error\":\"open_read_failed\"}}\n", .{});
        return;
    };
    defer db.close();

    db.exec(PRAGMA_POINT_READ_SQL) catch {
        std.debug.print("{{\"error\":\"pragma_read_failed\"}}\n", .{});
        return;
    };

    let sel = db.prepare(SELECT_SQL) catch {
        std.debug.print("{{\"error\":\"prepare_read_failed\"}}\n", .{});
        return;
    };
    defer sel.close();

    let checksum: i64 = 0;
    const t0 = HwTimer.nowNs();
    let k: i64 = 0;
    while (k < POINT_QUERIES) : (k += 1) {
        const target_id = @mod(k * 7919, ORDERS) + 1;
        sel.bindInt(1, target_id) catch return;
        if (sel.next()) {
            checksum += sel.columnInt(0);
        }
        sel.reset() catch return;
    }
    const t1 = HwTimer.nowNs();

    db.exec(PRAGMA_LOCKING_NORMAL_SQL) catch {};

    const elapsed_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const ops_per_sec: f64 = @as(f64, @floatFromInt(POINT_QUERIES)) / (elapsed_ms / 1000.0);
    const peak_rss_mb: f64 = @as(f64, @floatFromInt(readVmHwmKb())) / 1024.0;

    std.debug.print(
        "{{\"mode\":\"point_read\",\"queries\":{d},\"elapsed_ms\":{d:.3},\"ops_per_sec\":{d:.1},\"peak_rss_mb\":{d:.2},\"checksum\":{d}}}\n",
        .{ POINT_QUERIES, elapsed_ms, ops_per_sec, peak_rss_mb, checksum },
    );
}

struct WalWorkerCtx {
    db_path: []const u8,
    tid: i64,
    queries: i64,
    ready_count: *i32,
    go_flag: *i32,
    checksum: i64,
    err_code: i32,
}

@c fn walReaderWorker(arg: *u8?): *u8? {
    if (arg == null) return null;
    unsafe {
        const ctx: *WalWorkerCtx = @ptrCast(@alignCast(arg.?));
        const db_path = ctx.db_path;

        let db = Database.open(db_path, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX) catch {
            ctx.err_code = 1;
            * = @atomicRmw(i32, ctx.ready_count, .Add, 1, .seq_cst);
            return null;
        };
        defer db.close();

        db.exec(PRAGMA_READ_SQL) catch {
            ctx.err_code = 2;
            * = @atomicRmw(i32, ctx.ready_count, .Add, 1, .seq_cst);
            return null;
        };

        let sel = db.prepare(SELECT_SQL) catch {
            ctx.err_code = 3;
            * = @atomicRmw(i32, ctx.ready_count, .Add, 1, .seq_cst);
            return null;
        };
        defer sel.close();

        * = @atomicRmw(i32, ctx.ready_count, .Add, 1, .seq_cst);
        while (@atomicLoad(i32, ctx.go_flag, .acquire) == 0) {
            * = sched_yield();
        }

        let local_sum: i64 = 0;
        const offset: i64 = ctx.tid * 131071;
        let k: i64 = 0;
        while (k < ctx.queries) : (k += 1) {
            const target_id = @mod((k + offset) * 7919, ORDERS) + 1;
            sel.bindInt(1, target_id) catch {
                ctx.err_code = 4;
                break;
            };
            if (sel.next()) {
                local_sum += sel.columnInt(0);
            }
            sel.reset() catch {
                ctx.err_code = 5;
                break;
            };
        }
        ctx.checksum = local_sum;
    }
    return null;
}

fn runWalReadMulti(db_path: []const u8, num_threads: usize, mode_name: []const u8) {
    let n: usize = num_threads;
    if (n < (1 as usize)) {
        n = 1 as usize;
    }
    if (n > MAX_THREADS) {
        n = MAX_THREADS;
    }

    let threads: [MAX_THREADS]usize = [0; MAX_THREADS];
    let ctxs: [MAX_THREADS]WalWorkerCtx = undefined;
    let ready_count: i32 = 0;
    let go_flag: i32 = 0;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        ctxs[i] = WalWorkerCtx{
            db_path: db_path,
            tid: @intCast(i),
            queries: POINT_QUERIES,
            ready_count: *ready_count,
            go_flag: *go_flag,
            checksum: 0,
            err_code: 0,
        };
        unsafe {
            const rc = pthread_create(
                *threads[i],
                null,
                walReaderWorker,
                @ptrCast(*ctxs[i]),
            );
            if (rc != 0) {
                std.debug.print("{{\"error\":\"pthread_create_failed\"}}\n", .{});
                return;
            }
        }
    }

    while (@atomicLoad(i32, *ready_count, .acquire) < @as(i32, @intCast(n))) {
        * = sched_yield();
    }

    const t0 = HwTimer.nowNs();
    @atomicStore(i32, *go_flag, 1, .release);

    i = 0;
    while (i < n) : (i += 1) {
        * = pthread_join(threads[i], null);
    }
    const t1 = HwTimer.nowNs();

    let total_checksum: i64 = 0;
    i = 0;
    while (i < n) : (i += 1) {
        if (ctxs[i].err_code != 0) {
            std.debug.print("{{\"error\":\"worker_failed\",\"tid\":{d},\"code\":{d}}}\n", .{ i, ctxs[i].err_code });
            return;
        }
        total_checksum += ctxs[i].checksum;
    }

    const total_queries: i64 = @as(i64, @intCast(n)) * POINT_QUERIES;
    const elapsed_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const ops_per_sec: f64 = @as(f64, @floatFromInt(total_queries)) / (elapsed_ms / 1000.0);
    const peak_rss_mb: f64 = @as(f64, @floatFromInt(readVmHwmKb())) / 1024.0;

    std.debug.print(
        "{{\"mode\":\"{s}\",\"threads\":{d},\"queries\":{d},\"elapsed_ms\":{d:.3},\"ops_per_sec\":{d:.1},\"peak_rss_mb\":{d:.2},\"checksum\":{d}}}\n",
        .{ mode_name, n, total_queries, elapsed_ms, ops_per_sec, peak_rss_mb, total_checksum },
    );
}

struct OrderRow {
    id: i64,
    amount: i64,
    status: i64,
    customer_off: u32,
    customer_len: u32,
    sku_off: u32,
    sku_len: u32,
}

fn runBatchMaterialize(db_path: []const u8) {
    let db = Database.open(db_path, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX) catch {
        std.debug.print("{{\"error\":\"open_batch_failed\"}}\n", .{});
        return;
    };
    defer db.close();

    db.exec(PRAGMA_READ_SQL) catch {
        std.debug.print("{{\"error\":\"pragma_batch_failed\"}}\n", .{});
        return;
    };

    let sel = db.prepare(SELECT_BATCH_SQL) catch {
        std.debug.print("{{\"error\":\"prepare_batch_failed\"}}\n", .{});
        return;
    };
    defer sel.close();

    // Zero-heap, zero-GC pre-allocated stack buffers (`undefined` avoids zero-fill overhead).
    let rows: [BATCH_ROWS_PER_QUERY]OrderRow = undefined;
    let arena: [ARENA_CAP]u8 = undefined;

    let checksum: i64 = 0;
    let total_rows: i64 = 0;

    const t0 = HwTimer.nowNs();
    let q: i64 = 0;
    while (q < BATCH_QUERIES) : (q += 1) {
        const cat = @mod(q * 17, BATCH_CATS);
        sel.bindInt(1, cat) catch return;

        let row_count: usize = 0;
        let arena_pos: usize = 0;

        while (sel.next()) {
            const id = sel.columnInt(0);
            const amount = sel.columnInt(1);
            const status = sel.columnInt(2);
            const cust = sel.columnText(3);
            const sku = sel.columnText(4);

            const c_off = arena_pos;
            if (cust.len > 0) {
                @memcpy(arena[arena_pos..arena_pos + cust.len], cust);
                arena_pos += cust.len;
            }
            const s_off = arena_pos;
            if (sku.len > 0) {
                @memcpy(arena[arena_pos..arena_pos + sku.len], sku);
                arena_pos += sku.len;
            }

            if (row_count < BATCH_ROWS_PER_QUERY) {
                rows[row_count] = OrderRow{
                    id: id,
                    amount: amount,
                    status: status,
                    customer_off: @intCast(c_off),
                    customer_len: @intCast(cust.len),
                    sku_off: @intCast(s_off),
                    sku_len: @intCast(sku.len),
                };
                row_count += 1;
            }
        }
        sel.reset() catch return;

        total_rows += @as(i64, @intCast(row_count));
        let i: usize = 0;
        while (i < row_count) : (i += 1) {
            const r = rows[i];
            const c_start: usize = @intCast(r.customer_off);
            const c_len: usize = @intCast(r.customer_len);
            const s_start: usize = @intCast(r.sku_off);
            const s_len: usize = @intCast(r.sku_len);
            const c_b0: i64 = if (c_len > 0) @as(i64, arena[c_start]) else 0;
            const c_blast: i64 = if (c_len > 0) @as(i64, arena[c_start + c_len - 1]) else 0;
            const s_b0: i64 = if (s_len > 0) @as(i64, arena[s_start]) else 0;
            const s_blast: i64 = if (s_len > 0) @as(i64, arena[s_start + s_len - 1]) else 0;
            checksum += r.id + r.amount + r.status + @as(i64, @intCast(c_len)) + @as(i64, @intCast(s_len)) + c_b0 + c_blast + s_b0 + s_blast;
        }
        // O(1) linear arena reset (zero malloc, zero GC)
        arena_pos = 0;
    }
    const t1 = HwTimer.nowNs();

    const elapsed_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const rows_per_sec: f64 = @as(f64, @floatFromInt(total_rows)) / (elapsed_ms / 1000.0);
    const peak_rss_mb: f64 = @as(f64, @floatFromInt(readVmHwmKb())) / 1024.0;

    std.debug.print(
        "{{\"mode\":\"batch_materialize\",\"queries\":{d},\"rows\":{d},\"elapsed_ms\":{d:.3},\"rows_per_sec\":{d:.1},\"ops_per_sec\":{d:.1},\"peak_rss_mb\":{d:.2},\"checksum\":{d}}}\n",
        .{ BATCH_QUERIES, total_rows, elapsed_ms, rows_per_sec, rows_per_sec, peak_rss_mb, checksum },
    );
}

fn main() {
    const mode = envStr("SQLITE_BENCH_MODE", "write");
    const db_path = envStr("SQLITE_BENCH_DB", "/tmp/cp_sqlite_extreme.db");
    const default_threads: i32 = if (strEq(mode, "wal_read_16t")) 16 else 8;
    const threads: usize = @intCast(envInt("SQLITE_BENCH_THREADS", default_threads));

    if (strEq(mode, "write")) {
        runWrite(db_path);
    } else if (strEq(mode, "point_read")) {
        runPointRead(db_path);
    } else if (strEq(mode, "wal_read_8t") || strEq(mode, "wal_read_16t") || strEq(mode, "wal_read")) {
        runWalReadMulti(db_path, threads, mode);
    } else if (strEq(mode, "batch_materialize")) {
        runBatchMaterialize(db_path);
    } else {
        std.debug.print("{{\"error\":\"unknown_mode\"}}\n", .{});
    }
}
