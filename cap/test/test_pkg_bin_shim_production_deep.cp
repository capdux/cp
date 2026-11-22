import { test, expect } from "cap:test";
import {
    BIN_SHIM_VERSION_CURRENT,
    BIN_SHIM_VERSION_V1,
    BinLinkingShim,
    BinShimExtensionKind,
    BinShimFlags,
    BinShimShebang,
    classifyBinExtension,
    decodeShimUtf16LeToUtf8,
} from "cap:pkg";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("pkg bin_shim: script extension classifier and fallback shebangs", () => {
    expect(classifyBinExtension("esbuild/bin/cli.js") == BinShimExtensionKind.RunWithBun).toBe(true);
    expect(classifyBinExtension("pkg/index.mts") == BinShimExtensionKind.RunWithBun).toBe(true);
    expect(classifyBinExtension("pkg/run.sh") == BinShimExtensionKind.RunWithBun).toBe(true);
    expect(classifyBinExtension("pkg/tool.CMD") == BinShimExtensionKind.RunWithCmd).toBe(true);
    expect(classifyBinExtension("pkg/tool.bat") == BinShimExtensionKind.RunWithCmd).toBe(true);
    expect(classifyBinExtension("pkg/tool.ps1") == BinShimExtensionKind.RunWithPowerShell).toBe(true);
    expect(classifyBinExtension("pkg/native.exe") == null).toBe(true);
    expect(classifyBinExtension("pkg/.hidden") == null).toBe(true);

    if (sb := BinShimShebang.parseFromBinPath("my-cli/dist/index.js")) {
        expect(sliceEq(sb.launcher, "bun run")).toBe(true);
        expect(sb.is_node_or_bun).toBe(true);
        expect(sb.is_node).toBe(false);
    } else {
        expect(false).toBe(true);
    }

    if (sb_ps := BinShimShebang.parseFromBinPath("my-cli/run.ps1")) {
        expect(sliceEq(sb_ps.launcher, "powershell -ExecutionPolicy Bypass -File")).toBe(true);
        expect(sb_ps.is_node_or_bun).toBe(false);
    } else {
        expect(false).toBe(true);
    }
});

test("pkg bin_shim: shebang parsing for env node, env bun, CRLF, custom interpreter, and fallback", () => {
    // #!/usr/bin/env node -> is_node_or_bun=true, is_node=true
    if (sb_node := BinShimShebang.parse("#!/usr/bin/env node\nconsole.log(1);\n", "pkg/cli.js")) {
        expect(sliceEq(sb_node.launcher, "node")).toBe(true);
        expect(sb_node.is_node_or_bun).toBe(true);
        expect(sb_node.is_node).toBe(true);
    } else {
        expect(false).toBe(true);
    }

    // #!/usr/bin/env node --experimental-vm-modules with CRLF
    if (sb_node_args := BinShimShebang.parse("#!/usr/bin/env node --enable-source-maps\r\n", "pkg/cli.js")) {
        expect(sliceEq(sb_node_args.launcher, "node --enable-source-maps")).toBe(true);
        expect(sb_node_args.is_node_or_bun).toBe(true);
        expect(sb_node_args.is_node).toBe(true);
    } else {
        expect(false).toBe(true);
    }

    // #!/bin/env bun -> is_node_or_bun=true, is_node=false
    if (sb_bun := BinShimShebang.parse("#!/bin/env bun\n", "pkg/cli.ts")) {
        expect(sliceEq(sb_bun.launcher, "bun")).toBe(true);
        expect(sb_bun.is_node_or_bun).toBe(true);
        expect(sb_bun.is_node).toBe(false);
    } else {
        expect(false).toBe(true);
    }

    // Direct interpreter #!/bin/sh -> is_node_or_bun=false
    if (sb_sh := BinShimShebang.parse("#!/bin/sh\necho hi\n", "pkg/script")) {
        expect(sliceEq(sb_sh.launcher, "/bin/sh")).toBe(true);
        expect(sb_sh.is_node_or_bun).toBe(false);
        expect(sb_sh.is_node).toBe(false);
    } else {
        expect(false).toBe(true);
    }

    // Missing shebang falls back to extension
    if (sb_fb := BinShimShebang.parse("console.log('no shebang');", "pkg/cli.mjs")) {
        expect(sliceEq(sb_fb.launcher, "bun run")).toBe(true);
    } else {
        expect(false).toBe(true);
    }
});

