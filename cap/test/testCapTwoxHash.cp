import { test, expect_eq } from "cap:test";
import { XxHash32, XxHash64 } from "../../packages/twox-hash/src/index.cp";

fn main() {
    test("1. @cap/twox-hash — XXH32 official Cyan4973 vectors (empty & 'abc')", () => {
        let empty: [1]u8 = [0];
        expect_eq(XxHash32.oneshot(0, empty[0..0]), 46947589); // 0x02CC5D05

        let abc: [3]u8 = [97, 98, 99];
        expect_eq(XxHash32.oneshot(0, abc[0..3]), 852579327); // 0x32D153FF
    });

    test("2. @cap/twox-hash — XXH64 official Cyan4973 vectors (empty & 'abc')", () => {
        let empty: [1]u8 = [0];
        let exp_empty: u64 = ((4014398263 as u64) << 32) | (1373170073 as u64); // 0xEF46DB3751D8E999
        expect_eq(XxHash64.oneshot(0, empty[0..0]), exp_empty);

        let abc: [3]u8 = [97, 98, 99];
        let exp_abc: u64 = ((1153182965 as u64) << 32) | (2910259609 as u64); // 0x44BC2CF5AD770999
        expect_eq(XxHash64.oneshot(0, abc[0..3]), exp_abc);
    });

    test("3. @cap/twox-hash — XXH32 streaming multi-stripe (>=16B) matches oneshot", () => {
        let buf: [64]u8 = [0; 64];
        let i: usize = 0;
        while (i < 64) {
            buf[i] = ((i * 7 + 3) & 255) as u8;
            i = i + 1;
        }
        let one = XxHash32.oneshot(42, buf[0..64]);

        let h = XxHash32.withSeed(42);
        h.write(buf[0..5]);
        h.write(buf[5..23]);
        h.write(buf[23..64]);
        expect_eq(h.finish(), one);
    });

    test("4. @cap/twox-hash — XXH64 streaming multi-stripe (>=32B) matches oneshot", () => {
        let buf: [80]u8 = [0; 80];
        let i: usize = 0;
        while (i < 80) {
            buf[i] = ((i * 13 + 5) & 255) as u8;
            i = i + 1;
        }
        let one = XxHash64.oneshot(123456, buf[0..80]);

        let h = XxHash64.withSeed(123456);
        h.write(buf[0..11]);
        h.write(buf[11..47]);
        h.write(buf[47..80]);
        expect_eq(h.finish(), one);
    });

    test("5. @cap/twox-hash — writeStr matches byte slice hashing for XXH32 & XXH64", () => {
        let h32 = XxHash32.withSeed(0);
        h32.writeStr("a");
        h32.writeStr("bc");
        expect_eq(h32.finish(), 852579327);

        let h64 = XxHash64.withSeed(0);
        h64.writeStr("ab");
        h64.writeStr("c");
        let exp_abc: u64 = ((1153182965 as u64) << 32) | (2910259609 as u64);
        expect_eq(h64.finish(), exp_abc);
    });

    test("6. @cap/twox-hash — seed sensitivity & 4-byte/8-byte tail paths", () => {
        let data: [13]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13];
        let a32 = XxHash32.oneshot(0, data[0..13]);
        let b32 = XxHash32.oneshot(1, data[0..13]);
        expect_eq(a32 != b32, true);

        let a64 = XxHash64.oneshot(0, data[0..13]);
        let b64 = XxHash64.oneshot(1, data[0..13]);
        expect_eq(a64 != b64, true);
    });
}
