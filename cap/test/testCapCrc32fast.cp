import { test, expect_eq } from "cap:test";
import {
    Crc32Hasher, combineCrc32, hashBytes, hashStr
} from "../../packages/@cap/crc32fast/src/index.cp";

fn main() {
    test("1. @cap/crc32fast — standard check vector '123456789' == 0xCBF43926", () => {
        expect_eq(hashStr("123456789"), 3421780262); // 0xCBF43926
    });

    test("2. @cap/crc32fast — empty input hashes to 0", () => {
        expect_eq(hashStr(""), 0);
        let empty: [1]u8 = [0];
        expect_eq(hashBytes(empty[0..0]), 0);
    });

    test("3. @cap/crc32fast — streaming incremental update matches one-shot", () => {
        let h = Crc32Hasher.new();
        h.updateStr("1234");
        h.updateStr("56789");
        expect_eq(h.finalize(), 3421780262);
        expect_eq(h.amount, 9);
    });

    test("4. @cap/crc32fast — GF(2) combine merges two independent chunk hashes", () => {
        let h1 = Crc32Hasher.new();
        h1.updateStr("hello ");
        let h2 = Crc32Hasher.new();
        h2.updateStr("world!");
        h1.combine(h2);
        expect_eq(h1.finalize(), hashStr("hello world!"));
        expect_eq(h1.amount, 12);
    });

    test("5. @cap/crc32fast — combineCrc32 with zero length returns crc1", () => {
        let c1 = hashStr("abc");
        expect_eq(combineCrc32(c1, 0, 0), c1);
        expect_eq(combineCrc32(hashStr("foo"), hashStr("bar"), 3), hashStr("foobar"));
    });

    test("6. @cap/crc32fast — reset() and newWithInitial() behave identically", () => {
        let h = Crc32Hasher.new();
        h.updateStr("temp");
        h.reset();
        expect_eq(h.finalize(), 0);
        expect_eq(h.amount, 0);
        h.updateStr("123456789");
        expect_eq(h.finalize(), 3421780262);

        let h_init = Crc32Hasher.newWithInitial(hashStr("1234"));
        h_init.updateStr("56789");
        expect_eq(h_init.finalize(), 3421780262);
    });
}
