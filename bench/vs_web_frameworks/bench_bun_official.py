#!/usr/bin/env python3
"""
Bun Official-Style Benchmark Suite (Same-Machine Real Measurements):
  1. SQLite Northwind-Style Comprehensive Workload (100K writes + 50K indexed JOIN lookups + 1K JOIN+GROUP BY aggregations)
     - CP (`cap:sqlite` `-OReleaseFast`, statically linked `libsqlite3.a`) vs Bun (`bun:sqlite`) vs Node (`node:sqlite`)
  2. WebSocket RFC 6455 Frame Codec (128B Text Encode/Mask/Decode) + 16-Subscriber Broadcast Fanout (1,000,000 frames)
     - CP (`lib/ws/frame.cp` + `lib/ws/mask.cp` `-OReleaseFast`) vs Bun vs Node

All runtimes execute identical algorithms/schemas/payloads with strict cross-runtime checksum verification.
Peak VmRSS (VmHWM) is measured per-process via Linux `os.wait4` (`ru_maxrss`).
Results are written to `/data/dev/cp/bench/vs_web_frameworks/bun_official_results.json`.
"""

from __future__ import annotations

import datetime
import json
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

BENCH_DIR = Path(__file__).resolve().parent
CP_ROOT = BENCH_DIR.parent.parent
SQLITE_LIB = CP_ROOT / "vendor" / "sqlite" / "prebuilt" / "linux-x86_64" / "libsqlite3.a"
OUT_JSON = BENCH_DIR / "bun_official_results.json"

TRIALS = 3

# ---------------------------------------------------------------------------
# Benchmark 1 Sources: SQLite Northwind-Style (100K Write + 50K JOIN Lookup + 1K JOIN+GROUP BY)
# ---------------------------------------------------------------------------

