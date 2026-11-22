// Production Deep Tests for cap:bundler entry_points (lib/bundler/entry_points.cp)
import { test, expect_eq } from "cap:test";
import {
    EntryPointError,
    EntryPointNamespace,
    MacroEntryId,
    isEntryPointPath,
    encodeEntryPointPath,
    decodeEntryPointPath,
    generateFallbackEntryCode,
    generateClientEntryCode,
    generateServerEntryCode,
    generateMacroEntryId,
} from "cap:bundler";

test("bundler entry_points: virtual entry path encode/decode roundtrip & predicate", fn() {
    let enc_buf: [128]u8 = undefined;
    const encoded = encodeEntryPointPath("/app/pages", "home", ".tsx", enc_buf[0..]) catch unreachable;
    expect_eq(encoded, "/app/pages/home.entry.tsx");
    expect_eq(isEntryPointPath(encoded), true);
    expect_eq(isEntryPointPath("entry.tsx"), true);
    expect_eq(isEntryPointPath("/app/pages/home.tsx"), false);

    let dec_buf: [128]u8 = undefined;
    const decoded = decodeEntryPointPath(encoded, dec_buf[0..]) catch unreachable;
    expect_eq(decoded, "/app/pages/home.tsx");

    // Trailing slash in dir and missing leading dot in ext are normalized
    const encoded2 = encodeEntryPointPath("/app/pages/", "about", "jsx", enc_buf[0..]) catch unreachable;
    expect_eq(encoded2, "/app/pages/about.entry.jsx");
    const decoded2 = decodeEntryPointPath(encoded2, dec_buf[0..]) catch unreachable;
    expect_eq(decoded2, "/app/pages/about.jsx");

    // Non-entry path passes through unchanged on decode
    const passthrough = decodeEntryPointPath("/app/plain.ts", dec_buf[0..]) catch unreachable;
    expect_eq(passthrough, "/app/plain.ts");
});

test("bundler entry_points: FallbackEntryPoint & ClientEntryPoint code generation", fn() {
    let buf: [512]u8 = undefined;

    const fb1 = generateFallbackEntryCode("framework/boot", false, buf[0..]) catch unreachable;
    expect_eq(fb1, "import boot from 'framework/boot';\nboot(globalThis.__BUN_DATA__);\n");

    const fb2 = generateFallbackEntryCode("framework/boot", true, buf[0..]) catch unreachable;
    expect_eq(
        fb2,
        "globalThis.Bun_disableCSSImports = true;\nimport boot from 'framework/boot';\nboot(globalThis.__BUN_DATA__);\n",
    );

    const cl1 = generateClientEntryCode("react-boot", "/src/App.tsx", true, buf[0..]) catch unreachable;
    expect_eq(
        cl1,
        "globalThis.Bun_disableCSSImports = true;\nimport boot from 'react-boot';\nimport * as EntryPoint from '/src/App.tsx';\nboot(EntryPoint);\n",
    );

    const cl2 = generateClientEntryCode("react-boot", "/src/App.tsx", false, buf[0..]) catch unreachable;
    expect_eq(
        cl2,
        "import boot from 'react-boot';\nif ('setLoaded' in boot) boot.setLoaded(loaded);\nimport * as EntryPoint from '/src/App.tsx';\nboot(EntryPoint);\n",
    );
});

