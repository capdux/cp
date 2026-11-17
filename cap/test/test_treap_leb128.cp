import { test, expect_eq } from "cap:test";
import { Leb128 } from "cap:col";

fn main() {
    test("leb128 encode/decode", () => {
        let val: u64 = 123456;
        expect_eq(val, val);
    });
}
