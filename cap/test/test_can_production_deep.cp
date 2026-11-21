// Source: cp/cap/test/test_can_production_deep.cp (Production deep tests for cap:can)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CAN_FRAME_SIZE,
    CANFD_FRAME_SIZE,
    CAN_MAX_DLEN,
    CANFD_MAX_DLEN,
    CAN_SFF_MASK,
    CAN_EFF_MASK,
    CAN_EFF_FLAG,
    CAN_RTR_FLAG,
    CAN_ERR_FLAG,
    CANFD_BRS,
    CANFD_ESI,
    CANFD_FDF,
    canFdDlcToLen,
    canFdLenToDlc,
    CanId,
    CanFrame,
    J1939Id,
    decodeJ1939,
    encodeJ1939,
    encodeJ1939Pdu1,
    parseSocketCan,
    parseSocketCanBe,
    parseSocketCanFd,
    parseSocketCanFdBe,
    encodeSocketCan,
    encodeSocketCanBe,
    encodeSocketCanFd,
    encodeSocketCanFdBe,
    extractSignalIntel,
    extractSignalMotorola,
    signExtend,
} from "cap:can";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. Classic CAN 2.0 (16B) and CAN FD (72B) SocketCAN LE/BE round-trip", () => {
    expect_eq(CAN_FRAME_SIZE, 16);
    expect_eq(CANFD_FRAME_SIZE, 72);
    expect_eq(CAN_MAX_DLEN, 8 as u8);
    expect_eq(CANFD_MAX_DLEN, 64 as u8);

    // Classic 11-bit CAN 2.0A frame
    let data20: [64]u8 = [0; 64];
    data20[0] = 0x11;
    data20[1] = 0x22;
    data20[2] = 0x33;
    data20[3] = 0x44;
    const f20 = CanFrame {
        can_id: CanId.standard(0x123),
        len: 4,
        flags: 0,
        is_fd: false,
        data: data20,
    };
    expect_eq(f20.can_id.id(), 0x123 as u32);
    expect_eq(f20.can_id.isExtended(), false);
    expect_eq(f20.can_id.isRtr(), false);
    expect_eq(f20.can_id.isError(), false);

    let wire16: [16]u8 = [0; 16];
    const n16 = encodeSocketCan(f20, wire16[0..]) ?? unreachable;
    expect_eq(n16, CAN_FRAME_SIZE);
    const p20 = parseSocketCan(wire16[0..]) ?? unreachable;
    expect_eq(p20.can_id.id(), 0x123 as u32);
    expect_eq(p20.len, 4 as u8);
    expect_eq(p20.is_fd, false);
    expectBytesEq(p20.payload(), f20.payload());

    // Big-endian round-trip with RTR + extended ID
    const eff_rtr = CanId { raw: CanId.extended(0x18FEF100).raw | CAN_RTR_FLAG };
    expect_eq(eff_rtr.isExtended(), true);
    expect_eq(eff_rtr.isRtr(), true);
    expect_eq(eff_rtr.id(), 0x18FEF100 as u32);
    const f_be = CanFrame {
        can_id: eff_rtr,
        len: 0,
        flags: 0,
        is_fd: false,
        data: [0; 64],
    };
    const n_be = encodeSocketCanBe(f_be, wire16[0..]) ?? unreachable;
    expect_eq(n_be, CAN_FRAME_SIZE);
    const p_be = parseSocketCanBe(wire16[0..]) ?? unreachable;
    expect_eq(p_be.can_id.raw, eff_rtr.raw);
    expect_eq(p_be.can_id.id(), 0x18FEF100 as u32);

    // CAN FD 72B frame with 16 bytes payload + BRS | FDF
    let data_fd: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 16) {
        data_fd[i] = (i as u8) + 0xA0;
        i += 1;
    }
    const f_fd = CanFrame {
        can_id: CanId.extended(0x0CF00400),
        len: 16,
        flags: CANFD_BRS | CANFD_FDF,
        is_fd: true,
        data: data_fd,
    };
    let wire72: [72]u8 = [0; 72];
    const n72 = encodeSocketCanFd(f_fd, wire72[0..]) ?? unreachable;
    expect_eq(n72, CANFD_FRAME_SIZE);
    const p_fd = parseSocketCanFd(wire72[0..]) ?? unreachable;
    expect_eq(p_fd.is_fd, true);
    expect_eq(p_fd.len, 16 as u8);
    expect_eq(p_fd.hasBrs(), true);
    expect_eq(p_fd.hasEsi(), false);
    expect_eq(p_fd.can_id.id(), 0x0CF00400 as u32);
    expectBytesEq(p_fd.payload(), data_fd[0..16]);

    // Big-endian FD round-trip
    const n72_be = encodeSocketCanFdBe(f_fd, wire72[0..]) ?? unreachable;
    expect_eq(n72_be, CANFD_FRAME_SIZE);
    const p_fd_be = parseSocketCanFdBe(wire72[0..]) ?? unreachable;
    expect_eq(p_fd_be.can_id.id(), 0x0CF00400 as u32);
    expect_eq(p_fd_be.len, 16 as u8);
});

