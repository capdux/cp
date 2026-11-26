import { test, expect_eq } from "cap:test";
import {
    Crc16Hasher,
    crc16CcittFalse,
    crc16Xmodem,
    crc16Modbus,
    crc16Kermit,
    crc16Arc,
    crc16Usb,
} from "../../packages/@cap/crc16/src/index.cp";

fn main() {
    test("CCITT-FALSE and XMODEM standard 123456789 vectors", () => {
        expect_eq(crc16CcittFalse("123456789"), 0x29B1);
        expect_eq(crc16Xmodem("123456789"), 0x31C3);
    });

    test("MODBUS and KERMIT standard 123456789 vectors", () => {
        expect_eq(crc16Modbus("123456789"), 0x4B37);
        expect_eq(crc16Kermit("123456789"), 0x2189);
    });

    test("ARC/IBM and USB standard 123456789 vectors", () => {
        expect_eq(crc16Arc("123456789"), 0xBB3D);
        expect_eq(crc16Usb("123456789"), 0xB4C8);
    });

    test("empty inputs match init ^ xorout", () => {
        expect_eq(crc16CcittFalse(""), 0xFFFF);
        expect_eq(crc16Xmodem(""), 0x0000);
        expect_eq(crc16Modbus(""), 0xFFFF);
        expect_eq(crc16Usb(""), 0x0000);
    });

    test("streaming chunked updates and reset match one-shot", () => {
        let h = Crc16Hasher.initModbus();
        h.update("123");
        h.update("456");
        h.update("789");
        expect_eq(h.finalize(), 0x4B37);
        h.reset();
        h.update("123456789");
        expect_eq(h.finalize(), 0x4B37);
    });

    test("single-bit flip detection on Modbus RTU frame", () => {
        let a: [6]u8 = [0x01, 0x03, 0x00, 0x00, 0x00, 0x0A];
        let b: [6]u8 = [0x01, 0x03, 0x00, 0x00, 0x00, 0x0B];
        let ca = crc16Modbus(a[0..6]);
        let cb = crc16Modbus(b[0..6]);
        expect_eq(ca != cb, true);
        expect_eq(ca, 0xCDC5);
    });
}
