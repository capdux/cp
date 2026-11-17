import { test, expect_eq } from "cap:test";
import { Hash, XxHash3Stream } from "cap:hash";

fn main() {
    test("XxHash3 oneshot and incremental streaming", () => {
        let msg = "Hello, CP High-Speed Non-Cryptographic Hashing!";
        let h1: u64 = Hash.xxhash3(msg);
        expect_eq(h1 != 0, true);

        let stream = XxHash3Stream.init(0);
        stream.update("Hello, ");
        stream.update("CP High-Speed Non-Cryptographic Hashing!");
        let h2: u64 = stream.digest();
        expect_eq(h1, h2);

        let h128: u128 = Hash.xxhash128(msg);
        expect_eq(h128 != 0, true);
    });

    test("Murmur3 and CityHash64", () => {
        let text = "Distributed Actor Key Partitioning";
        let m3 = Hash.murmur3(text, 12345);
        expect_eq(m3 != 0, true);

        let city = Hash.cityhash64(text);
        expect_eq(city != 0, true);
    });

    test("CRC32 and Adler32 standard check vectors", () => {
        // Standard IEEE 802.3 CRC32 vector for "123456789" is 0xCBF43926
        let crc = Hash.crc32("123456789");
        expect_eq(crc, 0xcbf43926);

        // Standard RFC 1950 Adler32 vector for "example" is 0x0BC002ED
        let adler = Hash.adler32("example");
        expect_eq(adler, 0xbc002ed);
    });

    test("SipHash-2-4 DoS-resistant hash", () => {
        let key: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
        let payload = "Secret Agent Mailbox Payload";
        let sip64 = Hash.siphash(payload, key);
        expect_eq(sip64 != 0, true);

        let sip128 = Hash.siphash128(payload, key);
        expect_eq(sip128 != 0, true);
    });

    test("Fnv1a and Integer Avalanche Hash", () => {
        let fnv = Hash.fnv1a("identifier");
        expect_eq(fnv != 0, true);

        let h_int = Hash.hashInt(9876543210);
        expect_eq(h_int != 9876543210, true);
    });
}