SQLITE_CP_SRC = r"""
import { Database } from "cap:sqlite";
import { HwTimer } from "cap:bench";
import std from "std";

const CUSTOMERS: i64 = 1000;
const ORDERS: i64 = 100000;
const LOOKUPS: i64 = 50000;
const AGG_QUERIES: i64 = 1000;

fn runBench() {
    let db = Database.openMemory() catch return;
    defer db.close();

    db.exec("CREATE TABLE customers (id INTEGER PRIMARY KEY, region TEXT NOT NULL, tier INTEGER NOT NULL, name TEXT NOT NULL);") catch return;
    db.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL, category INTEGER NOT NULL, amount_cents INTEGER NOT NULL, status INTEGER NOT NULL, note TEXT NOT NULL);") catch return;
    db.exec("CREATE INDEX idx_orders_customer ON orders(customer_id);") catch return;
    db.exec("CREATE INDEX idx_orders_cat_status ON orders(category, status);") catch return;

    const regions: [4][]const u8 = .{ "NA", "EU", "APAC", "LATAM" };
    const cust_name: []const u8 = "customer_northwind";
    const order_note: []const u8 = "northwind_order_item";

    // Phase 1: 1,000 customers + 100,000 orders in a single transaction
    const t0 = HwTimer.nowNs();
    db.begin() catch return;
    {
        let ins_c = db.prepare("INSERT INTO customers (id, region, tier, name) VALUES (?, ?, ?, ?);") catch return;
        defer ins_c.close();
        let c: i64 = 1;
        while (c <= CUSTOMERS) {
            const r_idx: usize = @intCast(@mod(c, 4));
            ins_c.bindInt(1, c) catch return;
            ins_c.bindText(2, regions[r_idx]) catch return;
            ins_c.bindInt(3, @mod(c, 3) + 1) catch return;
            ins_c.bindText(4, cust_name) catch return;
            ins_c.run() catch return;
            c += 1;
        }
    }
    {
        let ins_o = db.prepare("INSERT INTO orders (id, customer_id, category, amount_cents, status, note) VALUES (?, ?, ?, ?, ?, ?);") catch return;
        defer ins_o.close();
        let i: i64 = 1;
        while (i <= ORDERS) {
            ins_o.bindInt(1, i) catch return;
            ins_o.bindInt(2, @mod(i * 37, CUSTOMERS) + 1) catch return;
            ins_o.bindInt(3, @mod(i, 10) + 1) catch return;
            ins_o.bindInt(4, @mod(i * 131, 50000) + 100) catch return;
            ins_o.bindInt(5, @mod(i, 4)) catch return;
            ins_o.bindText(6, order_note) catch return;
            ins_o.run() catch return;
            i += 1;
        }
    }
    db.commit() catch return;
    const t1 = HwTimer.nowNs();

    // Phase 2: 50,000 indexed primary-key + JOIN lookups
    let checksum: i64 = 0;
    {
        let sel = db.prepare("SELECT o.amount_cents, o.category, c.tier, c.region FROM orders o JOIN customers c ON o.customer_id = c.id WHERE o.id = ?;") catch return;
        defer sel.close();
        let k: i64 = 0;
        while (k < LOOKUPS) {
            const target_id = @mod(k * 7919, ORDERS) + 1;
            sel.bindInt(1, target_id) catch return;
            if (sel.step() catch false) {
                const amt = sel.columnInt(0);
                const cat = sel.columnInt(1);
                const tier = sel.columnInt(2);
                const reg = sel.columnText(3);
                checksum += amt + cat + tier + @as(i64, @intCast(reg.len));
            }
            sel.reset() catch return;
            k += 1;
        }
    }
    const t2 = HwTimer.nowNs();

    // Phase 3: 1,000 JOIN + GROUP BY aggregation queries
    {
        let agg = db.prepare("SELECT c.region, COUNT(*), SUM(o.amount_cents) FROM orders o JOIN customers c ON o.customer_id = c.id WHERE o.category = ? AND o.status = ? GROUP BY c.region;") catch return;
        defer agg.close();
        let q: i64 = 0;
        while (q < AGG_QUERIES) {
            const cat = @mod(q, 10) + 1;
            const st = @mod(q, 4);
            agg.bindInt(1, cat) catch return;
            agg.bindInt(2, st) catch return;
            while (agg.step() catch false) {
                const reg = agg.columnText(0);
                const cnt = agg.columnInt(1);
                const total = agg.columnInt(2);
                checksum += cnt + total + @as(i64, @intCast(reg.len));
            }
            agg.reset() catch return;
            q += 1;
        }
    }
    const t3 = HwTimer.nowNs();

    const write_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const lookup_ms: f64 = @as(f64, @floatFromInt(t2 - t1)) / 1_000_000.0;
    const agg_ms: f64 = @as(f64, @floatFromInt(t3 - t2)) / 1_000_000.0;
    const total_ms: f64 = write_ms + lookup_ms + agg_ms;
    const peak_rss_mb: f64 = @as(f64, @floatFromInt(readVmHwmKb())) / 1024.0;

    std.debug.print("{{\"write_ms\":{d:.3},\"lookup_ms\":{d:.3},\"agg_ms\":{d:.3},\"total_ms\":{d:.3},\"peak_rss_mb\":{d:.2},\"checksum\":{d}}}\n", .{
        write_ms,
        lookup_ms,
        agg_ms,
        total_ms,
        peak_rss_mb,
        checksum,
    });
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

fn main() {
    runBench();
}
"""

