// cap/test/testCapArrayvec.cp
// Comprehensive unit tests for @cap/arrayvec (Rust arrayvec parity)

import { test, expect_eq } from "cap:test";
import {
    ArrayString64,
    ArrayString256,
    ArrayVecI64,
    ArrayVecU64,
} from "../../packages/@cap/arrayvec/src/index.cp";

fn main() {
    test("1. @cap/arrayvec — ArrayString64 building, popByte, truncate & clear", () => {
        let s = ArrayString64.fromStr("hello").?;
        expect_eq(s.len(), 5 as usize);
        expect_eq(s.capacity(), 64 as usize);
        expect_eq(s.remaining(), 59 as usize);
        expect_eq(s.isEmpty(), false);
        expect_eq(s.isFull(), false);

        expect_eq(s.tryPushByte(' ' as u8), true);
        expect_eq(s.tryPushStr("world!"), true);
        expect_eq(s.asStr(), "hello world!");

        expect_eq(s.popByte().?, '!' as u8);
        expect_eq(s.asStr(), "hello world");

        s.truncate(5 as usize);
        expect_eq(s.asStr(), "hello");

        s.clear();
        expect_eq(s.isEmpty(), true);
        expect_eq(s.popByte() == null, true);
    });

    test("2. @cap/arrayvec — ArrayString64 capacity guard & pushStrTruncating", () => {
        let s = ArrayString64.init();
        let i: usize = 0;
        while (i < 60) {
            expect_eq(s.tryPushByte('a' as u8), true);
            i += 1;
        }
        // Remaining = 4 bytes -> pushing 6 bytes fails atomically
        expect_eq(s.tryPushStr("123456"), false);
        expect_eq(s.len(), 60 as usize);

        // Truncating push writes exactly 4 bytes
        const written = s.pushStrTruncating("123456");
        expect_eq(written, 4 as usize);
        expect_eq(s.isFull(), true);
        expect_eq(s.tryPushByte('z' as u8), false);
        expect_eq(s.asStr()[60..64], "1234");
    });

    test("3. @cap/arrayvec — ArrayString256 larger stack buffer & fromStr overflow", () => {
        let s = ArrayString256.init();
        expect_eq(s.capacity(), 256 as usize);
        expect_eq(s.tryPushStr("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"), true);
        expect_eq(s.len(), 51 as usize);

        // 65-byte string exceeds ArrayString64.fromStr
        const too_long = "01234567890123456789012345678901234567890123456789012345678901234";
        expect_eq(ArrayString64.fromStr(too_long) == null, true);
        expect_eq(ArrayString256.fromStr(too_long) != null, true);
    });

    test("4. @cap/arrayvec — ArrayVecI64 push, pop, get, set & ordered insert/remove", () => {
        let v = ArrayVecI64.init();
        expect_eq(v.isEmpty(), true);

        expect_eq(v.tryPush(10), true);
        expect_eq(v.tryPush(30), true);
        expect_eq(v.tryInsert(1 as usize, 20), true);
        expect_eq(v.len(), 3 as usize);
        expect_eq(v.get(0 as usize).?, 10 as i64);
        expect_eq(v.get(1 as usize).?, 20 as i64);
        expect_eq(v.get(2 as usize).?, 30 as i64);

        expect_eq(v.set(1 as usize, -20), true);
        expect_eq(v.set(9 as usize, 0), false);
        expect_eq(v.remove(1 as usize).?, -20 as i64);
        expect_eq(v.len(), 2 as usize);
        expect_eq(v.get(1 as usize).?, 30 as i64);

        expect_eq(v.pop().?, 30 as i64);
        expect_eq(v.pop().?, 10 as i64);
        expect_eq(v.pop() == null, true);
    });

    test("5. @cap/arrayvec — ArrayVecU64 O(1) swapRemove & truncate", () => {
        let v = ArrayVecU64.init();
        * = v.tryPush(100 as u64);
        * = v.tryPush(200 as u64);
        * = v.tryPush(300 as u64);
        * = v.tryPush(400 as u64);

        // swapRemove(1) replaces slot 1 with last element (400)
        expect_eq(v.swapRemove(1 as usize).?, 200 as u64);
        expect_eq(v.len(), 3 as usize);
        expect_eq(v.get(0 as usize).?, 100 as u64);
        expect_eq(v.get(1 as usize).?, 400 as u64);
        expect_eq(v.get(2 as usize).?, 300 as u64);

        v.truncate(1 as usize);
        expect_eq(v.len(), 1 as usize);
        expect_eq(v.get(0 as usize).?, 100 as u64);
        expect_eq(v.swapRemove(5 as usize) == null, true);
    });

    test("6. @cap/arrayvec — ArrayVecU64 full capacity (64) & out-of-bounds guards", () => {
        let v = ArrayVecU64.init();
        let i: u64 = 0;
        while (i < 64) {
            expect_eq(v.tryPush(i), true);
            i += 1;
        }
        expect_eq(v.isFull(), true);
        expect_eq(v.tryPush(999 as u64), false);
        expect_eq(v.tryInsert(0 as usize, 999 as u64), false);
        expect_eq(v.get(64 as usize) == null, true);
        expect_eq(v.remove(64 as usize) == null, true);

        v.clear();
        expect_eq(v.isEmpty(), true);
        expect_eq(v.isFull(), false);
    });
}