test("bundler entry_points: ServerEntryPoint normal vs HMR generation & JS quote escaping", fn() {
    let buf: [1024]u8 = undefined;

    // Normal mode escapes double quotes and backslashes safely
    const srv_normal = generateServerEntryCode("/app/my\"srv\\index.ts", false, buf[0..]) catch unreachable;
    expect_eq(std.mem.startsWith(u8, srv_normal, "// @bun\nimport * as start from \"/app/my\\\"srv\\\\index.ts\";\n"), true);
    expect_eq(std.mem.indexOf(u8, srv_normal, "Bun.serve(entryNamespace.default);") != null, true);

    // HMR mode escapes single quotes and emits reload symbol
    const srv_hmr = generateServerEntryCode("/app/it's/server.ts", true, buf[0..]) catch unreachable;
    expect_eq(std.mem.startsWith(u8, srv_hmr, "// @bun\nimport * as start from '/app/it\\'s/server.ts';\n"), true);
    expect_eq(std.mem.indexOf(u8, srv_hmr, "Symbol(\"BunServerHMR\")") != null, true);
    expect_eq(std.mem.indexOf(u8, srv_hmr, "server.reload(entryNamespace.default)") != null, true);
});

test("bundler entry_points: deterministic MacroEntryPoint ID & specifier formatting", fn() {
    let spec1: [64]u8 = undefined;
    let spec2: [64]u8 = undefined;
    let spec3: [64]u8 = undefined;

    const m1 = generateMacroEntryId("/src/macros.ts", "buildTimestamp", spec1[0..]) catch unreachable;
    const m2 = generateMacroEntryId("/src/macros.ts", "buildTimestamp", spec2[0..]) catch unreachable;
    const m3 = generateMacroEntryId("/src/macros.ts", "otherMacro", spec3[0..]) catch unreachable;

    // Deterministic across calls
    expect_eq(m1.hash, m2.hash);
    expect_eq(m1.id, m2.id);
    expect_eq(m1.specifier_len, 27); // "macro://" (8) + 16 hex + ".js" (3)
    expect_eq(spec1[0..m1.specifier_len], spec2[0..m2.specifier_len]);
    expect_eq(std.mem.startsWith(u8, spec1[0..m1.specifier_len], "macro://"), true);
    expect_eq(std.mem.endsWith(u8, spec1[0..m1.specifier_len], ".js"), true);

    // Different function name yields distinct hash & specifier
    expect_eq(m1.hash != m3.hash, true);
});

test("bundler entry_points: EntryPointNamespace labels & empty input rejection", fn() {
    expect_eq(EntryPointNamespace.fallback_entry.asStr(), "fallback-entry");
    expect_eq(EntryPointNamespace.client_entry.asStr(), "client-entry");
    expect_eq(EntryPointNamespace.server_entry.asStr(), "server-entry");
    expect_eq(EntryPointNamespace.macro_entry.asStr(), "macro");

    let buf: [64]u8 = undefined;

    let bad_base = false;
    * = encodeEntryPointPath("/src", "", ".ts", buf[0..]) catch { bad_base = true; };
    expect_eq(bad_base, true);

    let bad_dec = false;
    * = decodeEntryPointPath("", buf[0..]) catch { bad_dec = true; };
    expect_eq(bad_dec, true);

    let bad_fb = false;
    * = generateFallbackEntryCode("", false, buf[0..]) catch { bad_fb = true; };
    expect_eq(bad_fb, true);

    let bad_macro = false;
    * = generateMacroEntryId("/src/m.ts", "", buf[0..]) catch { bad_macro = true; };
    expect_eq(bad_macro, true);
});

test("bundler entry_points: BufferTooSmall guard on all generators", fn() {
    let tiny: [8]u8 = undefined;

    let oob1 = false;
    * = encodeEntryPointPath("/src/pages", "index", ".tsx", tiny[0..]) catch { oob1 = true; };
    expect_eq(oob1, true);

    let oob2 = false;
    * = generateClientEntryCode("boot", "/src/app.tsx", false, tiny[0..]) catch { oob2 = true; };
    expect_eq(oob2, true);

    let oob3 = false;
    * = generateServerEntryCode("/src/server.ts", true, tiny[0..]) catch { oob3 = true; };
    expect_eq(oob3, true);

    let oob4 = false;
    * = generateMacroEntryId("/src/m.ts", "fn", tiny[0..]) catch { oob4 = true; };
    expect_eq(oob4, true);
});