SQLITE_BUN_SRC = r"""
import { Database } from "bun:sqlite";
import { readFileSync } from "node:fs";

function readPeakRssMb() {
  const m = readFileSync("/proc/self/status", "utf8").match(/VmHWM:\s+(\d+)\s+kB/);
  return m ? Number((Number(m[1]) / 1024).toFixed(2)) : 0;
}

const CUSTOMERS = 1000;
const ORDERS = 100000;
const LOOKUPS = 50000;
const AGG_QUERIES = 1000;

const db = new Database(":memory:");
db.run("CREATE TABLE customers (id INTEGER PRIMARY KEY, region TEXT NOT NULL, tier INTEGER NOT NULL, name TEXT NOT NULL);");
db.run("CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL, category INTEGER NOT NULL, amount_cents INTEGER NOT NULL, status INTEGER NOT NULL, note TEXT NOT NULL);");
db.run("CREATE INDEX idx_orders_customer ON orders(customer_id);");
db.run("CREATE INDEX idx_orders_cat_status ON orders(category, status);");

const regions = ["NA", "EU", "APAC", "LATAM"];
const custName = "customer_northwind";
const orderNote = "northwind_order_item";

const t0 = Bun.nanoseconds();
const insC = db.prepare("INSERT INTO customers (id, region, tier, name) VALUES (?, ?, ?, ?);");
const insO = db.prepare("INSERT INTO orders (id, customer_id, category, amount_cents, status, note) VALUES (?, ?, ?, ?, ?, ?);");
const seedAll = db.transaction(() => {
  for (let c = 1; c <= CUSTOMERS; c++) {
    insC.run(c, regions[c % 4], (c % 3) + 1, custName);
  }
  for (let i = 1; i <= ORDERS; i++) {
    insO.run(i, ((i * 37) % CUSTOMERS) + 1, (i % 10) + 1, ((i * 131) % 50000) + 100, i % 4, orderNote);
  }
});
seedAll();
const t1 = Bun.nanoseconds();

let checksum = 0n;
const sel = db.prepare("SELECT o.amount_cents AS amt, o.category AS cat, c.tier AS tier, c.region AS reg FROM orders o JOIN customers c ON o.customer_id = c.id WHERE o.id = ?;");
for (let k = 0; k < LOOKUPS; k++) {
  const targetId = ((k * 7919) % ORDERS) + 1;
  const row = sel.get(targetId);
  if (row) {
    checksum += BigInt(row.amt + row.cat + row.tier + row.reg.length);
  }
}
const t2 = Bun.nanoseconds();

const agg = db.prepare("SELECT c.region AS reg, COUNT(*) AS cnt, SUM(o.amount_cents) AS total FROM orders o JOIN customers c ON o.customer_id = c.id WHERE o.category = ? AND o.status = ? GROUP BY c.region;");
for (let q = 0; q < AGG_QUERIES; q++) {
  const rows = agg.all((q % 10) + 1, q % 4);
  for (let j = 0; j < rows.length; j++) {
    const r = rows[j];
    checksum += BigInt(r.cnt) + BigInt(r.total) + BigInt(r.reg.length);
  }
}
const t3 = Bun.nanoseconds();
db.close();

const write_ms = (t1 - t0) / 1e6;
const lookup_ms = (t2 - t1) / 1e6;
const agg_ms = (t3 - t2) / 1e6;
const total_ms = write_ms + lookup_ms + agg_ms;

console.log(JSON.stringify({
  write_ms: Number(write_ms.toFixed(3)),
  lookup_ms: Number(lookup_ms.toFixed(3)),
  agg_ms: Number(agg_ms.toFixed(3)),
  total_ms: Number(total_ms.toFixed(3)),
  peak_rss_mb: readPeakRssMb(),
  checksum: Number(checksum),
}));
"""

