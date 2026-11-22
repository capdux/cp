import { test, expect_eq } from "cap:test";

pub struct Payload {
    x: i32,
}

pub union(enum) Tagged {
    a: Payload,
    b: Payload,
    c: i32,

    fn extract(): i32 {
        return switch (_) {
            .a, .b v => v.x,
            .c => 0,
        };
    }

    fn extractGuarded(min_val: i32): i32 {
        return switch (_) {
            .a, .b v if v.x >= min_val => v.x,
            .a, .b => -1,
            .c n => n,
        };
    }
}

fn main() {
    test("switch multi-tag merged arm with payload capture", () => {
        const u1 = Tagged{ a: Payload{ x: 42 } };
        const u2 = Tagged{ b: Payload{ x: 99 } };
        const u3 = Tagged{ c: 7 };

        expect_eq(u1.extract(), 42);
        expect_eq(u2.extract(), 99);
        expect_eq(u3.extract(), 0);

        expect_eq(u1.extractGuarded(10), 42);
        expect_eq(u1.extractGuarded(50), -1);
        expect_eq(u2.extractGuarded(50), 99);
        expect_eq(u3.extractGuarded(0), 7);
    });
}
