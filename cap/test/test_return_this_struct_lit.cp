import { test, expect_eq } from "cap:test";

pub struct S {
    x: i32,
    y: i32 = 0,

    static fn make(): @This() {
        return @This(){ x: 1 };
    }

    static fn makeDot(): @This() {
        return @This(){ .x = 2, .y = 3 };
    }

    static fn makeShorthand(x: i32, y: i32): @This() {
        let tmp = @This(){ x, y };
        return tmp;
    }
}

pub struct Empty {
    x: i32 = 42,

    static fn makeDefault(): @This() {
        return @This(){};
    }
}

fn main() {
    test("@This() struct literal forms", () => {
        let s1 = S.make();
        expect_eq(s1.x, 1);
        expect_eq(s1.y, 0);

        let s2 = S.makeDot();
        expect_eq(s2.x, 2);
        expect_eq(s2.y, 3);

        let s3 = S.makeShorthand(10, 20);
        expect_eq(s3.x, 10);
        expect_eq(s3.y, 20);

        let e = Empty.makeDefault();
        expect_eq(e.x, 42);
    });
}