SQLITE_NODE_SRC = r"""
import { DatabaseSync } from "node:sqlite";
import { readFileSync } from "node:fs";

function readPeakRssMb() {
  const m = readFileSync("/proc/self/status", "utf8").match(/VmHWM:\s+(\d+)\s+kB/);
  return m ? Number((Number(m[1]) / 1024).toFixed(2)) : 0;
}

const CUSTOMERS = 1000;
const ORDERS = 100000;
const LOOKUPS = 50000;
const AGG_QUERIES = 1000;

const db = new DatabaseSync(":memory:");
db.exec("CREATE TABLE customers (id INTEGER PRIMARY KEY, region TEXT NOT NULL, tier INTEGER NOT NULL, name TEXT NOT NULL);");
db.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL, category INTEGER NOT NULL, amount_cents INTEGER NOT NULL, status INTEGER NOT NULL, note TEXT NOT NULL);");
db.exec("CREATE INDEX idx_orders_customer ON orders(customer_id);");
db.exec("CREATE INDEX idx_orders_cat_status ON orders(category, status);");

const regions = ["NA", "EU", "APAC", "LATAM"];
const custName = "customer_northwind";
const orderNote = "northwind_order_item";

const t0 = process.hrtime.bigint();
db.exec("BEGIN;");
const insC = db.prepare("INSERT INTO customers (id, region, tier, name) VALUES (?, ?, ?, ?);");
for (let c = 1; c <= CUSTOMERS; c++) {
  insC.run(c, regions[c % 4], (c % 3) + 1, custName);
}
const insO = db.prepare("INSERT INTO orders (id, customer_id, category, amount_cents, status, note) VALUES (?, ?, ?, ?, ?, ?);");
for (let i = 1; i <= ORDERS; i++) {
  insO.run(i, ((i * 37) % CUSTOMERS) + 1, (i % 10) + 1, ((i * 131) % 50000) + 100, i % 4, orderNote);
}
db.exec("COMMIT;");
const t1 = process.hrtime.bigint();

let checksum = 0n;
const sel = db.prepare("SELECT o.amount_cents AS amt, o.category AS cat, c.tier AS tier, c.region AS reg FROM orders o JOIN customers c ON o.customer_id = c.id WHERE o.id = ?;");
for (let k = 0; k < LOOKUPS; k++) {
  const targetId = ((k * 7919) % ORDERS) + 1;
  const row = sel.get(targetId);
  if (row) {
    checksum += BigInt(row.amt + row.cat + row.tier + row.reg.length);
  }
}
const t2 = process.hrtime.bigint();

const agg = db.prepare("SELECT c.region AS reg, COUNT(*) AS cnt, SUM(o.amount_cents) AS total FROM orders o JOIN customers c ON o.customer_id = c.id WHERE o.category = ? AND o.status = ? GROUP BY c.region;");
for (let q = 0; q < AGG_QUERIES; q++) {
  const rows = agg.all((q % 10) + 1, q % 4);
  for (let j = 0; j < rows.length; j++) {
    const r = rows[j];
    checksum += BigInt(r.cnt) + BigInt(r.total) + BigInt(r.reg.length);
  }
}
const t3 = process.hrtime.bigint();
db.close();

const write_ms = Number(t1 - t0) / 1e6;
const lookup_ms = Number(t2 - t1) / 1e6;
const agg_ms = Number(t3 - t2) / 1e6;
const total_ms = write_ms + lookup_ms + agg_ms;

console.log(JSON.stringify({
  write_ms: Number(write_ms.toFixed(3)),
  lookup_ms: Number(lookup_ms.toFixed(3)),
  agg_ms: Number(agg_ms.toFixed(3)),
  total_ms: Number(total_ms.toFixed(3)),
  peak_rss_mb: readPeakRssMb(),
  checksum: Number(checksum),
}));
"""

# ---------------------------------------------------------------------------
# Benchmark 2 Sources: WebSocket Frame Codec (128B Text Encode/Mask/Decode) + 16-Sub Broadcast
# ---------------------------------------------------------------------------

WS_CP_SRC = r"""
import { FrameHeader, FrameBuilder } from "cap:ws/frame";
import { Mask } from "cap:ws/mask";
import { HwTimer } from "cap:bench";
import std from "std";

const FRAMES: usize = 1000000;
const SUBSCRIBERS: usize = 16;
const PAYLOAD: []const u8 = "{\"type\":\"chat\",\"room\":\"lobby-01\",\"user\":\"agent_42\",\"seq\":1000000,\"msg\":\"hello websocket benchmark payload padding 0123456789ab\"}";

fn runWsBench() {
    let wire_buf: [256]u8 = undefined;
    let bcast_buf: [256]u8 = undefined;
    let sub_bufs: [SUBSCRIBERS][256]u8 = undefined;

    const t0 = HwTimer.nowNs();
    let checksum: u64 = 0;
    let i: usize = 0;
    while (i < FRAMES) {
        const mask_key: [4]u8 = [
            @as(u8, @truncate(i & 0xFF)),
            @as(u8, @truncate((i >> 8) & 0xFF)),
            @as(u8, 0xA5),
            @as(u8, 0x5A),
        ];

        // 1. Client -> Server: Encode + Mask 128B Text frame
        const wire_len = FrameBuilder.buildText(&wire_buf, PAYLOAD, mask_key) catch return;

        // 2. Server: Decode header + Unmask payload
        const hdr = FrameHeader.decode(wire_buf[0..wire_len]) catch return;
        const payload_slice = wire_buf[hdr.header_len .. hdr.header_len + hdr.payload_len];
        if (hdr.masked) {
            Mask.apply(payload_slice, hdr.mask_key);
        }

        // 3. Server -> 16 Subscribers: Encode unmasked broadcast frame + fanout copy
        const bcast_len = FrameBuilder.buildText(&bcast_buf, payload_slice, null) catch return;
        let s: usize = 0;
        while (s < SUBSCRIBERS) {
            @memcpy(sub_bufs[s][0..bcast_len], bcast_buf[0..bcast_len]);
            s += 1;
        }

        const pick_sub = i & 15;
        const pick_byte = i & 127;
        checksum += @as(u64, wire_len) + @as(u64, bcast_len) + @as(u64, payload_slice[pick_byte]) + @as(u64, sub_bufs[pick_sub][4 + pick_byte]);
        i += 1;
    }
    const t1 = HwTimer.nowNs();

    const total_ms: f64 = @as(f64, @floatFromInt(t1 - t0)) / 1_000_000.0;
    const peak_rss_mb: f64 = @as(f64, @floatFromInt(readVmHwmKb())) / 1024.0;
    std.debug.print("{{\"payload_bytes\":{d},\"frames\":{d},\"subscribers\":{d},\"total_ms\":{d:.3},\"peak_rss_mb\":{d:.2},\"checksum\":{d}}}\n", .{
        PAYLOAD.len,
        FRAMES,
        SUBSCRIBERS,
        total_ms,
        peak_rss_mb,
        checksum,
    });
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

fn main() {
    runWsBench();
}
"""

