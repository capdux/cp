// Example 02: Built-in cap:sqlite (Embedded SQLite 3.54.1, zero install)
// Run with: cap examples/02_sqlite_memory.cp

import { Database } from "cap:sqlite";
import std from "std";

fn main() {
    let db = Database.openMemory() catch {
        std.debug.print("Failed to open in-memory SQLite\n", .{});
        return;
    };

    db.exec("CREATE TABLE agents (id INTEGER PRIMARY KEY, name TEXT, score INTEGER);") catch return;
    db.exec("INSERT INTO agents (name, score) VALUES ('alpha', 98), ('beta', 95), ('gamma', 100);") catch return;

    let stmt = db.prepare("SELECT name, score FROM agents WHERE score >= ? ORDER BY score DESC;") catch return;
    stmt.bindInt(1, 96) catch return;

    std.debug.print("Top Agents (SQLite :memory:):\n", .{});
    while (stmt.step() catch false) {
        std.debug.print("  - {s}: {d}\n", .{ stmt.columnText(0), stmt.columnInt(1) });
    }
}
