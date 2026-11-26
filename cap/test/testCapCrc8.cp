import { test, expect_eq } from "cap:test";
import {
    Crc8Hasher,
    crc8Smbus,
    crc8Maxim,
    crc8SaeJ1850,
    crc8Autosar,
    crc8Cdma2000,
    crc8Bluetooth,
} from "../../packages/@cap/crc8/src/index.cp";

fn main() {
    test("1. @cap/crc8 — RevEng '123456789' vectors for SMBus, Maxim & SAE-J1850", () => {
        const check = "123456789";
        expect_eq(crc8Smbus(check), 0xF4 as u8);
        expect_eq(crc8Maxim(check), 0xA1 as u8);
        expect_eq(crc8SaeJ1850(check), 0x4B as u8);
    });

    test("2. @cap/crc8 — RevEng '123456789' vectors for AUTOSAR, CDMA2000 & Bluetooth", () => {
        const check = "123456789";
        expect_eq(crc8Autosar(check), 0xDF as u8);
        expect_eq(crc8Cdma2000(check), 0xDA as u8);
        expect_eq(crc8Bluetooth(check), 0x26 as u8);
    });

    test("3. @cap/crc8 — Empty input boundary across all 6 standards", () => {
        const empty = "";
        expect_eq(crc8Smbus(empty), 0x00 as u8);
        expect_eq(crc8Maxim(empty), 0x00 as u8);
        expect_eq(crc8SaeJ1850(empty), 0x00 as u8);
        expect_eq(crc8Autosar(empty), 0x00 as u8);
        expect_eq(crc8Cdma2000(empty), 0xFF as u8);
        expect_eq(crc8Bluetooth(empty), 0x00 as u8);
    });

    test("4. @cap/crc8 — Streaming chunked update & reset consistency", () => {
        let h = Crc8Hasher.initSmbus();
        h.update("123");
        h.update("456");
        h.update("789");
        expect_eq(h.finalize(), 0xF4 as u8);
        // finalize() is idempotent
        expect_eq(h.finalize(), 0xF4 as u8);

        h.reset();
        h.update("123456789");
        expect_eq(h.finalize(), 0xF4 as u8);

        let hm = Crc8Hasher.initMaxim();
        hm.update("12");
        hm.update("34567");
        hm.update("89");
        expect_eq(hm.finalize(), 0xA1 as u8);
        hm.reset();
        expect_eq(hm.finalize(), 0x00 as u8);
    });

    test("5. @cap/crc8 — Dallas/Maxim 1-Wire ROM 8-byte frame verification", () => {
        // Canonical Dallas 1-Wire ROM example from AN27:
        // 0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00 -> CRC = 0xA2
        const rom_payload: [7]u8 = [0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00];
        const crc = crc8Maxim(rom_payload[0..7]);
        expect_eq(crc, 0xA2 as u8);

        // Appending the CRC to the 7-byte payload must yield 0x00 residue
        const full_frame: [8]u8 = [0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00, 0xA2];
        expect_eq(crc8Maxim(full_frame[0..8]), 0x00 as u8);
    });

    test("6. @cap/crc8 — Single-bit flip detection & custom polynomial", () => {
        let frame: [9]u8 = [49, 50, 51, 52, 53, 54, 55, 56, 57]; // "123456789"
        const orig = crc8Autosar(frame[0..9]);
        expect_eq(orig, 0xDF as u8);

        let byte_idx: usize = 0;
        while (byte_idx < 9) {
            let bit: u8 = 0;
            while (bit < 8) {
                const mask: u8 = (1 as u8) << (bit as u3);
                frame[byte_idx] = frame[byte_idx] ^ mask;
                const corrupted = crc8Autosar(frame[0..9]);
                expect_eq(corrupted != orig, true);
                frame[byte_idx] = frame[byte_idx] ^ mask;
                bit += 1;
            }
            byte_idx += 1;
        }

        // Custom CRC-8/ROHC (poly=0x07, init=0xFF, refIn=true, refOut=true, xorOut=0x00 -> check=0xD0)
        let rohc = Crc8Hasher.initCustom(0x07, 0xFF, true, true, 0x00);
        rohc.update("123456789");
        expect_eq(rohc.finalize(), 0xD0 as u8);
    });
}
