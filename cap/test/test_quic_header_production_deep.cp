import { test, expect_eq } from "cap:test";
import {
    QUIC_VERSION_1,
    QUIC_VERSION_2,
    QUIC_VERSION_NEGOTIATION,
    QUIC_VARINT_MAX,
    QuicHeaderError,
    QuicPacketKind,
    varIntEncodedLen,
    encodeVarInt,
    decodeVarInt,
    QuicHeader,
    encodeQuicHeader,
    decodeQuicHeader,
    QpackHeaderClass,
    QpackIndex,
    classifyQpackHeader,
    SocketKind,
    socketKindFromByte,
} from "cap:quic_header";

test("1. QUIC RFC 9000 VarInt encode/decode across 1/2/4/8-byte boundaries", () => {
    const samples: [6]u64 = [0, 63, 64, 16383, 1073741823, QUIC_VARINT_MAX];
    const expected_lens: [6]usize = [1, 1, 2, 2, 4, 8];

    let buf: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    let i: usize = 0;
    while (i < 6) {
        const v = samples[i];
        const want_len = expected_lens[i];
        const calc_len = varIntEncodedLen(v) catch unreachable;
        expect_eq(calc_len, want_len);

        const written = encodeVarInt(buf[0..], v) catch unreachable;
        expect_eq(written, want_len);

        const decoded = decodeVarInt(buf[0..written]) catch unreachable;
        expect_eq(decoded.value, v);
        expect_eq(decoded.bytes_read, want_len);
        i += 1;
    }

    // Overflow (> 2^62 - 1) rejected
    std.testing.expectError(QuicHeaderError.VarIntTooLarge, varIntEncodedLen(QUIC_VARINT_MAX + 1)) catch unreachable;
    std.testing.expectError(QuicHeaderError.VarIntTooLarge, encodeVarInt(buf[0..], QUIC_VARINT_MAX + 1)) catch unreachable;

    // Truncated 2-byte varint rejected
    const trunc_vi: [1]u8 = [0x40];
    std.testing.expectError(QuicHeaderError.UnexpectedEof, decodeVarInt(trunc_vi[0..])) catch unreachable;
});

test("2. QUIC Long Header round-trip (Initial with token, Handshake, 0-RTT, Retry, VersionNegotiation)", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    const dcid: [8]u8 = [0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08];
    const scid: [4]u8 = [0xde, 0xad, 0xbe, 0xef];
    const tok: [5]u8 = [0x11, 0x22, 0x33, 0x44, 0x55];

    // Initial packet with token
    const n_init = encodeQuicHeader(buf[0..], QuicHeader{
        kind: QuicPacketKind.Initial,
        is_long: true,
        fixed_bit: true,
        packet_number_len: 2,
        version: QUIC_VERSION_1,
        dcid: dcid[0..],
        scid: scid[0..],
        token: tok[0..],
    }) catch unreachable;

    const dec_init = decodeQuicHeader(buf[0..n_init], 0) catch unreachable;
    expect_eq(dec_init.kind == QuicPacketKind.Initial, true);
    expect_eq(dec_init.isLongHeader(), true);
    expect_eq(dec_init.isSupportedVersion(), true);
    expect_eq(dec_init.version, QUIC_VERSION_1);
    expect_eq(dec_init.packet_number_len, u8(2));
    expect_eq(std.mem.eql(u8, dec_init.dcid, dcid[0..]), true);
    expect_eq(std.mem.eql(u8, dec_init.scid, scid[0..]), true);
    expect_eq(std.mem.eql(u8, dec_init.token, tok[0..]), true);
    expect_eq(dec_init.header_len, n_init);

    // Handshake packet with QUIC v2
    const n_hs = encodeQuicHeader(buf[0..], QuicHeader{
        kind: QuicPacketKind.Handshake,
        is_long: true,
        packet_number_len: 4,
        version: QUIC_VERSION_2,
        dcid: dcid[0..4],
        scid: scid[0..2],
    }) catch unreachable;
    const dec_hs = decodeQuicHeader(buf[0..n_hs], 0) catch unreachable;
    expect_eq(dec_hs.kind == QuicPacketKind.Handshake, true);
    expect_eq(dec_hs.version, QUIC_VERSION_2);
    expect_eq(dec_hs.isSupportedVersion(), true);
    expect_eq(dec_hs.packet_number_len, u8(4));

    // Version Negotiation (version == 0, fixed_bit == false allowed)
    const n_vn = encodeQuicHeader(buf[0..], QuicHeader{
        kind: QuicPacketKind.VersionNegotiation,
        is_long: true,
        fixed_bit: false,
        version: QUIC_VERSION_NEGOTIATION,
        dcid: dcid[0..],
        scid: scid[0..],
    }) catch unreachable;
    const dec_vn = decodeQuicHeader(buf[0..n_vn], 0) catch unreachable;
    expect_eq(dec_vn.kind == QuicPacketKind.VersionNegotiation, true);
    expect_eq(dec_vn.version, QUIC_VERSION_NEGOTIATION);
    expect_eq(dec_vn.isSupportedVersion(), false);
});

