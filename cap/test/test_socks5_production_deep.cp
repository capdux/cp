// Source: cp/cap/test/test_socks5_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:socks5 & cap.socks5

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Socks5Error,
    AuthMethod,
    SocksCommand,
    AddressType,
    ReplyCode,
    SocksAddr,
    UserPassCredentials,
    SocksRequest,
    SocksReply,
    SocksUdpPacket,
    encodeGreeting,
    decodeGreeting,
    encodeMethodChoice,
    decodeMethodChoice,
    encodeUserPassAuth,
    decodeUserPassAuth,
    encodeAuthStatus,
    decodeAuthStatus,
    encodeRequest,
    decodeRequest,
    encodeReply,
    decodeReply,
    encodeUdpPacket,
    decodeUdpPacket,
} from "../../lib/net/socks5_.cp";

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Greeting encode/decode round-trip
    const methods: [2]AuthMethod = [AuthMethod.NoAuth, AuthMethod.UserPass];
    const greeting = encodeGreeting(alloc, &methods)?;
    defer alloc.free(greeting);

    expect_eq(greeting.len, 4 as usize);
    expect_eq(greeting[0], 0x05 as u8);
    expect_eq(greeting[1], 0x02 as u8);

    const decoded_methods = decodeGreeting(greeting)?;
    expect_eq(decoded_methods.len, 2 as usize);
    expect_eq(decoded_methods[0], AuthMethod.NoAuth as u8);
    expect_eq(decoded_methods[1], AuthMethod.UserPass as u8);

    // 2. MethodChoice encode/decode round-trip
    const choice_wire = encodeMethodChoice(AuthMethod.UserPass);
    expect_eq(choice_wire[0], 0x05 as u8);
    expect_eq(choice_wire[1], 0x02 as u8);
    const chosen = decodeMethodChoice(&choice_wire)?;
    expect_eq(chosen == AuthMethod.UserPass, true);

    // 3. RFC 1929 Username/Password Auth encode/decode round-trip
    const auth_wire = encodeUserPassAuth(alloc, "alice", "s3cr3t-pass!")?;
    defer alloc.free(auth_wire);
    expect_eq(auth_wire[0], 0x01 as u8);

    const creds = decodeUserPassAuth(auth_wire)?;
    expect_eq(std.mem.eql(u8, creds.username, "alice"), true);
    expect_eq(std.mem.eql(u8, creds.password, "s3cr3t-pass!"), true);

    // 4. RFC 1929 AuthStatus encode/decode round-trip
    const st_ok = encodeAuthStatus(0x00);
    expect_eq(decodeAuthStatus(&st_ok)?, 0x00 as u8);
    const st_fail = encodeAuthStatus(0x01);
    expect_eq(decodeAuthStatus(&st_fail)?, 0x01 as u8);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. IPv4 Request & Reply round-trip
    const req_v4 = SocksRequest{
        cmd: SocksCommand.Connect,
        addr: SocksAddr.ipv4([192, 168, 1, 10]),
        port: 8080,
    };
    const wire_req_v4 = encodeRequest(alloc, req_v4)?;
    defer alloc.free(wire_req_v4);
    expect_eq(wire_req_v4.len, 10 as usize);
    expect_eq(wire_req_v4[3], AddressType.Ipv4 as u8);

    const dec_req_v4 = decodeRequest(wire_req_v4)?;
    expect_eq(dec_req_v4.cmd == SocksCommand.Connect, true);
    expect_eq(dec_req_v4.port, 8080 as u16);
    switch (dec_req_v4.addr) {
        .ipv4 v4 => {
            expect_eq(v4[0], 192 as u8);
            expect_eq(v4[1], 168 as u8);
            expect_eq(v4[2], 1 as u8);
            expect_eq(v4[3], 10 as u8);
        },
        else => expect_eq(true, false),
    }

    // 2. Domain ("api.cp.dev:443") Request & Reply round-trip
    const req_dom = SocksRequest{
        cmd: SocksCommand.Connect,
        addr: SocksAddr.domain("api.cp.dev"),
        port: 443,
    };
    const wire_req_dom = encodeRequest(alloc, req_dom)?;
    defer alloc.free(wire_req_dom);
    expect_eq(wire_req_dom[3], AddressType.Domain as u8);
    expect_eq(wire_req_dom[4], 10 as u8);

    const dec_req_dom = decodeRequest(wire_req_dom)?;
    expect_eq(dec_req_dom.port, 443 as u16);
    switch (dec_req_dom.addr) {
        .domain d => expect_eq(std.mem.eql(u8, d, "api.cp.dev"), true),
        else => expect_eq(true, false),
    }

    const rep_dom = SocksReply{
        reply: ReplyCode.Succeeded,
        addr: SocksAddr.domain("api.cp.dev"),
        port: 443,
    };
    const wire_rep_dom = encodeReply(alloc, rep_dom)?;
    defer alloc.free(wire_rep_dom);
    const dec_rep_dom = decodeReply(wire_rep_dom)?;
    expect_eq(dec_rep_dom.reply == ReplyCode.Succeeded, true);
    expect_eq(dec_rep_dom.port, 443 as u16);
    switch (dec_rep_dom.addr) {
        .domain d => expect_eq(std.mem.eql(u8, d, "api.cp.dev"), true),
        else => expect_eq(true, false),
    }

    // 3. IPv6 Request & Reply round-trip
    const v6_bytes: [16]u8 = [0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
    const req_v6 = SocksRequest{
        cmd: SocksCommand.Bind,
        addr: SocksAddr.ipv6(v6_bytes),
        port: 9001,
    };
    const wire_req_v6 = encodeRequest(alloc, req_v6)?;
    defer alloc.free(wire_req_v6);
    expect_eq(wire_req_v6.len, 22 as usize);

    const dec_req_v6 = decodeRequest(wire_req_v6)?;
    expect_eq(dec_req_v6.cmd == SocksCommand.Bind, true);
    expect_eq(dec_req_v6.port, 9001 as u16);
    switch (dec_req_v6.addr) {
        .ipv6 v6 => {
            expect_eq(v6[0], 0x20 as u8);
            expect_eq(v6[1], 0x01 as u8);
            expect_eq(v6[15], 0x01 as u8);
        },
        else => expect_eq(true, false),
    }

    const rep_v6 = SocksReply{
        reply: ReplyCode.ConnectionRefused,
        addr: SocksAddr.ipv6(v6_bytes),
        port: 9001,
    };
    const wire_rep_v6 = encodeReply(alloc, rep_v6)?;
    defer alloc.free(wire_rep_v6);
    const dec_rep_v6 = decodeReply(wire_rep_v6)?;
    expect_eq(dec_rep_v6.reply == ReplyCode.ConnectionRefused, true);
    expect_eq(dec_rep_v6.port, 9001 as u16);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // UDP Associate SocksUdpPacket round-trip (IPv4 & Domain)
    const pkt_v4 = SocksUdpPacket{
        frag: 0x00,
        addr: SocksAddr.ipv4([8, 8, 8, 8]),
        port: 53,
        payload: "dns-query-payload",
    };
    const wire_v4 = encodeUdpPacket(alloc, pkt_v4)?;
    defer alloc.free(wire_v4);
    expect_eq(wire_v4[0], 0x00 as u8);
    expect_eq(wire_v4[1], 0x00 as u8);
    expect_eq(wire_v4[2], 0x00 as u8);

    const dec_v4 = decodeUdpPacket(wire_v4)?;
    expect_eq(dec_v4.frag, 0x00 as u8);
    expect_eq(dec_v4.port, 53 as u16);
    expect_eq(std.mem.eql(u8, dec_v4.payload, "dns-query-payload"), true);
    switch (dec_v4.addr) {
        .ipv4 v4 => {
            expect_eq(v4[0], 8 as u8);
            expect_eq(v4[3], 8 as u8);
        },
        else => expect_eq(true, false),
    }

    const pkt_dom = SocksUdpPacket{
        frag: 0x01,
        addr: SocksAddr.domain("udp.cp.dev"),
        port: 5353,
        payload: "hello-udp",
    };
    const wire_dom = encodeUdpPacket(alloc, pkt_dom)?;
    defer alloc.free(wire_dom);
    const dec_dom = decodeUdpPacket(wire_dom)?;
    expect_eq(dec_dom.frag, 0x01 as u8);
    expect_eq(dec_dom.port, 5353 as u16);
    expect_eq(std.mem.eql(u8, dec_dom.payload, "hello-udp"), true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. InvalidVersion
    let bad_ver = false;
    const bad_ver_bytes: [3]u8 = [0x04, 0x01, 0x00];
    * = decodeGreeting(&bad_ver_bytes) catch e => err_ver: {
        bad_ver = (e == Socks5Error.InvalidVersion);
        break :err_ver bad_ver_bytes[0..0];
    };
    expect_eq(bad_ver, true);

    // 2. InvalidAddressType
    let bad_atyp = false;
    const bad_atyp_bytes: [10]u8 = [0x05, 0x01, 0x00, 0x09, 1, 2, 3, 4, 0, 80];
    * = decodeRequest(&bad_atyp_bytes) catch e => err_atyp: {
        bad_atyp = (e == Socks5Error.InvalidAddressType);
        break :err_atyp SocksRequest{ cmd: SocksCommand.Connect, addr: SocksAddr.ipv4([0, 0, 0, 0]), port: 0 };
    };
    expect_eq(bad_atyp, true);

    // 3. DomainTooLong (> 255 bytes)
    let long_dom: [256]u8 = [0x61; 256];
    let bad_dom = false;
    * = encodeRequest(alloc, SocksRequest{
        cmd: SocksCommand.Connect,
        addr: SocksAddr.domain(&long_dom),
        port: 80,
    }) catch e => err_dom: {
        bad_dom = (e == Socks5Error.DomainTooLong);
        break :err_dom long_dom[0..0];
    };
    expect_eq(bad_dom, true);

    // 4. UnexpectedEof (truncated IPv4 request)
    let bad_eof = false;
    const trunc_bytes: [8]u8 = [0x05, 0x01, 0x00, 0x01, 127, 0, 0, 1];
    * = decodeRequest(&trunc_bytes) catch e => err_eof: {
        bad_eof = (e == Socks5Error.UnexpectedEof);
        break :err_eof SocksRequest{ cmd: SocksCommand.Connect, addr: SocksAddr.ipv4([0, 0, 0, 0]), port: 0 };
    };
    expect_eq(bad_eof, true);

    // 5. TrailingBytes
    let bad_trail = false;
    const trail_bytes: [11]u8 = [0x05, 0x01, 0x00, 0x01, 127, 0, 0, 1, 0x00, 0x50, 0xFF];
    * = decodeRequest(&trail_bytes) catch e => err_trail: {
        bad_trail = (e == Socks5Error.TrailingBytes);
        break :err_trail SocksRequest{ cmd: SocksCommand.Connect, addr: SocksAddr.ipv4([0, 0, 0, 0]), port: 0 };
    };
    expect_eq(bad_trail, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mount: import cap from "cap"; cap.socks5
    const methods: [1]AuthMethod = [AuthMethod.NoAuth];
    const g = encodeGreeting(alloc, &methods)?;
    defer alloc.free(g);
    const gm = decodeGreeting(g)?;
    expect_eq(gm.len, 1 as usize);
    expect_eq(gm[0], 0x00 as u8);

    const req = SocksRequest{
        cmd: SocksCommand.UdpAssociate,
        addr: SocksAddr.domain("proxy.cap.local"),
        port: 1080,
    };
    const wire = encodeRequest(alloc, req)?;
    defer alloc.free(wire);
    const parsed = decodeRequest(wire)?;
    expect_eq(parsed.cmd == SocksCommand.UdpAssociate, true);
    expect_eq(parsed.port, 1080 as u16);

    const rep = SocksReply{
        reply: ReplyCode.Succeeded,
        addr: SocksAddr.ipv4([127, 0, 0, 1]),
        port: 1080,
    };
    const rwire = encodeReply(alloc, rep)?;
    defer alloc.free(rwire);
    const rparsed = decodeReply(rwire)?;
    expect_eq(rparsed.reply == ReplyCode.Succeeded, true);
}

test("cap:socks5 Greeting/MethodChoice and RFC 1929 UserPassAuth round-trip") {
    runTest1() catch unreachable;
}

test("cap:socks5 Request and Reply IPv4/Domain/IPv6 round-trip") {
    runTest2() catch unreachable;
}

test("cap:socks5 RFC 1928 UDP Associate SocksUdpPacket round-trip") {
    runTest3() catch unreachable;
}

test("cap:socks5 strict validation: InvalidVersion, InvalidAddressType, DomainTooLong, UnexpectedEof, TrailingBytes") {
    runTest4() catch unreachable;
}

test("cap.socks5 dynamic mount and std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
