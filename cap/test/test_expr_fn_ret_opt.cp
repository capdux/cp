import { test, expect_eq } from "cap:test";

pub struct T {
    x: i32,
    static fn fromJS(val: u64): (*T)? {
        * = val;
        return null;
    }
    static fn pair(a: i32, b: u8): ([]const u8)? {
        * = a;
        * = b;
        return null;
    }
}

fn checkFromJS(): bool {
    return @TypeOf(&T.fromJS) == fn (val: u64): (*T)?;
}

fn checkPair(): bool {
    return @TypeOf(&T.pair) == fn (a: i32, b: u8): ([]const u8)?;
}

fn main() {
    test("expression-position fn type with postfix optional return", () => {
        expect_eq(checkFromJS(), true);
        expect_eq(checkPair(), true);
    });
}
