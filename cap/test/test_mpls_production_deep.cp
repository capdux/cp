// Source: cp/cap/test/test_mpls_production_deep.cp (Production deep tests for cap:mpls)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MPLS_ETHERTYPE_UNICAST,
    MPLS_ETHERTYPE_MULTICAST,
    MPLS_LSE_SIZE,
    MPLS_MAX_STACK_DEPTH,
    MPLS_MAX_LABEL,
    LABEL_IPV4_EXPLICIT_NULL,
    LABEL_ROUTER_ALERT,
    LABEL_IPV6_EXPLICIT_NULL,
    LABEL_IMPLICIT_NULL,
    LABEL_ENTROPY_INDICATOR,
    LABEL_GAL,
    LABEL_OAM_ALERT,
    LABEL_EXTENSION,
    MplsPayloadKind,
    MplsLabel,
    decodeLse,
    encodeLse,
    MplsLabelIter,
    peekPayloadVersion,
    MplsStack,
    parseMpls,
    encodeMplsStack,
    MplsGach,
    parseMplsGach,
    MplsPwCw,
    parseMplsPwCw,
} from "cap:mpls";

test("1. Single and multi-label MPLS stack encode/decode roundtrip and zero-copy iteration", () => {
    expect_eq(MPLS_ETHERTYPE_UNICAST, 0x8847 as u16);
    expect_eq(MPLS_ETHERTYPE_MULTICAST, 0x8848 as u16);
    expect_eq(MPLS_LSE_SIZE, 4);
    expect_eq(MPLS_MAX_STACK_DEPTH, 16);

    const labels_in: [3]MplsLabel = [
        MplsLabel{ .label = 1048575, .tc = 7, .bottom_of_stack = false, .ttl = 64 },
        MplsLabel{ .label = 20001, .tc = 3, .bottom_of_stack = false, .ttl = 128 },
        MplsLabel{ .label = 300, .tc = 0, .bottom_of_stack = true, .ttl = 255 },
    ];
    const ipv4_payload: [4]u8 = [0x45, 0x00, 0x00, 0x28];

    let wire: [64]u8 = std.mem.zeroes([64]u8);
    const total = encodeMplsStack(labels_in[0..], ipv4_payload[0..], wire[0..]) ?? unreachable;
    expect_eq(total, 12 + 4);

    const parsed = parseMpls(wire[0..total]) ?? unreachable;
    expect_eq(parsed.label_count, 3);
    expect_eq(parsed.header_len, 12);
    expect_eq(parsed.payload.len, 4);
    expect_eq(parsed.payloadKind(), MplsPayloadKind.ipv4);

    const top = parsed.topLabel() ?? unreachable;
    expect_eq(top.label, MPLS_MAX_LABEL);
    expect_eq(top.tc, 7 as u8);
    expect_eq(top.bottom_of_stack, false);
    expect_eq(top.ttl, 64 as u8);

    const bot = parsed.bottomLabel() ?? unreachable;
    expect_eq(bot.label, 300 as u32);
    expect_eq(bot.bottom_of_stack, true);
    expect_eq(bot.ttl, 255 as u8);

    let it = parsed.labels();
    expect_eq(it.remaining(), 3);
    const l0 = it.next() ?? unreachable;
    const l1 = it.next() ?? unreachable;
    const l2 = it.next() ?? unreachable;
    expect_eq(l0.encode(), encodeLse(labels_in[0]));
    expect_eq(l1.label, 20001 as u32);
    expect_eq(l1.tc, 3 as u8);
    expect_eq(l2.bottom_of_stack, true);
    expect_eq(it.next() == null, true);
});

test("2. RFC 3032/7274 special-purpose label classification (0..15 vs >= 16)", () => {
    const special_ids: [8]u32 = [
        LABEL_IPV4_EXPLICIT_NULL,
        LABEL_ROUTER_ALERT,
        LABEL_IPV6_EXPLICIT_NULL,
        LABEL_IMPLICIT_NULL,
        LABEL_ENTROPY_INDICATOR,
        LABEL_GAL,
        LABEL_OAM_ALERT,
        LABEL_EXTENSION,
    ];
    let i: usize = 0;
    while (i < 8) {
        const l = MplsLabel{ .label = special_ids[i], .tc = 0, .bottom_of_stack = true, .ttl = 1 };
        expect_eq(l.isSpecial(), true);
        const rt = decodeLse(l.encode());
        expect_eq(rt.label, special_ids[i]);
        expect_eq(rt.isSpecial(), true);
        i += 1;
    }

    const normal = MplsLabel{ .label = 16, .tc = 2, .bottom_of_stack = true, .ttl = 32 };
    expect_eq(normal.isSpecial(), false);
});

test("3. Entropy Label Indicator (7) and GAL (13) + RFC 5586 G-ACh decoding", () => {
    // Tunnel label (5000) -> ELI (7) -> Entropy Label (0x54321) -> GAL (13, BoS=1)
    const stack: [4]MplsLabel = [
        MplsLabel{ .label = 5000, .tc = 0, .bottom_of_stack = false, .ttl = 64 },
        MplsLabel{ .label = LABEL_ENTROPY_INDICATOR, .tc = 0, .bottom_of_stack = false, .ttl = 64 },
        MplsLabel{ .label = 0x54321, .tc = 0, .bottom_of_stack = false, .ttl = 0 },
        MplsLabel{ .label = LABEL_GAL, .tc = 6, .bottom_of_stack = true, .ttl = 1 },
    ];
    // RFC 5586 G-ACh: nibble=0x1, ver=0, res=0, channel_type=0x0022 (BFD) + "OAM"
    const gach_bytes: [7]u8 = [0x10, 0x00, 0x00, 0x22, 'O', 'A', 'M'];

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const n = encodeMplsStack(stack[0..], gach_bytes[0..], buf[0..]) ?? unreachable;
    const parsed = parseMpls(buf[0..n]) ?? unreachable;
    expect_eq(parsed.label_count, 4);
    expect_eq(parsed.payloadKind(), MplsPayloadKind.gach);

    const bot = parsed.bottomLabel() ?? unreachable;
    expect_eq(bot.label, LABEL_GAL);
    expect_eq(bot.isSpecial(), true);

    const gach = parseMplsGach(parsed.payload) ?? unreachable;
    expect_eq(gach.version, 0 as u8);
    expect_eq(gach.reserved, 0 as u8);
    expect_eq(gach.channel_type, 0x0022 as u16);
    expect_eq(std.mem.eql(u8, gach.payload, "OAM"), true);
});