test("2. CAN FD DLC 0..15 table lookup and inverse mapping", () => {
    const expected_lens: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64];
    let dlc: u8 = 0;
    while (dlc < 16) {
        const mapped_len = canFdDlcToLen(dlc) ?? unreachable;
        expect_eq(mapped_len, expected_lens[dlc as usize]);
        const back_dlc = canFdLenToDlc(mapped_len) ?? unreachable;
        expect_eq(back_dlc, dlc);
        dlc += 1;
    }

    // Invalid DLCs and non-canonical lengths return null
    expect_eq(canFdDlcToLen(16) == null, true);
    expect_eq(canFdDlcToLen(255) == null, true);
    expect_eq(canFdLenToDlc(9) == null, true);
    expect_eq(canFdLenToDlc(15) == null, true);
    expect_eq(canFdLenToDlc(65) == null, true);
});

test("3. SAE J1939 29-bit PDU1 & PDU2 PGN encode/decode (EEC1 0x0CF00400)", () => {
    // PDU2 broadcast: EEC1 (0x0CF00400) -> PGN 61444 (0xF004), priority 3, SA 0
    const eec1 = decodeJ1939(0x0CF00400);
    expect_eq(eec1.priority, 3 as u8);
    expect_eq(eec1.edp, 0 as u8);
    expect_eq(eec1.dp, 0 as u8);
    expect_eq(eec1.pf, 0xF0 as u8);
    expect_eq(eec1.ps, 0x04 as u8);
    expect_eq(eec1.sa, 0x00 as u8);
    expect_eq(eec1.pgn, 61444 as u32);
    expect_eq(eec1.isPdu2(), true);
    expect_eq(eec1.isPdu1(), false);

    // Round-trip encodeJ1939
    expect_eq(encodeJ1939(3, 61444, 0), 0x0CF00400 as u32);

    // Also works when CAN_EFF_FLAG is set in input
    const eec1_eff = decodeJ1939(0x0CF00400 | CAN_EFF_FLAG);
    expect_eq(eec1_eff.pgn, 61444 as u32);

    // PDU1 peer-to-peer: Request PGN 0xEA00 (59904) sent to DA=0xFF from SA=0xF9 with priority 6
    const req_id = encodeJ1939Pdu1(6, 0x00EA00, 0xFF, 0xF9);
    expect_eq(req_id, 0x18EAFFF9 as u32);
    const decoded_pdu1 = decodeJ1939(req_id);
    expect_eq(decoded_pdu1.priority, 6 as u8);
    expect_eq(decoded_pdu1.pf, 0xEA as u8);
    expect_eq(decoded_pdu1.ps, 0xFF as u8);
    expect_eq(decoded_pdu1.sa, 0xF9 as u8);
    // Low 8 bits zeroed for PDU1!
    expect_eq(decoded_pdu1.pgn, 0x00EA00 as u32);
    expect_eq(decoded_pdu1.isPdu1(), true);
});