WS_JS_SRC = r"""
import { readFileSync } from "node:fs";

function readPeakRssMb() {
  const m = readFileSync("/proc/self/status", "utf8").match(/VmHWM:\s+(\d+)\s+kB/);
  return m ? Number((Number(m[1]) / 1024).toFixed(2)) : 0;
}

const FRAMES = 1000000;
const SUBSCRIBERS = 16;
const PAYLOAD_STR = '{"type":"chat","room":"lobby-01","user":"agent_42","seq":1000000,"msg":"hello websocket benchmark payload padding 0123456789ab"}';
const PAYLOAD = Buffer.from(PAYLOAD_STR, "utf8");

function applyMask(buf, offset, len, k0, k1, k2, k3) {
  let i = 0;
  const end4 = len & ~3;
  for (; i < end4; i += 4) {
    const p = offset + i;
    buf[p] ^= k0;
    buf[p + 1] ^= k1;
    buf[p + 2] ^= k2;
    buf[p + 3] ^= k3;
  }
  for (; i < len; i++) {
    const m = i & 3;
    const k = m === 0 ? k0 : m === 1 ? k1 : m === 2 ? k2 : k3;
    buf[offset + i] ^= k;
  }
}

function buildTextFrame(out, payload, payloadOff, payloadLen, masked, k0, k1, k2, k3) {
  out[0] = 0x81; // FIN=1, opcode=Text(1)
  let off = 2;
  if (payloadLen <= 125) {
    out[1] = (masked ? 0x80 : 0) | payloadLen;
  } else {
    out[1] = (masked ? 0x80 : 0) | 126;
    out[2] = (payloadLen >>> 8) & 0xff;
    out[3] = payloadLen & 0xff;
    off = 4;
  }
  if (masked) {
    out[off] = k0;
    out[off + 1] = k1;
    out[off + 2] = k2;
    out[off + 3] = k3;
    off += 4;
  }
  payload.copy(out, off, payloadOff, payloadOff + payloadLen);
  if (masked) {
    applyMask(out, off, payloadLen, k0, k1, k2, k3);
  }
  return off + payloadLen;
}

function decodeHeader(buf, len) {
  const b0 = buf[0];
  const b1 = buf[1];
  const masked = (b1 & 0x80) !== 0;
  let plen = b1 & 0x7f;
  let off = 2;
  if (plen === 126) {
    plen = (buf[2] << 8) | buf[3];
    off = 4;
  }
  let k0 = 0, k1 = 0, k2 = 0, k3 = 0;
  if (masked) {
    k0 = buf[off];
    k1 = buf[off + 1];
    k2 = buf[off + 2];
    k3 = buf[off + 3];
    off += 4;
  }
  return { headerLen: off, payloadLen: plen, masked, k0, k1, k2, k3, opcode: b0 & 0x0f };
}

const wireBuf = Buffer.allocUnsafe(256);
const bcastBuf = Buffer.allocUnsafe(256);
const subBufs = Array.from({ length: SUBSCRIBERS }, () => Buffer.allocUnsafe(256));

const nowNs = typeof Bun !== "undefined" ? () => BigInt(Bun.nanoseconds()) : () => process.hrtime.bigint();

const t0 = nowNs();
let checksum = 0;
for (let i = 0; i < FRAMES; i++) {
  const k0 = i & 0xff;
  const k1 = (i >>> 8) & 0xff;
  const k2 = 0xa5;
  const k3 = 0x5a;

  // 1. Client -> Server: Encode + Mask 128B Text frame
  const wireLen = buildTextFrame(wireBuf, PAYLOAD, 0, PAYLOAD.length, true, k0, k1, k2, k3);

  // 2. Server: Decode header + Unmask payload
  const hdr = decodeHeader(wireBuf, wireLen);
  if (hdr.masked) {
    applyMask(wireBuf, hdr.headerLen, hdr.payloadLen, hdr.k0, hdr.k1, hdr.k2, hdr.k3);
  }

  // 3. Server -> 16 Subscribers: Encode unmasked broadcast frame + fanout copy
  const bcastLen = buildTextFrame(bcastBuf, wireBuf, hdr.headerLen, hdr.payloadLen, false, 0, 0, 0, 0);
  for (let s = 0; s < SUBSCRIBERS; s++) {
    bcastBuf.copy(subBufs[s], 0, 0, bcastLen);
  }

  const pickSub = i & 15;
  const pickByte = i & 127;
  checksum += wireLen + bcastLen + wireBuf[hdr.headerLen + pickByte] + subBufs[pickSub][4 + pickByte];
}
const t1 = nowNs();

const total_ms = Number(t1 - t0) / 1e6;
console.log(JSON.stringify({
  payload_bytes: PAYLOAD.length,
  frames: FRAMES,
  subscribers: SUBSCRIBERS,
  total_ms: Number(total_ms.toFixed(3)),
  peak_rss_mb: readPeakRssMb(),
  checksum,
}));
"""


