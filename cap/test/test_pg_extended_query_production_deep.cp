import std from "std";
import { test, expect } from "cap:test";
import {
    TargetKind,
    ParamFormat,
    BindParam,
    ExtendedQueryBuilder,
} from "cap:pg";

fn readU32Be(b: []const u8, off: usize): u32 {
    return (@as(u32, b[off]) << 24)
        | (@as(u32, b[off + 1]) << 16)
        | (@as(u32, b[off + 2]) << 8)
        | @as(u32, b[off + 3]);
}

test("pg_extended_query: pipelined Parse + Bind + Describe + Execute + Sync", () => {
    let b = ExtendedQueryBuilder.init(std.testing.allocator);

    const oids: [1]u32 = [23]; // INT4 OID
    b.writeParse("stmt1", "SELECT $1::int", oids[0..]) catch unreachable;

    const pfmts: [1]ParamFormat = [.text];
    const params: [2]BindParam = [
        BindParam{ value: "42" },
        BindParam{ value: null },
    ];
    const rfmts: [1]ParamFormat = [.binary];
    b.writeBind("p1", "stmt1", pfmts[0..], params[0..], rfmts[0..]) catch unreachable;
    b.writeDescribe(.portal, "p1") catch unreachable;
    b.writeExecute("p1", 100) catch unreachable;
    b.writeClose(.statement, "stmt1") catch unreachable;
    b.writeSync() catch unreachable;

    const wire = b.bytes();
    // Walk all 6 framed messages and verify exact tags & lengths
    let pos: usize = 0;
    const expected_tags: [6]u8 = ['P', 'B', 'D', 'E', 'C', 'S'];
    let idx: usize = 0;
    while (idx < 6) : (idx += 1) {
        expect(wire[pos]).toBe(expected_tags[idx]);
        const mlen = readU32Be(wire, pos + 1);
        expect(mlen >= 4).toBeTrue();
        pos += 1 + @as(usize, mlen);
    }
    expect(pos).toBe(wire.len);
});

test("pg_extended_query: SimpleQuery, Flush, Terminate, and reset", () => {
    let b = ExtendedQueryBuilder.init(std.testing.allocator);
    b.writeSimpleQuery("SELECT 1") catch unreachable;
    b.writeFlush() catch unreachable;
    b.writeTerminate() catch unreachable;

    const wire = b.bytes();
    expect(wire[0]).toBe('Q');
    const qlen = readU32Be(wire, 1);
    expect(qlen).toBe(4 + 9); // 4 + "SELECT 1\0"
    expect(wire[1 + qlen]).toBe('H');
    expect(wire[1 + qlen + 5]).toBe('X');

    b.reset();
    expect(b.len()).toBe(0);
});
