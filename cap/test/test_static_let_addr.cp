import { test, expect_eq } from "cap:test";

struct Counter {
    val: u32,
}

struct T {
    id: u32 = 1,
    static let q: u32 = 0;
    static let c: Counter = Counter { val: 10 };

    static fn get(): *u32 {
        return &q;
    }

    static fn getViaLocal(): *u32 {
        let p = &q;
        return p;
    }

    static fn getQualified(): *u32 {
        return &T.q;
    }

    static fn getFieldAddr(): *u32 {
        return &c.val;
    }

    fn getFromInstance(): *u32 {
        let _id = _.id;
        return &q;
    }
}

fn main() {
    test("struct static let address return", () => {
        let p1 = T.get();
        expect_eq(p1.*, 0);
        p1.* = 42;

        let p2 = T.getViaLocal();
        expect_eq(p2.*, 42);

        let p3 = T.getQualified();
        expect_eq(p3.*, 42);

        let t = T {};
        let p4 = t.getFromInstance();
        expect_eq(p4.*, 42);

        let pf = T.getFieldAddr();
        expect_eq(pf.*, 10);
        pf.* = 99;
        expect_eq(T.c.val, 99);
    });
}
