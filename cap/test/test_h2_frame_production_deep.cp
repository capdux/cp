import { test, expect } from "cap:test";
import {
    CLIENT_PREFACE,
    FRAME_HEADER_SIZE,
    SETTINGS_UNIT_SIZE,
    STREAM_PRIORITY_SIZE,
    FLAG_ACK,
    FLAG_END_STREAM,
    FLAG_END_HEADERS,
    FLAG_PRIORITY,
    FrameType,
    ErrorCode,
    SettingsType,
    UInt31WithReserved,
    FrameHeader,
    StreamPriority,
    SettingsUnit,
    hasClientPreface,
} from "cap:h2_frame";

test("h2_frame: client preface and enum classifications", () => {
    expect(CLIENT_PREFACE.len).toBe(24);
    expect(hasClientPreface(CLIENT_PREFACE)).toBeTrue();
    expect(hasClientPreface("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\nEXTRA")).toBeTrue();
    expect(hasClientPreface("GET / HTTP/1.1\r\n\r\n")).toBeFalse();

    expect(FrameType.SETTINGS.isKnown()).toBeTrue();
    expect(FrameType.SETTINGS.isConnectionLevel()).toBeTrue();
    expect(FrameType.HEADERS.isConnectionLevel()).toBeFalse();
    expect(ErrorCode.NO_ERROR.isOk()).toBeTrue();
    expect(ErrorCode.PROTOCOL_ERROR.isOk()).toBeFalse();
});

test("h2_frame: FrameHeader 9-byte big-endian roundtrip", () => {
    let buf: [9]u8 = undefined;
    const hdr = FrameHeader{
        length: 0x010203,
        frame_type: .HEADERS,
        flags: FLAG_END_STREAM | FLAG_END_HEADERS | FLAG_PRIORITY,
        stream_id: 1337,
        reserved: true,
    };
    const n = hdr.encode(buf[0..]) catch unreachable;
    expect(n).toBe(FRAME_HEADER_SIZE);
    expect(buf[0]).toBe(0x01);
    expect(buf[1]).toBe(0x02);
    expect(buf[2]).toBe(0x03);
    expect(buf[3]).toBe(0x01);
    expect(buf[5] & 0x80).toBe(0x80);

    const decoded = FrameHeader.decode(buf[0..]) catch unreachable;
    expect(decoded.length).toBe(0x010203);
    expect(decoded.frame_type == .HEADERS).toBeTrue();
    expect(decoded.isEndStream()).toBeTrue();
    expect(decoded.isEndHeaders()).toBeTrue();
    expect(decoded.hasFlag(FLAG_PRIORITY)).toBeTrue();
    expect(decoded.stream_id).toBe(1337);
    expect(decoded.reserved).toBeTrue();
});

test("h2_frame: SettingsUnit 6-byte and StreamPriority 5-byte roundtrip", () => {
    let sbuf: [6]u8 = undefined;
    const unit = SettingsUnit{
        setting: .MAX_CONCURRENT_STREAMS,
        value: 256,
    };
    expect(unit.encode(sbuf[0..]) catch unreachable).toBe(SETTINGS_UNIT_SIZE);
    const sunit = SettingsUnit.decode(sbuf[0..]) catch unreachable;
    expect(sunit.setting == .MAX_CONCURRENT_STREAMS).toBeTrue();
    expect(sunit.value).toBe(256);

    let pbuf: [5]u8 = undefined;
    const prio = StreamPriority{
        exclusive: true,
        stream_dependency: 42,
        weight: 200,
    };
    expect(prio.encode(pbuf[0..]) catch unreachable).toBe(STREAM_PRIORITY_SIZE);
    const sprio = StreamPriority.decode(pbuf[0..]) catch unreachable;
    expect(sprio.exclusive).toBeTrue();
    expect(sprio.stream_dependency).toBe(42);
    expect(sprio.weight).toBe(200);
});

test("h2_frame: short buffers and UInt31WithReserved boundary checks", () => {
    const short4: [4]u8 = [0x80, 0x00, 0x00, 0x05];
    const u31 = UInt31WithReserved.decodeBe(short4[0..]) catch unreachable;
    expect(u31.reserved).toBeTrue();
    expect(u31.value).toBe(5);
    expect(u31.toU32()).toBe(0x80000005);

    const tiny: [3]u8 = [1, 2, 3];
    const r_hdr = FrameHeader.decode(tiny[0..]) catch null;
    expect(r_hdr == null).toBeTrue();
    const r_set = SettingsUnit.decode(tiny[0..]) catch null;
    expect(r_set == null).toBeTrue();
    const r_pri = StreamPriority.decode(tiny[0..]) catch null;
    expect(r_pri == null).toBeTrue();
});
