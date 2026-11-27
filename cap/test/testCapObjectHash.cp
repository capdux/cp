import { test, expect_eq } from "cap:test";
import {
    OHSH_KIND_NULL,
    OHSH_KIND_BOOL,
    OHSH_KIND_NUMBER,
    OHSH_KIND_STRING,
    HashField,
    hashBytesFnv64,
    hashStringFnv64,
    hashFieldsFnv64,
    hashFieldsHex,
} from "../../packages/object-hash/src/index.cp";

fn main() {
    test("1. @cap/object-hash — Standard FNV-1a 64-bit vectors for string and bytes", () => {
        // FNV-1a 64-bit("") == 0xcbf29ce484222325
        const empty_expected: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
        expect_eq(hashStringFnv64(""), empty_expected);

        // FNV-1a 64-bit("foobar") == 0x85944171f73967e8
        const foobar_expected: u64 = ((0x85944171 as u64) << 32) | (0xf73967e8 as u64);
        expect_eq(hashStringFnv64("foobar"), foobar_expected);

        const raw: [6]u8 = ['f', 'o', 'o', 'b', 'a', 'r'];
        expect_eq(hashBytesFnv64(raw[0..]), foobar_expected);
    });

    test("2. @cap/object-hash — Order-independent hashing across permutations", () => {
        const obj_a: [4]HashField = [
            HashField { key: "name", value: "cap", kind: OHSH_KIND_STRING },
            HashField { key: "ver", value: "1", kind: OHSH_KIND_NUMBER },
            HashField { key: "active", value: "true", kind: OHSH_KIND_BOOL },
            HashField { key: "meta", value: "", kind: OHSH_KIND_NULL },
        ];
        const obj_b: [4]HashField = [
            HashField { key: "meta", value: "", kind: OHSH_KIND_NULL },
            HashField { key: "active", value: "true", kind: OHSH_KIND_BOOL },
            HashField { key: "name", value: "cap", kind: OHSH_KIND_STRING },
            HashField { key: "ver", value: "1", kind: OHSH_KIND_NUMBER },
        ];
        expect_eq(hashFieldsFnv64(obj_a[0..]), hashFieldsFnv64(obj_b[0..]));
    });

    test("3. @cap/object-hash — Type kind distinction (number '1' vs string '1')", () => {
        const num_obj: [1]HashField = [
            HashField { key: "id", value: "1", kind: OHSH_KIND_NUMBER },
        ];
        const str_obj: [1]HashField = [
            HashField { key: "id", value: "1", kind: OHSH_KIND_STRING },
        ];
        expect_eq(hashFieldsFnv64(num_obj[0..]) != hashFieldsFnv64(str_obj[0..]), true);
    });

    test("4. @cap/object-hash — Value and key changes produce distinct hashes", () => {
        const a: [2]HashField = [
            HashField { key: "a", value: "bc", kind: OHSH_KIND_STRING },
            HashField { key: "d", value: "e", kind: OHSH_KIND_STRING },
        ];
        const b: [2]HashField = [
            HashField { key: "ab", value: "c", kind: OHSH_KIND_STRING },
            HashField { key: "d", value: "e", kind: OHSH_KIND_STRING },
        ];
        expect_eq(hashFieldsFnv64(a[0..]) != hashFieldsFnv64(b[0..]), true);
    });

    test("5. @cap/object-hash — hashFieldsHex() produces deterministic 16-char lowercase hex", () => {
        const a: [2]HashField = [
            HashField { key: "z", value: "99", kind: OHSH_KIND_NUMBER },
            HashField { key: "a", value: "ok", kind: OHSH_KIND_STRING },
        ];
        const b: [2]HashField = [
            HashField { key: "a", value: "ok", kind: OHSH_KIND_STRING },
            HashField { key: "z", value: "99", kind: OHSH_KIND_NUMBER },
        ];
        let hex_a: [16]u8 = undefined;
        let hex_b: [16]u8 = undefined;
        const na: usize = hashFieldsHex(a[0..], hex_a[0..]);
        const nb: usize = hashFieldsHex(b[0..], hex_b[0..]);
        expect_eq(na, 16);
        expect_eq(nb, 16);
        expect_eq(hex_a[0..na], hex_b[0..nb]);

        // Verify all 16 chars are lowercase hex
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            const c: u8 = hex_a[i];
            const is_hex: bool = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
            expect_eq(is_hex, true);
        }
    });

    test("6. @cap/object-hash — Empty fields and short hex buffer truncation safety", () => {
        const empty: []const HashField = [];
        const empty_expected: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
        expect_eq(hashFieldsFnv64(empty), empty_expected);

        let full_hex: [16]u8 = undefined;
        expect_eq(hashFieldsHex(empty, full_hex[0..]), 16);
        expect_eq(full_hex[0..16], "cbf29ce484222325");

        let short_hex: [8]u8 = undefined;
        expect_eq(hashFieldsHex(empty, short_hex[0..]), 8);
        expect_eq(short_hex[0..8], "cbf29ce4");
    });
}
