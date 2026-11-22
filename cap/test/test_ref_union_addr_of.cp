import { test, expect_eq } from "cap:test";

union(enum) RefSliceUnion {
    borrowed: []const u8,
    owned: i32,

    fn mutM(*_) {
        switch (_.*) {
            .owned(v) => {
                _.* = RefSliceUnion { owned: v + 1 };
            },
            .borrowed(_) => {
                _.* = RefSliceUnion { owned: 42 };
            },
        }
    }

    fn getOwned() -> i32 {
        return switch (_) {
            .owned(v) => v,
            .borrowed(s) => s.len as i32,
        };
    }
}

ref struct RefSliceStruct {
    data: []const u8,
    count: i32,

    fn bump(*_) {
        _.count += 1;
    }
}

struct Caller {
    fn takeUnion(*_, p: *RefSliceUnion) {
        p.mutM();
    }

    fn takeRefStruct(*_, p: *RefSliceStruct) {
        p.bump();
    }

    fn runUnion(*_) -> i32 {
        let u: RefSliceUnion = RefSliceUnion { borrowed: "hello" };
        _.takeUnion(&u);
        return u.getOwned();
    }

    fn runUnionOwned(*_) -> i32 {
        let u: RefSliceUnion = RefSliceUnion { owned: 10 };
        _.takeUnion(&u);
        return u.getOwned();
    }

    fn runRefStruct(*_) -> i32 {
        let s = RefSliceStruct { data: "abc", count: 7 };
        _.takeRefStruct(&s);
        return s.count;
    }
}

fn freeTakeUnion(p: *RefSliceUnion) {
    p.mutM();
}

fn main() {
    test("ref union with []const u8 field preserves &u in call args", () => {
        let c = Caller {};
        expect_eq(c.runUnion(), 42);
        expect_eq(c.runUnionOwned(), 11);

        let u2: RefSliceUnion = RefSliceUnion { owned: 99 };
        freeTakeUnion(&u2);
        expect_eq(u2.getOwned(), 100);
    });

    test("ref struct with []const u8 field preserves &s in call args", () => {
        let c = Caller {};
        expect_eq(c.runRefStruct(), 8);
    });
}