test("pkg bin_shim: BinShimFlags 16-bit packing and version magic validation", () => {
    const f = BinShimFlags{
        is_node_or_bun: true,
        is_node: true,
        has_shebang: true,
        version_tag: BIN_SHIM_VERSION_CURRENT,
    };
    const raw = f.toU16();
    const back = BinShimFlags.fromU16(raw);
    expect(back.isValid()).toBe(true);
    expect(back.is_node_or_bun).toBe(true);
    expect(back.is_node).toBe(true);
    expect(back.has_shebang).toBe(true);
    expect(back.version_tag).toBe(BIN_SHIM_VERSION_CURRENT);

    const old_valid = BinShimFlags{ version_tag: BIN_SHIM_VERSION_V1 };
    expect(old_valid.isValid()).toBe(true);

    const bogus = BinShimFlags{ version_tag: 1234 };
    expect(bogus.isValid()).toBe(false);
});

test("pkg bin_shim: encodeInto and decode lossless round-trip with shebang", () => {
    const sb = BinShimShebang.parse("#!/usr/bin/env node\n", "cowsay/cli.js");
    const shim = BinLinkingShim{
        bin_path: "cowsay/cli.js",
        shebang: sb,
    };

    let wire: [256]u8 = [0; 256];
    const needed = shim.encodedLength();
    const written = shim.encodeInto(wire[0..]) catch 0;
    expect(written).toBe(needed);
    expect(written % 2).toBe(0);

    const decoded = BinLinkingShim.decode(wire[0..written]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(decoded.flags.isValid()).toBe(true);
    expect(decoded.flags.has_shebang).toBe(true);
    expect(decoded.flags.is_node_or_bun).toBe(true);
    expect(decoded.flags.is_node).toBe(true);

    let utf8_buf: [128]u8 = [0; 128];
    const pn = decodeShimUtf16LeToUtf8(decoded.bin_path_utf16_bytes, utf8_buf[0..]) catch 0;
    expect(sliceEq(utf8_buf[0..pn], "cowsay/cli.js")).toBe(true);

    const ln = decodeShimUtf16LeToUtf8(decoded.launcher_utf16_bytes, utf8_buf[0..]) catch 0;
    // Launcher trailer always ends with trailing space per Bun shim format
    expect(sliceEq(utf8_buf[0..ln], "node ")).toBe(true);
});

test("pkg bin_shim: encodeInto and decode lossless round-trip without shebang (native binary)", () => {
    const shim = BinLinkingShim{
        bin_path: "@esbuild/win32-x64/esbuild.exe",
        shebang: null,
    };

    let wire: [256]u8 = [0; 256];
    const written = shim.encodeInto(wire[0..]) catch 0;
    expect(written).toBe(shim.encodedLength());

    const decoded = BinLinkingShim.decode(wire[0..written]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(decoded.flags.has_shebang).toBe(false);
    expect(decoded.flags.is_node_or_bun).toBe(false);
    expect(decoded.launcher_utf16_bytes.len).toBe(0);

    let utf8_buf: [128]u8 = [0; 128];
    const pn = decodeShimUtf16LeToUtf8(decoded.bin_path_utf16_bytes, utf8_buf[0..]) catch 0;
    expect(sliceEq(utf8_buf[0..pn], "@esbuild/win32-x64/esbuild.exe")).toBe(true);
});

test("pkg bin_shim: error guards for leading slash, empty path, tiny buffer, and corrupt shims", () => {
    let wire: [128]u8 = [0; 128];

    // Leading slash rejected
    const bad_slash = BinLinkingShim{ bin_path: "/abs/bin.js", shebang: null };
    let slash_err = false;
    * = bad_slash.encodeInto(wire[0..]) catch {
        slash_err = true;
    };
    expect(slash_err).toBe(true);

    // Empty path rejected
    const bad_empty = BinLinkingShim{ bin_path: "", shebang: null };
    let empty_err = false;
    * = bad_empty.encodeInto(wire[0..]) catch {
        empty_err = true;
    };
    expect(empty_err).toBe(true);

    // BufferTooSmall rejected
    const ok_shim = BinLinkingShim{ bin_path: "pkg/bin.exe", shebang: null };
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = ok_shim.encodeInto(tiny[0..]) catch {
        buf_err = true;
    };
    expect(buf_err).toBe(true);

    // Corrupt magic version tag rejected
    const bad_magic: [12]u8 = [0; 12];
    let magic_err = false;
    * = BinLinkingShim.decode(bad_magic[0..]) catch {
        magic_err = true;
    };
    expect(magic_err).toBe(true);

    // Odd UTF-16 byte length rejected
    let out: [16]u8 = [0; 16];
    const odd_bytes: [3]u8 = ['a', 0, 'b'];
    let odd_err = false;
    * = decodeShimUtf16LeToUtf8(odd_bytes[0..], out[0..]) catch {
        odd_err = true;
    };
    expect(odd_err).toBe(true);
});
