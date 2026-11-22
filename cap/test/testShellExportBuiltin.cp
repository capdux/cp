// cap/test/testShellExportBuiltin.cp
// Production-grade deep tests for cap:shell exportBuiltin.cp (Bun Shell `export` builtin)

import { test, assert, assertEq } from "cap:test";
import {
    SHELL_EXPORT_MAX_VARS,
    ShellExportError,
    isValidShellIdent,
    ShellExportTable,
} from "cap:shell";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("ShellExport: POSIX identifier validation", () => {
    assert(isValidShellIdent("FOO"));
    assert(isValidShellIdent("_private_1"));
    assert(isValidShellIdent("a"));
    assert(isValidShellIdent("NODE_ENV"));

    assert(!isValidShellIdent(""));
    assert(!isValidShellIdent("1ABC"));
    assert(!isValidShellIdent("FOO-BAR"));
    assert(!isValidShellIdent("FOO.BAR"));
    assert(!isValidShellIdent("=VAL"));
    assert(!isValidShellIdent("A B"));
});

test("ShellExport: applyArg handles KEY=VAL, multiple '=', bare promotion and empty args", () => {
    let t = ShellExportTable.init();
    * = t.setVar("EXISTING", "keep_me", false) catch unreachable;
    assert(!t.isExported("EXISTING"));

    // Bare export promotes existing local var without clobbering value
    * = t.applyArg("EXISTING") catch unreachable;
    assert(t.isExported("EXISTING"));
    if (v := t.get("EXISTING")) {
        assert(eqStr(v, "keep_me"));
    } else {
        assert(false);
    }

    // Bare export of absent var creates exported empty value
    * = t.applyArg("NEW_EMPTY") catch unreachable;
    assert(t.isExported("NEW_EMPTY"));
    if (v := t.get("NEW_EMPTY")) {
        assertEq(v.len, 0);
    } else {
        assert(false);
    }

    // Splits only on first '='
    * = t.applyArg("DATABASE_URL=postgres://u:p@h/db?sslmode=require") catch unreachable;
    if (v := t.get("DATABASE_URL")) {
        assert(eqStr(v, "postgres://u:p@h/db?sslmode=require"));
    } else {
        assert(false);
    }

    // Empty arg is a no-op
    * = t.applyArg("") catch unreachable;
    assertEq(t.count(), 3);
    assertEq(t.exportedCount(), 3);
});

test("ShellExport: formatSorted outputs only exported vars in ascending key order", () => {
    let t = ShellExportTable.init();
    * = t.setVar("LOCAL_SECRET", "hidden", false) catch unreachable;
    const args: [4][]const u8 = ["ZEBRA=3", "ALPHA=1", "MIDDLE=2", "ALPHA=1_updated"];
    const applied = t.applyArgs(args[0..]) catch unreachable;
    assertEq(applied, 4);
    assertEq(t.count(), 4);
    assertEq(t.exportedCount(), 3);

    let buf: [128]u8 = [0; 128];
    const n = t.formatSorted(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n], "ALPHA=1_updated\nMIDDLE=2\nZEBRA=3\n"));
});

test("ShellExport: unset removes entries and preserves remaining order", () => {
    let t = ShellExportTable.init();
    * = t.applyArg("A=1") catch unreachable;
    * = t.applyArg("B=2") catch unreachable;
    * = t.applyArg("C=3") catch unreachable;

    assert(t.unset("B"));
    assert(!t.unset("B"));
    assert(t.get("B") == null);
    assertEq(t.count(), 2);

    let buf: [64]u8 = [0; 64];
    const n = t.formatSorted(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n], "A=1\nC=3\n"));
});

test("ShellExport: invalid identifier and tiny buffer guards", () => {
    let t = ShellExportTable.init();

    let bad_bare = false;
    * = t.applyArg("123BAD") catch |e| {
        bad_bare = (e == ShellExportError.InvalidIdentifier);
    };
    assert(bad_bare);

    let bad_assign = false;
    * = t.applyArg("BAD-KEY=val") catch |e| {
        bad_assign = (e == ShellExportError.InvalidIdentifier);
    };
    assert(bad_assign);

    let empty_key = false;
    * = t.applyArg("=orphan") catch |e| {
        empty_key = (e == ShellExportError.InvalidIdentifier);
    };
    assert(empty_key);

    * = t.applyArg("LONG_KEY=LONG_VALUE") catch unreachable;
    let tiny: [8]u8 = [0; 8];
    let buf_err = false;
    * = t.formatSorted(tiny[0..]) catch |e| {
        buf_err = (e == ShellExportError.BufferTooSmall);
    };
    assert(buf_err);
});

test("ShellExport: fixed capacity guard at SHELL_EXPORT_MAX_VARS (64)", () => {
    assertEq(SHELL_EXPORT_MAX_VARS, 64);
    let t = ShellExportTable.init();
    const keys: [64][]const u8 = [
        "K00", "K01", "K02", "K03", "K04", "K05", "K06", "K07",
        "K08", "K09", "K10", "K11", "K12", "K13", "K14", "K15",
        "K16", "K17", "K18", "K19", "K20", "K21", "K22", "K23",
        "K24", "K25", "K26", "K27", "K28", "K29", "K30", "K31",
        "K32", "K33", "K34", "K35", "K36", "K37", "K38", "K39",
        "K40", "K41", "K42", "K43", "K44", "K45", "K46", "K47",
        "K48", "K49", "K50", "K51", "K52", "K53", "K54", "K55",
        "K56", "K57", "K58", "K59", "K60", "K61", "K62", "K63",
    ];
    let i: usize = 0;
    while (i < 64) {
        * = t.setVar(keys[i], "v", true) catch unreachable;
        i += 1;
    }
    assertEq(t.count(), 64);

    // Updating an existing key still succeeds at capacity
    * = t.setVar("K00", "updated", true) catch unreachable;

    // Inserting a 65th distinct key returns TableFull
    let full_err = false;
    * = t.setVar("K64", "overflow", true) catch |e| {
        full_err = (e == ShellExportError.TableFull);
    };
    assert(full_err);
});
