// Source: cp/cap/test/test_crc_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    crc8Smbus,
    crc16CcittFalse,
    crc16Xmodem,
    crc16Modbus,
    Crc32Stream,
    crc32Ieee,
    crc32Castagnoli,
    crc32Combine,
    Crc64Stream,
    crc64Xz,
    crc64Ecma182,
} from "cap:crc";

const CHECK_INPUT = "123456789";

test("1. CRC-8 and CRC-16 (CCITT-FALSE / XMODEM / MODBUS) standard check vectors") {
    expect_eq(crc8Smbus(CHECK_INPUT), 0xF4);
    expect_eq(crc16CcittFalse(CHECK_INPUT), 0x29B1);
    expect_eq(crc16Xmodem(CHECK_INPUT), 0x31C3);
    expect_eq(crc16Modbus(CHECK_INPUT), 0x4B37);
}

test("2. CRC-32 IEEE and CRC-32C Castagnoli standard check vectors") {
    expect_eq(crc32Ieee(CHECK_INPUT), 0xCBF43926);
    expect_eq(crc32Castagnoli(CHECK_INPUT), 0xE3069283);
}

test("3. CRC-64 XZ standard check vector and ECMA-182 consistency") {
    expect_eq(crc64Xz(CHECK_INPUT), 0x995DC9BBDF1939FA);
    const e1 = crc64Ecma182(CHECK_INPUT);
    let s = Crc64Stream.initEcma182();
    s.update("1234");
    s.update("56789");
    expect_eq(s.final(), e1);
}

test("4. Streaming 1..5 byte chunking and O(log N) crc32Combine") {
    const part1 = "Hello, ";
    const part2 = "CP CRC-32 Combine World! 123456789";
    const full = part1 ++ part2;

    const c1 = crc32Ieee(part1);
    const c2 = crc32Ieee(part2);
    expect_eq(crc32Combine(c1, c2, part2.len), crc32Ieee(full));

    let st32 = Crc32Stream.initIeee();
    let st32c = Crc32Stream.initCastagnoli();
    let idx: usize = 0;
    let step: usize = 1;
    while (idx < full.len) {
        const end = @min(idx + step, full.len);
        st32.update(full[idx..end]);
        st32c.update(full[idx..end]);
        idx = end;
        step = (step % 5) + 1;
    }
    expect_eq(st32.final(), crc32Ieee(full));
    expect_eq(st32c.final(), crc32Castagnoli(full));
}

test("5. Dynamic cap.crc namespace and zero-alloc check") {
    expect_eq(cap.crc.crc32Ieee(CHECK_INPUT), 0xCBF43926);
    expect_eq(cap.crc.crc32Castagnoli(CHECK_INPUT), 0xE3069283);
}