def run_cmd_simple(cmd: list[str], cwd: Path | None = None) -> tuple[int, str, str]:
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def run_with_peak_rss(cmd: list[str], cwd: Path | None = None) -> tuple[int, str, str, float]:
    """Run subprocess and collect exact per-process peak VmRSS (MB) via os.wait4."""
    proc = subprocess.Popen(
        cmd,
        cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert proc.stdout is not None and proc.stderr is not None
    out_bytes = proc.stdout.read()
    err_bytes = proc.stderr.read()
    proc.stdout.close()
    proc.stderr.close()

    _, status, rusage = os.wait4(proc.pid, 0)
    # Prevent Popen.__del__ from warning about unwaited child
    proc.returncode = os.waitstatus_to_exitcode(status)
    # On Linux, ru_maxrss is in KiB
    peak_rss_mb = round(rusage.ru_maxrss / 1024.0, 2)
    return (
        proc.returncode,
        out_bytes.decode("utf-8", errors="replace").strip(),
        err_bytes.decode("utf-8", errors="replace").strip(),
        peak_rss_mb,
    )


def find_bin(name: str, extra_paths: list[str] | None = None) -> str | None:
    found = shutil.which(name)
    if found:
        return found
    for p in extra_paths or []:
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return None


def get_version(bin_path: str | None, args: list[str] = ["--version"]) -> str | None:
    if not bin_path:
        return None
    rc, out, err = run_cmd_simple([bin_path, *args])
    if rc != 0:
        return None
    text = out or err
    return text.splitlines()[0].strip() if text else None


def get_cpu_info() -> dict[str, Any]:
    model = platform.processor() or "unknown"
    cores = os.cpu_count() or 0
    rc, out, _ = run_cmd_simple(["lscpu"])
    if rc == 0:
        for line in out.splitlines():
            if line.startswith("Model name:"):
                model = line.split(":", 1)[1].strip()
                break
    return {
        "model": model,
        "logical_cpus": cores,
        "arch": platform.machine(),
        "os": f"{platform.system()} {platform.release()}",
    }


def compile_cp(
    src_path: Path,
    build_dir: Path,
    link_libs: list[Path] | None = None,
    link_libc: bool = True,
) -> Path:
    stem = src_path.stem
    zig_out = build_dir / f"{stem}.zig"
    bin_out = build_dir / stem

    cpc_cmd = ["cpc", str(src_path), "-I", str(CP_ROOT / "lib"), "-o", str(zig_out)]
    rc, out, err = run_cmd_simple(cpc_cmd)
    if rc != 0:
        raise RuntimeError(f"cpc failed for {src_path.name}:\n{out}\n{err}")

    zig_cmd = ["zig", "build-exe", str(zig_out)]
    for lib in link_libs or []:
        zig_cmd.append(str(lib))
    zig_cmd.append("-OReleaseFast")
    if link_libc:
        zig_cmd.append("-lc")
    zig_cmd.append(f"-femit-bin={bin_out}")

    rc, out, err = run_cmd_simple(zig_cmd, cwd=build_dir)
    if rc != 0:
        raise RuntimeError(f"zig build-exe failed for {src_path.name}:\n{out}\n{err}")

    return bin_out


def run_json_bench(cmd: list[str], trials: int = TRIALS) -> dict[str, Any]:
    samples: list[dict[str, Any]] = []
    rss_samples: list[float] = []
    for _ in range(trials):
        rc, out, err, peak_rss_mb = run_with_peak_rss(cmd)
        if rc != 0:
            raise RuntimeError(f"Command failed: {' '.join(cmd)}\nstdout: {out}\nstderr: {err}")
        raw = out if out.startswith("{") else err
        lines = [ln.strip() for ln in raw.splitlines() if ln.strip().startswith("{")]
        if not lines:
            raise RuntimeError(f"No JSON output from {' '.join(cmd)}\nstdout: {out}\nstderr: {err}")
        samples.append(json.loads(lines[-1]))
        rss_samples.append(peak_rss_mb)

    result: dict[str, Any] = {}
    for k in samples[0].keys():
        vals = [s[k] for s in samples]
        if k.endswith("_ms") or k == "peak_rss_mb":
            result[k] = round(statistics.median(vals), 3 if k.endswith("_ms") else 2)
        else:
            result[k] = vals[0]
    if "peak_rss_mb" not in result or result["peak_rss_mb"] <= 0:
        result["peak_rss_mb"] = round(statistics.median(rss_samples), 2)
    return result


def speedup(other_ms: float | None, cp_ms: float | None) -> float | None:
    if other_ms is None or cp_ms is None or cp_ms <= 0:
        return None
    return round(other_ms / cp_ms, 2)


def main() -> int:
    bun_bin = find_bin("bun", ["/root/.bun/bin/bun", "/data/dev/bun/build/release/bun"])
    node_bin = find_bin("node")
    if not SQLITE_LIB.is_file():
        raise FileNotFoundError(f"Missing prebuilt SQLite static library: {SQLITE_LIB}")

    cpu_info = get_cpu_info()
    runtimes = {
        "cp": {
            "cpc_version": get_version(find_bin("cpc")),
            "zig_version": get_version(find_bin("zig"), ["version"]),
            "opt_mode": "ReleaseFast",
        },
        "bun": {"path": bun_bin, "version": get_version(bun_bin)},
        "node": {"path": node_bin, "version": get_version(node_bin)},
    }

    build_dir = Path(tempfile.mkdtemp(prefix="cp_bench_bun_official_"))
    try:
        cp_sql_src = build_dir / "bench_sqlite_nw.cp"
        bun_sql_src = build_dir / "bench_sqlite_nw_bun.js"
        node_sql_src = build_dir / "bench_sqlite_nw_node.mjs"
        cp_ws_src = build_dir / "bench_ws_codec.cp"
        js_ws_src = build_dir / "bench_ws_codec.mjs"

        cp_sql_src.write_text(SQLITE_CP_SRC, encoding="utf-8")
        bun_sql_src.write_text(SQLITE_BUN_SRC, encoding="utf-8")
        node_sql_src.write_text(SQLITE_NODE_SRC, encoding="utf-8")
        cp_ws_src.write_text(WS_CP_SRC, encoding="utf-8")
        js_ws_src.write_text(WS_JS_SRC, encoding="utf-8")

        print("==> Compiling CP benchmarks (-OReleaseFast)...")
        cp_sql_bin = compile_cp(cp_sql_src, build_dir, link_libs=[SQLITE_LIB], link_libc=True)
        cp_ws_bin = compile_cp(cp_ws_src, build_dir, link_libs=[], link_libc=True)

        # ---- Bench 1: SQLite Northwind ----
        print("==> Running Bench 1: SQLite Northwind (100K writes + 50K JOIN lookups + 1K JOIN+GROUP BY)...")
        sql_res: dict[str, Any] = {}
        sql_res["cp"] = run_json_bench([str(cp_sql_bin)])
        if bun_bin:
            sql_res["bun"] = run_json_bench([bun_bin, str(bun_sql_src)])
        if node_bin:
            sql_res["node"] = run_json_bench([node_bin, str(node_sql_src)])

        sql_ck = sql_res["cp"]["checksum"]
        total_sql_ops = 100_000 + 50_000 + 1_000
        for k, v in sql_res.items():
            if v["checksum"] != sql_ck:
                raise AssertionError(f"SQLite checksum mismatch for {k}: {v['checksum']} != {sql_ck}")
            v["ops_per_sec"] = round(total_sql_ops / (v["total_ms"] / 1000.0), 1)
            v["write_ops_per_sec"] = round(100_000 / (v["write_ms"] / 1000.0), 1)
            v["lookup_ops_per_sec"] = round(50_000 / (v["lookup_ms"] / 1000.0), 1)
            v["agg_qps"] = round(1_000 / (v["agg_ms"] / 1000.0), 1)

        sql_data = {
            "description": "SQLite :memory: Northwind-style 100K order writes + 50K indexed JOIN lookups + 1K JOIN+GROUP BY aggregations",
            "trials": TRIALS,
            "workload": {
                "customers": 1000,
                "orders_written": 100000,
                "indexed_join_lookups": 50000,
                "join_group_by_queries": 1000,
                "total_ops": total_sql_ops,
            },
            "results": sql_res,
            "cp_speedup_vs": {
                k: speedup(v["total_ms"], sql_res["cp"]["total_ms"])
                for k, v in sql_res.items() if k != "cp"
            },
        }

        # ---- Bench 2: WebSocket Frame Codec + Broadcast ----
        print("==> Running Bench 2: WebSocket 128B Frame Codec (1M encode/mask/decode + 16-sub broadcast)...")
        ws_res: dict[str, Any] = {}
        ws_res["cp"] = run_json_bench([str(cp_ws_bin)])
        if bun_bin:
            ws_res["bun"] = run_json_bench([bun_bin, str(js_ws_src)])
        if node_bin:
            ws_res["node"] = run_json_bench([node_bin, str(js_ws_src)])

        ws_ck = ws_res["cp"]["checksum"]
        payload_bytes = ws_res["cp"]["payload_bytes"]
        frames = ws_res["cp"]["frames"]
        subs = ws_res["cp"]["subscribers"]
        for k, v in ws_res.items():
            if v["checksum"] != ws_ck:
                raise AssertionError(f"WebSocket checksum mismatch for {k}: {v['checksum']} != {ws_ck}")
            sec = v["total_ms"] / 1000.0
            v["msg_per_sec"] = round(frames / sec, 1)
            v["broadcast_msg_per_sec"] = round((frames * subs) / sec, 1)
            v["payload_mb_per_sec"] = round((frames * payload_bytes) / (1024 * 1024) / sec, 2)

        ws_data = {
            "description": "RFC 6455 WebSocket 128B text frame encode/mask/decode + 16-subscriber broadcast fanout (1,000,000 frames)",
            "trials": TRIALS,
            "workload": {
                "payload_bytes": payload_bytes,
                "frames": frames,
                "subscribers": subs,
                "broadcast_deliveries": frames * subs,
            },
            "results": ws_res,
            "cp_speedup_vs": {
                k: speedup(v["total_ms"], ws_res["cp"]["total_ms"])
                for k, v in ws_res.items() if k != "cp"
            },
        }

        final_data = {
            "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "host": {"cpu": cpu_info},
            "runtimes": runtimes,
            "benchmarks": {
                "sqlite_northwind": sql_data,
                "websocket_codec_broadcast": ws_data,
            },
        }

        OUT_JSON.write_text(json.dumps(final_data, indent=2) + "\n", encoding="utf-8")
        print(f"==> Wrote {OUT_JSON}")
        print(json.dumps(final_data["benchmarks"], indent=2))
        return 0
    finally:
        shutil.rmtree(build_dir, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
