// cap/test/test_elf_ffi_net_conf.cp
// End-to-End Regression Test for Mach-O/PE Inspectors (cap:elf), C-ABI Struct Layout (cap:ffi),
// HTTP CONNECT & SOCKS5 Proxy Codecs (cap:net), and ConfValueParser (cap:conf)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MachHeader64,
    MachoInspector,
    PeInspector
} from "cap:elf";
import {
    FFIType,
    CStructLayout
} from "cap:ffi";
import {
    HttpConnectTunnel,
    Socks5Codec
} from "cap:net";
import {
    ConfValueParser
} from "cap:conf";

fn main() {
    test("1. cap:elf — Mach-O 64-bit Header & PE/COFF Header Inspectors", () => {
        // 1. Mach-O 64-bit header roundtrip
        let mh_buf: [32]u8 = [0; 32];
        const expected_mh = MachHeader64 {
            magic: MachoInspector.magic64(),
            cputype: MachoInspector.cpuTypeArm64(),
            cpusubtype: 0,
            filetype: MachoInspector.fileTypeExecute(),
            ncmds: 16,
            sizeofcmds: 1408,
            flags: 0x00200085,
            reserved: 0,
        };
        expect_eq(MachoInspector.encodeHeader64(mh_buf[0..32], expected_mh), 32);

        const parsed_mh = MachoInspector.parseHeader64(mh_buf[0..32]).?;
        expect_eq(parsed_mh.magic, 0xFEEDFACF);
        expect_eq(parsed_mh.cputype, MachoInspector.cpuTypeArm64());
        expect_eq(parsed_mh.filetype, 2);
        expect_eq(parsed_mh.ncmds, 16);
        expect_eq(parsed_mh.sizeofcmds, 1408);

        // 2. PE/COFF header inspection: MZ at 0..2, e_lfanew = 64 at 60..64, "PE\0\0" at 64..68, COFF at 68..88
        let pe_buf: [96]u8 = [0; 96];
        pe_buf[0] = 'M';
        pe_buf[1] = 'Z';
        pe_buf[60] = 64; // e_lfanew = 64
        pe_buf[64] = 'P';
        pe_buf[65] = 'E';
        pe_buf[66] = 0;
        pe_buf[67] = 0;
        // Machine = 0x8664 (AMD64 LE: 0x64, 0x86)
        pe_buf[68] = 0x64;
        pe_buf[69] = 0x86;
        // NumberOfSections = 5
        pe_buf[70] = 5;
        pe_buf[71] = 0;
        // SizeOfOptionalHeader = 240 (0x00F0)
        pe_buf[84] = 0xF0;
        pe_buf[85] = 0x00;

        const parsed_pe = PeInspector.parseCoffHeader(pe_buf[0..96]).?;
        expect_eq(parsed_pe.e_lfanew, 64);
        expect_eq(parsed_pe.machine, PeInspector.machineAmd64());
        expect_eq(parsed_pe.num_sections, 5);
        expect_eq(parsed_pe.size_of_optional_header, 240);
    });

    test("2. cap:ffi — CStructLayout Natural Alignment, Padding & Finalized Stride", () => {
        // struct { u8 a; i64 b; u16 c; } -> offsets: a=0, b=8, c=16, total_size=24, max_align=8
        let layout = CStructLayout.init();
        const off_a = layout.addField(FFIType.U8);
        const off_b = layout.addField(FFIType.I64);
        const off_c = layout.addField(FFIType.U16);
        const total = layout.finalize();

        expect_eq(off_a, 0);
        expect_eq(off_b, 8);
        expect_eq(off_c, 16);
        expect_eq(layout.max_align, 8);
        expect_eq(total, 24);
        expect_eq(layout.finalized, true);
    });

    test("3. cap:net — HTTP CONNECT Proxy Tunnel & SOCKS5 Handshake Codec", () => {
        // 1. HTTP CONNECT request & response
        let req_buf: [128]u8 = [0; 128];
        const req_len = HttpConnectTunnel.buildConnectRequest(req_buf[0..128], "api.example.com", 443);
        const expected_req = "CONNECT api.example.com:443 HTTP/1.1\r\nHost: api.example.com:443\r\nProxy-Connection: keep-alive\r\n\r\n";
        expect_eq(std.mem.eql(u8, req_buf[0..req_len], expected_req), true);

        expect_eq(HttpConnectTunnel.isTunnelEstablished("HTTP/1.1 200 Connection Established\r\n\r\n"), true);
        expect_eq(HttpConnectTunnel.parseConnectResponse("HTTP/1.1 407 Proxy Authentication Required\r\n\r\n").?, 407);

        // 2. SOCKS5 Greeting & Connect frames
        let s5_buf: [32]u8 = [0; 32];
        expect_eq(Socks5Codec.encodeGreetingNoAuth(s5_buf[0..32]), 3);
        expect_eq(s5_buf[0], 0x05);
        expect_eq(s5_buf[1], 0x01);
        expect_eq(s5_buf[2], 0x00);
        expect_eq(Socks5Codec.verifyGreetingReply("\x05\x00"), true);

        const dom_len = Socks5Codec.encodeConnectDomain(s5_buf[0..32], "cp.dev", 8080);
        expect_eq(dom_len, 13);
        expect_eq(s5_buf[3], 0x03); // ATYP = Domain
        expect_eq(s5_buf[4], 6);    // len("cp.dev")
        expect_eq(s5_buf[11], 0x1F); // 8080 >> 8
        expect_eq(s5_buf[12], 0x90); // 8080 & 0xFF
    });

    test("4. cap:conf — ConfValueParser Booleans, Ports, Durations & Byte Sizes", () => {
        expect_eq(ConfValueParser.parseBool("true").?, true);
        expect_eq(ConfValueParser.parseBool("YES").?, true);
        expect_eq(ConfValueParser.parseBool("on").?, true);
        expect_eq(ConfValueParser.parseBool("false").?, false);
        expect_eq(ConfValueParser.parseBool("0").?, false);
        expect_eq(ConfValueParser.parseBool("invalid") == null, true);

        expect_eq(ConfValueParser.parsePort("8080").?, 8080);
        expect_eq(ConfValueParser.parsePort("0") == null, true);
        expect_eq(ConfValueParser.parsePort("70000") == null, true);

        expect_eq(ConfValueParser.parseDurationMs("250ms").?, 250);
        expect_eq(ConfValueParser.parseDurationMs("15s").?, 15000);
        expect_eq(ConfValueParser.parseDurationMs("2m").?, 120000);
        expect_eq(ConfValueParser.parseDurationMs("1h").?, 3600000);

        expect_eq(ConfValueParser.parseByteSize("512b").?, 512);
        expect_eq(ConfValueParser.parseByteSize("64kb").?, 65536);
        expect_eq(ConfValueParser.parseByteSize("16mb").?, 16777216);
    });
}