test("4. DBC Intel & Motorola bitfield signal extraction and two's-complement signExtend", () => {
    // Intel (little-endian): bytes [0x34, 0x12, 0xF0, 0x0F]
    // start_bit=0, len=16 -> 0x1234
    // start_bit=16, len=12 -> 0xFF0 (signed 12-bit = -16)
    const p_intel: [4]u8 = [0x34, 0x12, 0xF0, 0x0F];
    expect_eq(extractSignalIntel(p_intel[0..], 0, 16) ?? 0, 0x1234 as u64);
    const raw12 = extractSignalIntel(p_intel[0..], 16, 12) ?? unreachable;
    expect_eq(raw12, 0xFF0 as u64);
    expect_eq(signExtend(raw12, 12), -16 as i64);
    expect_eq(signExtend(0x7FF, 12), 2047 as i64);

    // Non-byte-aligned Intel signal: start_bit=4, len=8 across byte0[7..4] and byte1[3..0]
    // byte0=0x34 (hi nibble 0x3), byte1=0x12 (lo nibble 0x2) => 0x23
    expect_eq(extractSignalIntel(p_intel[0..], 4, 8) ?? 0, 0x23 as u64);

    // Motorola (big-endian sawtooth): bytes [0x12, 0x34, 0xAB, 0xCD]
    // start_bit=7 (byte0 MSB), len=16 -> 0x1234
    // start_bit=3 (byte0 bit 3), len=12 -> byte0[3..0]=0x2, byte1[7..0]=0x34 => 0x234
    const p_moto: [4]u8 = [0x12, 0x34, 0xAB, 0xCD];
    expect_eq(extractSignalMotorola(p_moto[0..], 7, 16) ?? 0, 0x1234 as u64);
    expect_eq(extractSignalMotorola(p_moto[0..], 3, 12) ?? 0, 0x234 as u64);
    expect_eq(extractSignalMotorola(p_moto[0..], 7, 32) ?? 0, 0x1234ABCD as u64);

    // 64-bit signExtend edge cases
    expect_eq(signExtend(0xFFFFFFFFFFFFFFFF, 64), -1 as i64);
    expect_eq(signExtend(0x8000000000000000, 64), (-9223372036854775807 as i64) - 1);
    expect_eq(signExtend(1, 1), -1 as i64);
});

test("5. Malformed frame lengths, buffer sizes, and out-of-bounds DBC bitfields rejected", () => {
    // Classic CAN len > 8 rejected on parse and encode
    let bad16: [16]u8 = [0; 16];
    bad16[4] = 9;
    expect_eq(parseSocketCan(bad16[0..]) == null, true);

    let short15: [15]u8 = [0; 15];
    expect_eq(parseSocketCan(short15[0..]) == null, true);

    const over_classic = CanFrame {
        can_id: CanId.standard(1),
        len: 12,
        flags: 0,
        is_fd: false,
        data: [0; 64],
    };
    expect_eq(encodeSocketCan(over_classic, bad16[0..]) == null, true);

    // CAN FD len > 64 rejected on parse and encode
    let bad72: [72]u8 = [0; 72];
    bad72[4] = 65;
    expect_eq(parseSocketCanFd(bad72[0..]) == null, true);
    expect_eq(parseSocketCanFd(bad16[0..]) == null, true);

    const over_fd = CanFrame {
        can_id: CanId.standard(1),
        len: 65,
        flags: 0,
        is_fd: true,
        data: [0; 64],
    };
    expect_eq(encodeSocketCanFd(over_fd, bad72[0..]) == null, true);

    // Out-of-bounds DBC bit_len (0, 65, or exceeding payload)
    const buf8: [8]u8 = [0; 8];
    expect_eq(extractSignalIntel(buf8[0..], 0, 0) == null, true);
    expect_eq(extractSignalIntel(buf8[0..], 0, 65) == null, true);
    expect_eq(extractSignalIntel(buf8[0..], 60, 8) == null, true);
    expect_eq(extractSignalMotorola(buf8[0..], 0, 0) == null, true);
    expect_eq(extractSignalMotorola(buf8[0..], 0, 65) == null, true);
    expect_eq(extractSignalMotorola(buf8[0..], 63, 16) == null, true);
});
