import { test, expect_eq } from "cap:test";
import { BigInt, BigInt256 } from "cap:bigint";

fn main() {
    test("BigInt basic arithmetic", () => {
        let a = BigInt.fromInt(100) catch unreachable;
        defer a.deinit();
        let b = BigInt.fromInt(30) catch unreachable;
        defer b.deinit();

        let sum = a.add(&b) catch unreachable;
        defer sum.deinit();
        expect_eq(sum.toString(10), "130");

        let sub = a.sub(&b) catch unreachable;
        defer sub.deinit();
        expect_eq(sub.toString(10), "70");

        let mul = a.mul(&b) catch unreachable;
        defer mul.deinit();
        expect_eq(mul.toString(10), "3000");

        let q = a.divTrunc(&b) catch unreachable;
        defer q.deinit();
        expect_eq(q.toString(10), "3");

        let r = a.rem(&b) catch unreachable;
        defer r.deinit();
        expect_eq(r.toString(10), "10");
    });

    test("BigInt256 basics", () => {
        const a = BigInt256.fromU64(100);
        const b = BigInt256.fromU64(200);
        
        const sum = a.add(b);
        expect_eq(sum.limbs[0], 300);
        
        const sub = b.sub(a);
        expect_eq(sub.limbs[0], 100);
        
        const mul = a.mul(b);
        expect_eq(mul.limbs[0], 20000);
    });

    test("BigInt256 Hex parsing", () => {
        const val = BigInt256.fromHex("0xdeadbeef123456789abcdef0") catch unreachable;
        expect_eq(val.toHex(), "deadbeef123456789abcdef0");
    });
}