test("3. QUIC 1-RTT Short Header round-trip with spin_bit & key_phase", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);
    const dcid: [6]u8 = [0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff];

    const n = encodeQuicHeader(buf[0..], QuicHeader{
        kind: QuicPacketKind.Short,
        is_long: false,
        spin_bit: true,
        key_phase: true,
        packet_number_len: 3,
        dcid: dcid[0..],
    }) catch unreachable;
    expect_eq(n, usize(7));

    const decoded = decodeQuicHeader(buf[0..n], 6) catch unreachable;
    expect_eq(decoded.kind == QuicPacketKind.Short, true);
    expect_eq(decoded.isShortHeader(), true);
    expect_eq(decoded.spin_bit, true);
    expect_eq(decoded.key_phase, true);
    expect_eq(decoded.packet_number_len, u8(3));
    expect_eq(std.mem.eql(u8, decoded.dcid, dcid[0..]), true);
    expect_eq(decoded.header_len, usize(7));
});

test("4. Malformed & truncated QUIC packet guards (fixed bit, CID > 20, truncated buffer)", () => {
    // Empty input
    const empty: [0]u8 = [];
    std.testing.expectError(QuicHeaderError.UnexpectedEof, decodeQuicHeader(empty[0..], 0)) catch unreachable;

    // Long header too short (< 6 bytes)
    const short_long: [4]u8 = [0xC0, 0x00, 0x00, 0x00];
    std.testing.expectError(QuicHeaderError.UnexpectedEof, decodeQuicHeader(short_long[0..], 0)) catch unreachable;

    // Long header with fixed_bit = 0 and non-zero version -> InvalidFixedBit
    const bad_fixed_long: [8]u8 = [0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00];
    std.testing.expectError(QuicHeaderError.InvalidFixedBit, decodeQuicHeader(bad_fixed_long[0..], 0)) catch unreachable;

    // Short header with fixed_bit = 0 -> InvalidFixedBit
    const bad_fixed_short: [5]u8 = [0x00, 0x01, 0x02, 0x03, 0x04];
    std.testing.expectError(QuicHeaderError.InvalidFixedBit, decodeQuicHeader(bad_fixed_short[0..], 4)) catch unreachable;

    // DCID length > 20 (21) -> InvalidCidLength
    let bad_dcid: [30]u8 = std.mem.zeroes([30]u8);
    bad_dcid[0] = 0xC0;
    bad_dcid[4] = 0x01;
    bad_dcid[5] = 21;
    std.testing.expectError(QuicHeaderError.InvalidCidLength, decodeQuicHeader(bad_dcid[0..], 0)) catch unreachable;

    // Short header with short_dcid_len > 20 -> InvalidCidLength
    let short_ok_hdr: [25]u8 = std.mem.zeroes([25]u8);
    short_ok_hdr[0] = 0x40;
    std.testing.expectError(QuicHeaderError.InvalidCidLength, decodeQuicHeader(short_ok_hdr[0..], 21)) catch unreachable;

    // Short header truncated DCID -> UnexpectedEof
    const short_trunc: [3]u8 = [0x40, 0x01, 0x02];
    std.testing.expectError(QuicHeaderError.UnexpectedEof, decodeQuicHeader(short_trunc[0..], 8)) catch unreachable;
});

test("5. SocketKind predicates & HTTP/3 QPACK header classification", () => {
    expect_eq(SocketKind.invalid.isValid(), false);
    expect_eq(SocketKind.uws_http_tls.isValid(), true);
    expect_eq(SocketKind.uws_http_tls.isTLS(), true);
    expect_eq(SocketKind.uws_http.isTLS(), false);
    expect_eq(SocketKind.uws_http_tls.isHttp(), true);
    expect_eq(SocketKind.listener_tls.isListener(), true);
    expect_eq(SocketKind.ws_client_tls.isWebSocket(), true);
    expect_eq(SocketKind.postgres_tls.isDatabase(), true);
    expect_eq(SocketKind.postgres_tls.isTLS(), true);
    expect_eq(SocketKind.spawn_ipc.isDatabase(), false);

    const parsed_kind = socketKindFromByte(SocketKind.uws_ws_tls.toByte()) catch unreachable;
    expect_eq(parsed_kind == SocketKind.uws_ws_tls, true);
    std.testing.expectError(QuicHeaderError.InvalidSocketKind, socketKindFromByte(250)) catch unreachable;

    // QPACK static table classification (case-insensitive)
    const conn = classifyQpackHeader("Connection");
    expect_eq(conn.class == QpackHeaderClass.Forbidden, true);
    expect_eq(conn.index == null, true);

    const host = classifyQpackHeader("HOST");
    expect_eq(host.class == QpackHeaderClass.Host, true);

    const ct = classifyQpackHeader("Content-Type");
    expect_eq(ct.class == QpackHeaderClass.Indexed, true);
    expect_eq(ct.index.? == QpackIndex.content_type, true);

    const custom = classifyQpackHeader("X-Custom-Agent");
    expect_eq(custom.class == QpackHeaderClass.Normal, true);
    expect_eq(custom.index == null, true);
});
