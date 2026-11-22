import { test, expect_eq } from "cap:test";

pub struct Counter {
    x: i32,
}

pub union(enum) U {
    a: i32,
    b: i32,
    c: Counter,

    fn mutF(*_) {
        switch (_.*) {
            .a *v => { v.* += 1; },
            .b => {},
            .c *p => { p.x += 5; },
        }
    }

    fn mutMerged(*_) {
        switch (_) {
            .a, .b *v => { v.* += 10; },
            .c *p => { p.x += 20; },
        }
    }

    fn mutGuarded(*_): i32 {
        return switch (_.*) {
            .a *v if v.* > 0 => {
                v.* += 100;
                v.*
            },
            .a *v => {
                v.* = -1;
                v.*
            },
            .b *v => {
                v.* += 2;
                v.*
            },
            .c *p => {
                p.x += 3;
                p.x
            },
        };
    }

    fn getVal(): i32 {
        return switch (_) {
            .a v => v,
            .b v => v,
            .c p => p.x,
        };
    }
}

fn main() {
    test("switch pipe-less mutable payload capture *v", () => {
        let u1 = U{ a: 10 };
        u1.mutF();
        expect_eq(u1.getVal(), 11);

        let u2 = U{ b: 20 };
        u2.mutF();
        expect_eq(u2.getVal(), 20);

        let u3 = U{ c: Counter{ x: 5 } };
        u3.mutF();
        expect_eq(u3.getVal(), 10);
    });

    test("switch multi-tag merged mutable payload capture .a, .b *v", () => {
        let u1 = U{ a: 1 };
        let u2 = U{ b: 2 };
        let u3 = U{ c: Counter{ x: 3 } };
        u1.mutMerged();
        u2.mutMerged();
        u3.mutMerged();
        expect_eq(u1.getVal(), 11);
        expect_eq(u2.getVal(), 12);
        expect_eq(u3.getVal(), 23);
    });

    test("switch guarded mutable payload capture .a *v if ...", () => {
        let u1 = U{ a: 5 };
        expect_eq(u1.mutGuarded(), 105);
        expect_eq(u1.getVal(), 105);

        let u2 = U{ a: 0 };
        expect_eq(u2.mutGuarded(), -1);
        expect_eq(u2.getVal(), -1);

        let u3 = U{ b: 40 };
        expect_eq(u3.mutGuarded(), 42);
        expect_eq(u3.getVal(), 42);
    });
}
