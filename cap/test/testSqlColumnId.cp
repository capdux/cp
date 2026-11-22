// cap/test/testSqlColumnId.cp
// Production-grade deep tests for cap:sql columnId.cp (Bun ColumnIdentifier & duplicate-column tracking)

import { test, assert, assertEq } from "cap:test";
import {
    SQL_COLUMN_ID_MAX_COLS,
    SQL_COLUMN_ID_MAX_INDEX,
    SqlColumnIdError,
    SqlColumnIdKind,
    SqlColumnId,
    SqlColumnIdMap,
} from "cap:sql";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SqlColumnId: numeric index classification and u32::MAX boundary", () => {
    assertEq(SQL_COLUMN_ID_MAX_COLS, 64);
    assertEq(SQL_COLUMN_ID_MAX_INDEX, 4294967294);

    const zero = SqlColumnId.parse("0");
    assert(zero.isIndex());
    assertEq(zero.index_val, 0);

    const small = SqlColumnId.parse("42");
    assert(small.isIndex());
    assertEq(small.index_val, 42);

    // Exact upper bound: u32::MAX - 1 = 4294967294 -> .index
    const max_ok = SqlColumnId.parse("4294967294");
    assert(max_ok.isIndex());
    assertEq(max_ok.index_val, 4294967294);

    // u32::MAX = 4294967295 -> strictly NOT < maxInt(u32), must remain .name
    const at_u32_max = SqlColumnId.parse("4294967295");
    assert(at_u32_max.isName());
    assert(eqStr(at_u32_max.name_slice, "4294967295"));

    // >10 digits stays .name
    const too_long = SqlColumnId.parse("10000000000");
    assert(too_long.isName());
});

test("SqlColumnId: empty, alphanumeric, and signed strings remain .name", () => {
    const empty = SqlColumnId.parse("");
    assert(empty.isName());
    assertEq(empty.name_slice.len, 0);

    const normal = SqlColumnId.parse("user_id");
    assert(normal.isName());
    assert(eqStr(normal.name_slice, "user_id"));

    const neg = SqlColumnId.parse("-1");
    assert(neg.isName());

    const mixed = SqlColumnId.parse("123abc");
    assert(mixed.isName());

    const plus = SqlColumnId.parse("+5");
    assert(plus.isName());
});

test("SqlColumnIdMap: duplicate column shadowing (last-column-wins object semantics)", () => {
    let m = SqlColumnIdMap.init();
    const p0 = m.addColumn("id") catch unreachable;
    const p1 = m.addColumn("email") catch unreachable;
    const p2 = m.addColumn("id") catch unreachable;
    const p3 = m.addColumn("email") catch unreachable;
    const p4 = m.addColumn("id") catch unreachable;

    assertEq(p0, 0);
    assertEq(p1, 1);
    assertEq(p2, 2);
    assertEq(p3, 3);
    assertEq(p4, 4);

    assertEq(m.count(), 5);
    assertEq(m.duplicateCount(), 3);
    assertEq(m.activeCount(), 2);
    assert(m.hasDuplicates());

    // Earlier slots marked `.duplicate`, latest slots remain active
    if (c0 := m.get(0)) { assert(c0.isDuplicate()); } else { assert(false); }
    if (c1 := m.get(1)) { assert(c1.isDuplicate()); } else { assert(false); }
    if (c2 := m.get(2)) { assert(c2.isDuplicate()); } else { assert(false); }
    if (c3 := m.get(3)) { assert(c3.isName()); } else { assert(false); }
    if (c4 := m.get(4)) { assert(c4.isName()); } else { assert(false); }

    // lookupByName returns the last-column-wins position
    if (id_pos := m.lookupByName("id")) {
        assertEq(id_pos, 4);
    } else {
        assert(false);
    }
    if (email_pos := m.lookupByName("email")) {
        assertEq(email_pos, 3);
    } else {
        assert(false);
    }
    assert(m.lookupByName("missing") == null);

    // Positional raw names remain intact for `.values()` mode
    if (raw0 := m.rawNameAt(0)) { assert(eqStr(raw0, "id")); } else { assert(false); }
    assert(m.rawNameAt(5) == null);
});

test("SqlColumnIdMap: numeric index columns and leading-zero equivalence", () => {
    let m = SqlColumnIdMap.init();
    * = m.addColumn("0") catch unreachable;
    * = m.addColumn("title") catch unreachable;
    * = m.addColumn("00") catch unreachable; // Numeric value 0 shadows "0"

    assertEq(m.count(), 3);
    assertEq(m.duplicateCount(), 1);
    assert(m.hasDuplicates());

    if (idx_pos := m.lookupByIndex(0)) {
        assertEq(idx_pos, 2);
    } else {
        assert(false);
    }
    assert(m.lookupByIndex(1) == null);
});

test("SqlColumnIdMap: header list formatting and tiny buffer guard", () => {
    let m = SqlColumnIdMap.init();
    * = m.addColumn("id") catch unreachable;
    * = m.addColumn("name") catch unreachable;
    * = m.addColumn("created_at") catch unreachable;

    let buf: [64]u8 = [0; 64];
    const n = m.formatHeaderList(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n], "id,name,created_at"));

    let tiny: [5]u8 = [0; 5];
    let buf_err = false;
    * = m.formatHeaderList(tiny[0..]) catch |e| {
        buf_err = (e == SqlColumnIdError.BufferTooSmall);
    };
    assert(buf_err);
});

test("SqlColumnIdMap: capacity guard at 64 columns and equality helpers", () => {
    let m = SqlColumnIdMap.init();
    let i: usize = 0;
    while (i < SQL_COLUMN_ID_MAX_COLS) {
        * = m.addColumn("col") catch unreachable;
        i += 1;
    }
    assertEq(m.count(), 64);
    assertEq(m.duplicateCount(), 63);
    assertEq(m.activeCount(), 1);

    let full_err = false;
    * = m.addColumn("overflow") catch |e| {
        full_err = (e == SqlColumnIdError.TooManyColumns);
    };
    assert(full_err);

    const a = SqlColumnId.fromIndex(7);
    const b = SqlColumnId.parse("7");
    const dup = SqlColumnId.duplicateOf("7");
    assert(a.eql(b));
    assert(a.sameKey(b));
    assert(!a.sameKey(dup));
    assert(dup.kind == SqlColumnIdKind.duplicate);
});
