import { test, expect_eq } from "cap:test";

pub enum M: u8 {
    a,
    b,
    inline fn ok(): bool { return true; }
    pub inline fn isA(): bool { return _ == M.a; }
    noinline fn code(): u8 { return @intFromEnum(_); }
    static inline fn defaultVal(): M { return M.a; }
}

fn main() {
    test("enum inline/noinline/static methods", () => {
        expect_eq(M.ok(), true);
        let v = M.defaultVal();
        expect_eq(v.isA(), true);
        expect_eq(v.code(), 0);
        let b = M.b;
        expect_eq(b.isA(), false);
        expect_eq(b.code(), 1);
    });
}