test("4. Payload sniffing (IPv4, IPv6, G-ACh, PW-CW, unknown) and RFC 4385 PW-CW parsing", () => {
    const v4: [4]u8 = [0x45, 0x00, 0x00, 0x14];
    const v6: [4]u8 = [0x60, 0x00, 0x00, 0x00];
    const gach_b: [4]u8 = [0x10, 0x00, 0x00, 0x07];
    // PW-CW: nibble=0x0, flags=0x5, frg=2 (10b), len=12 (0x0C) -> byte1 = 0x8C, seq=0x0102
    const pwcw_b: [8]u8 = [0x05, 0x8C, 0x01, 0x02, 'E', 'T', 'H', '!'];
    const unk: [2]u8 = [0x90, 0x00];

    expect_eq(peekPayloadVersion(v4[0..]), MplsPayloadKind.ipv4);
    expect_eq(peekPayloadVersion(v6[0..]), MplsPayloadKind.ipv6);
    expect_eq(peekPayloadVersion(gach_b[0..]), MplsPayloadKind.gach);
    expect_eq(peekPayloadVersion(pwcw_b[0..]), MplsPayloadKind.pw_cw);
    expect_eq(peekPayloadVersion(unk[0..]), MplsPayloadKind.unknown);
    expect_eq(peekPayloadVersion(""), MplsPayloadKind.unknown);

    const cw = parseMplsPwCw(pwcw_b[0..]) ?? unreachable;
    expect_eq(cw.flags, 5 as u8);
    expect_eq(cw.frg, 2 as u8);
    expect_eq(cw.length, 12 as u8);
    expect_eq(cw.sequence_number, 0x0102 as u16);
    expect_eq(std.mem.eql(u8, cw.payload, "ETH!"), true);

    // Mismatched nibble rejected
    expect_eq(parseMplsPwCw(gach_b[0..]) == null, true);
    expect_eq(parseMplsGach(pwcw_b[0..]) == null, true);
});

test("5. Missing BoS, truncated stack, excessive stack depth (> 16), and short buffer resilience", () => {
    // 1. Truncated (< 4B)
    const short3: [3]u8 = [0x00, 0x01, 0x00];
    expect_eq(parseMpls(short3[0..]) == null, true);
    expect_eq(parseMplsGach(short3[0..]) == null, true);
    expect_eq(parseMplsPwCw(short3[0..]) == null, true);

    // 2. Missing BoS (S == 0 on all entries)
    const no_bos_lse = encodeLse(MplsLabel{ .label = 100, .tc = 0, .bottom_of_stack = false, .ttl = 64 });
    let no_bos: [8]u8 = std.mem.zeroes([8]u8);
    no_bos[0] = ((no_bos_lse >> 24) & 0xFF) as u8;
    no_bos[1] = ((no_bos_lse >> 16) & 0xFF) as u8;
    no_bos[2] = ((no_bos_lse >> 8) & 0xFF) as u8;
    no_bos[3] = (no_bos_lse & 0xFF) as u8;
    no_bos[4] = no_bos[0];
    no_bos[5] = no_bos[1];
    no_bos[6] = no_bos[2];
    no_bos[7] = no_bos[3];
    expect_eq(parseMpls(no_bos[0..]) == null, true);

    // 3. Stack deeper than MPLS_MAX_STACK_DEPTH (17 entries with S=0 for first 16, S=1 on 17th)
    let deep: [68]u8 = std.mem.zeroes([68]u8);
    deep[66] = 0x01; // S=1 at 17th entry (index 16, offset 64..68)
    expect_eq(parseMpls(deep[0..]) == null, true);

    // 4. Exact MPLS_MAX_STACK_DEPTH (16 entries with S=1 on 16th) succeeds
    let exact16: [64]u8 = std.mem.zeroes([64]u8);
    exact16[62] = 0x01; // S=1 at 16th entry (offset 60..64)
    const ok16 = parseMpls(exact16[0..]) ?? unreachable;
    expect_eq(ok16.label_count, 16);

    // 5. Encoder rejects empty stack, out-of-range 20-bit label, and short buffer
    const empty_labels: [0]MplsLabel = [];
    let out: [8]u8 = std.mem.zeroes([8]u8);
    expect_eq(encodeMplsStack(empty_labels[0..], "", out[0..]) == null, true);

    const bad_lbl: [1]MplsLabel = [MplsLabel{ .label = 0x0010_0000, .tc = 0, .bottom_of_stack = true, .ttl = 1 }];
    expect_eq(encodeMplsStack(bad_lbl[0..], "", out[0..]) == null, true);

    const good_lbl: [1]MplsLabel = [MplsLabel{ .label = 100, .tc = 0, .bottom_of_stack = true, .ttl = 64 }];
    expect_eq(encodeMplsStack(good_lbl[0..], "12345", out[0..]) == null, true); // needs 9 > 8
});
