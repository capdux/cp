// cap/test/test_resolver_bundler_deep.cp
// ============================================================================
// Deep Regression Suite for cap:resolver & cap:bundler (Batch 4 - Subagent 4A)
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { tmpPath, writeFileSync, mkdir, remove, rmdir } from "cap:fs";

import {
    DataURL,
    ModuleType,
    ExportResolveStatus,
    PackageJson,
    TsConfigJson,
    ResolveKind,
    ResolvedKind,
    ResolveConditions,
    StatHash,
    ModuleResolver,
} from "cap:resolver";

import {
    BundleTarget,
    OutputFormat,
    ChunkKind,
    ChunkOrder,
    ChunkEntry,
    BundleConfig,
    BundleV2,
    INVALID_MODULE_INDEX,
    ImportKind,
    ImportRecord,
    ModuleGraph,
    TargetOs,
    TargetArch,
    TargetAbi,
    CompileTarget,
    STANDALONE_HEADER_MAGIC,
    STANDALONE_TRAILER_MAGIC,
    StandaloneModuleFormat,
    StandaloneFileSide,
    StandaloneEncoding,
    StandaloneModuleEntry,
    StandaloneModuleGraph,
} from "cap:bundler";

// ============================================================================
// 1. cap:resolver — RFC 2397 DataURL
// ============================================================================
fn testDataUrl(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // Percent-encoded with charset
    const du1_opt = try DataURL.parse("data:text/plain;charset=utf-8,Hello%20CP%21");
    expect_eq(du1_opt != null, true);
    const du1 = du1_opt.?;
    expect_eq(std.mem.eql(u8, du1.mime_type, "text/plain"), true);
    expect_eq(std.mem.eql(u8, du1.charset, "utf-8"), true);
    expect_eq(du1.is_base64, false);
    const dec1 = try du1.decodeAlloc(alloc);
    expect_eq(std.mem.eql(u8, dec1, "Hello CP!"), true);

    // Base64 encoded JSON
    const du2_opt = try DataURL.parse("DATA:application/json;charset=utf-8;base64,eyJhIjoxfQ==");
    expect_eq(du2_opt != null, true);
    const du2 = du2_opt.?;
    expect_eq(std.mem.eql(u8, du2.mime_type, "application/json"), true);
    expect_eq(std.mem.eql(u8, du2.charset, "utf-8"), true);
    expect_eq(du2.is_base64, true);
    const dec2 = try du2.decodeAlloc(alloc);
    expect_eq(std.mem.eql(u8, dec2, "{\"a\":1}"), true);

    // Default mediatype & charset
    const du3 = (try DataURL.parse("data:,raw%2Bdata")).?;
    expect_eq(std.mem.eql(u8, du3.mime_type, "text/plain"), true);
    expect_eq(std.mem.eql(u8, du3.effectiveCharset(), "US-ASCII"), true);
    const dec3 = try du3.decodeAlloc(alloc);
    expect_eq(std.mem.eql(u8, dec3, "raw+data"), true);

    // Non-data URL returns null; malformed data URL without comma errors
    expect_eq((try DataURL.parse("https://cp-lang.org")) == null, true);
    let saw_err = false;
    * = DataURL.parse("data:text/plain-no-comma") catch {
        saw_err = true;
    };
    expect_eq(saw_err, true);

    // Base64 encode roundtrip
    const encoded = try DataURL.encodeBase64Alloc(alloc, "text/javascript", "export const x = 42;");
    const roundtrip = (try DataURL.parse(encoded)).?;
    expect_eq(roundtrip.is_base64, true);
    expect_eq(std.mem.eql(u8, try roundtrip.decodeAlloc(alloc), "export const x = 42;"), true);
}

// ============================================================================
// 2. cap:resolver — PackageJson (exports, imports, conditions, deps)
// ============================================================================
fn testPackageJson(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    const pkg_src =
        "{\n" ++
        "  \"name\": \"@cp/demo-pkg\",\n" ++
        "  \"version\": \"1.2.3\",\n" ++
        "  \"type\": \"module\",\n" ++
        "  \"main\": \"./dist/index.cjs\",\n" ++
        "  \"module\": \"./dist/index.mjs\",\n" ++
        "  \"browser\": {\n" ++
        "    \"./dist/net.js\": \"./dist/net.browser.js\"\n" ++
        "  },\n" ++
        "  \"exports\": {\n" ++
        "    \".\": {\n" ++
        "      \"types\": \"./dist/index.d.ts\",\n" ++
        "      \"cp\": \"./src/index.cp\",\n" ++
        "      \"import\": \"./dist/index.mjs\",\n" ++
        "      \"require\": \"./dist/index.cjs\",\n" ++
        "      \"default\": \"./dist/index.js\"\n" ++
        "    },\n" ++
        "    \"./foo\": {\n" ++
        "      \"bun\": \"./bun/foo.js\",\n" ++
        "      \"default\": \"./dist/foo.js\"\n" ++
        "    },\n" ++
        "    \"./internal\": null,\n" ++
        "    \"./*\": \"./src/*.cp\"\n" ++
        "  },\n" ++
        "  \"imports\": {\n" ++
        "    \"#config\": {\n" ++
        "      \"cp\": \"./src/config.cp\",\n" ++
        "      \"default\": \"./src/config.js\"\n" ++
        "    },\n" ++
        "    \"#utils/*\": \"./src/utils/*.cp\"\n" ++
        "  },\n" ++
        "  \"dependencies\": {\n" ++
        "    \"zod\": \"^3.23.0\"\n" ++
        "  },\n" ++
        "  \"devDependencies\": {\n" ++
        "    \"vitest\": \"^2.0.0\"\n" ++
        "  }\n" ++
        "}\n";

    const pkg = try PackageJson.parse(alloc, pkg_src);
    expect_eq(std.mem.eql(u8, pkg.name, "@cp/demo-pkg"), true);
    expect_eq(std.mem.eql(u8, pkg.version, "1.2.3"), true);
    expect_eq(pkg.module_type, ModuleType.Module);
    expect_eq(pkg.isEsm(), true);
    expect_eq(std.mem.eql(u8, pkg.main, "./dist/index.cjs"), true);
    expect_eq(std.mem.eql(u8, pkg.module, "./dist/index.mjs"), true);

    // Dependencies lookup
    expect_eq(std.mem.eql(u8, pkg.getDependency("zod").?, "^3.23.0"), true);
    expect_eq(std.mem.eql(u8, pkg.getDevDependency("vitest").?, "^2.0.0"), true);
    expect_eq(pkg.hasDependency("vitest"), true);
    expect_eq(pkg.hasDependency("missing"), false);

    // Browser field mapping
    expect_eq(std.mem.eql(u8, pkg.resolveBrowser("./dist/net.js").?, "./dist/net.browser.js"), true);

    // Exports conditional routing
    const cp_conds: [2][]const u8 = ["cp", "default"];
    const imp_conds: [2][]const u8 = ["import", "default"];
    const req_conds: [2][]const u8 = ["require", "default"];
    const bun_conds: [2][]const u8 = ["bun", "default"];
    const types_conds: [2][]const u8 = ["types", "default"];

    expect_eq(std.mem.eql(u8, pkg.resolveExportPath(alloc, ".", &cp_conds).?, "./src/index.cp"), true);
    expect_eq(std.mem.eql(u8, pkg.resolveExportPath(alloc, ".", &imp_conds).?, "./dist/index.mjs"), true);
    expect_eq(std.mem.eql(u8, pkg.resolveExportPath(alloc, ".", &req_conds).?, "./dist/index.cjs"), true);
    expect_eq(std.mem.eql(u8, pkg.resolveExportPath(alloc, ".", &types_conds).?, "./dist/index.d.ts"), true);
    expect_eq(std.mem.eql(u8, pkg.resolveExportPath(alloc, "./foo", &bun_conds).?, "./bun/foo.js"), true);

    // Wildcard export "./*" -> "./src/*.cp"
    expect_eq(std.mem.eql(u8, pkg.resolveExportPath(alloc, "./math/vec", &cp_conds).?, "./src/math/vec.cp"), true);

    // Explicitly disabled export -> null
    const disabled_res = try pkg.resolveExport(alloc, "./internal", &cp_conds);
    expect_eq(disabled_res.status, ExportResolveStatus.Disabled);

    // Private #imports routing
    expect_eq(std.mem.eql(u8, pkg.resolveImportPath(alloc, "#config", &cp_conds).?, "./src/config.cp"), true);
    expect_eq(std.mem.eql(u8, pkg.resolveImportPath(alloc, "#utils/hash", &cp_conds).?, "./src/utils/hash.cp"), true);
}

// ============================================================================
// 3. cap:resolver — JSONC TsConfigJson & Path Aliases
// ============================================================================
fn testTsConfigJson(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    const jsonc =
        "{\n" ++
        "  // Top-level extends\n" ++
        "  \"extends\": \"./tsconfig.base.json\",\n" ++
        "  /* Multi-line\n" ++
        "     JSONC block comment */\n" ++
        "  \"compilerOptions\": {\n" ++
        "    \"baseUrl\": \"src\",\n" ++
        "    \"rootDir\": \"src\",\n" ++
        "    \"outDir\": \"dist\",\n" ++
        "    \"target\": \"ES2022\",\n" ++
        "    \"jsx\": \"react-jsx\",\n" ++
        "    \"paths\": {\n" ++
        "      \"@/*\": [\"app/*\"], // trailing comma & comment in string: \"// keep\"\n" ++
        "      \"@ui/*\": [\"components/*/index.cp\"],\n" ++
        "      \"@env\": [\"config/env.cp\"],\n" ++
        "    },\n" ++
        "  },\n" ++
        "}\n";

    const tsc = try TsConfigJson.parse(alloc, jsonc);
    expect_eq(std.mem.eql(u8, tsc.extends, "./tsconfig.base.json"), true);
    expect_eq(std.mem.eql(u8, tsc.base_url, "src"), true);
    expect_eq(std.mem.eql(u8, tsc.root_dir, "src"), true);
    expect_eq(std.mem.eql(u8, tsc.out_dir, "dist"), true);
    expect_eq(std.mem.eql(u8, tsc.target, "ES2022"), true);
    expect_eq(std.mem.eql(u8, tsc.jsx, "react-jsx"), true);

    expect_eq(std.mem.eql(u8, tsc.resolveAlias(alloc, "@/core/router").?, "src/app/core/router"), true);
    expect_eq(std.mem.eql(u8, tsc.resolveAlias(alloc, "@ui/button").?, "src/components/button/index.cp"), true);
    expect_eq(std.mem.eql(u8, tsc.resolveAlias(alloc, "@env").?, "src/config/env.cp"), true);
    expect_eq(tsc.resolveAlias(alloc, "lodash") == null, true);
}

// ============================================================================
// 4. cap:resolver — ModuleResolver & StatHash (Protocols, Aliases, FS Probing, Exports)
// ============================================================================
fn testModuleResolverAndStatHash(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // Build a temporary project tree on disk
    let root_buf: [256]u8 = undefined;
    const root = tmpPath("cap_res_proj", "", &root_buf);
    mkdir(root);
    const src_dir = try std.fmt.allocPrint(alloc, "{s}/src", .{root});
    const utils_dir = try std.fmt.allocPrint(alloc, "{s}/src/utils", .{root});
    const nm_dir = try std.fmt.allocPrint(alloc, "{s}/node_modules", .{root});
    const pkg_dir = try std.fmt.allocPrint(alloc, "{s}/node_modules/acme-lib", .{root});
    const pkg_dist = try std.fmt.allocPrint(alloc, "{s}/node_modules/acme-lib/dist", .{root});
    mkdir(src_dir);
    mkdir(utils_dir);
    mkdir(nm_dir);
    mkdir(pkg_dir);
    mkdir(pkg_dist);

    const f_main = try std.fmt.allocPrint(alloc, "{s}/src/main.cp", .{root});
    const f_helper = try std.fmt.allocPrint(alloc, "{s}/src/helper.cp", .{root});
    const f_idx = try std.fmt.allocPrint(alloc, "{s}/src/utils/index.cp", .{root});
    const f_pkg_json = try std.fmt.allocPrint(alloc, "{s}/node_modules/acme-lib/package.json", .{root});
    const f_pkg_esm = try std.fmt.allocPrint(alloc, "{s}/node_modules/acme-lib/dist/esm.js", .{root});
    const f_pkg_cjs = try std.fmt.allocPrint(alloc, "{s}/node_modules/acme-lib/dist/cjs.cjs", .{root});

    writeFileSync(f_main, "pub fn main() {}");
    writeFileSync(f_helper, "pub fn help() {}");
    writeFileSync(f_idx, "pub fn util() {}");
    writeFileSync(f_pkg_json, "{\"name\":\"acme-lib\",\"type\":\"module\",\"exports\":{\".\":{\"import\":\"./dist/esm.js\",\"require\":\"./dist/cjs.cjs\"}}}");
    writeFileSync(f_pkg_esm, "export const x = 1;");
    writeFileSync(f_pkg_cjs, "module.exports = { x: 1 };");

    defer {
        remove(f_main);
        remove(f_helper);
        remove(f_idx);
        remove(f_pkg_json);
        remove(f_pkg_esm);
        remove(f_pkg_cjs);
        rmdir(pkg_dist);
        rmdir(pkg_dir);
        rmdir(nm_dir);
        rmdir(utils_dir);
        rmdir(src_dir);
        rmdir(root);
    }

    let resolver = ModuleResolver.init(root);
    const tsc = try TsConfigJson.parse(alloc, "{\"compilerOptions\":{\"baseUrl\":\".\",\"paths\":{\"@/*\":[\"src/*\"]}}}");
    resolver.setTsConfig(tsc);

    // 4.1 Builtin protocols: cap:* and node:*
    const r_cap = try resolver.resolve(alloc, src_dir, "cap:io", ResolveKind.ImportStmt);
    expect_eq(r_cap.kind, ResolvedKind.Builtin);
    expect_eq(std.mem.eql(u8, r_cap.protocol, "cap"), true);

    const r_node = try resolver.resolve(alloc, src_dir, "node:fs", ResolveKind.ImportStmt);
    expect_eq(r_node.kind, ResolvedKind.Builtin);
    expect_eq(std.mem.eql(u8, r_node.protocol, "node"), true);

    // 4.2 Data URL resolution
    const r_data = try resolver.resolve(alloc, src_dir, "data:text/javascript;base64,ZXhwb3J0IDE=", ResolveKind.ImportStmt);
    expect_eq(r_data.kind, ResolvedKind.DataUrl);
    expect_eq(r_data.data_url != null, true);

    // 4.3 Relative extension probing (./helper -> src/helper.cp) & directory index.cp (./utils -> src/utils/index.cp)
    const r_rel = try resolver.resolve(alloc, src_dir, "./helper", ResolveKind.ImportStmt);
    expect_eq(r_rel.kind, ResolvedKind.File);
    expect_eq(std.mem.eql(u8, r_rel.path, f_helper), true);
    expect_eq(r_rel.stat_hash != null, true);

    const r_idx = try resolver.resolve(alloc, src_dir, "./utils", ResolveKind.ImportStmt);
    expect_eq(r_idx.kind, ResolvedKind.File);
    expect_eq(std.mem.eql(u8, r_idx.path, f_idx), true);

    // 4.4 tsconfig paths alias (@/helper -> src/helper.cp)
    const r_alias = try resolver.resolve(alloc, src_dir, "@/helper", ResolveKind.ImportStmt);
    expect_eq(r_alias.kind, ResolvedKind.File);
    expect_eq(std.mem.eql(u8, r_alias.path, f_helper), true);

    // 4.5 package.json exports conditional routing via node_modules
    const r_pkg_esm = try resolver.resolve(alloc, src_dir, "acme-lib", ResolveKind.ImportStmt);
    expect_eq(r_pkg_esm.kind, ResolvedKind.File);
    expect_eq(std.mem.eql(u8, r_pkg_esm.path, f_pkg_esm), true);
    expect_eq(r_pkg_esm.module_type, ModuleType.Module);

    const r_pkg_cjs = try resolver.resolve(alloc, src_dir, "acme-lib", ResolveKind.RequireCall);
    expect_eq(r_pkg_cjs.kind, ResolvedKind.File);
    expect_eq(std.mem.eql(u8, r_pkg_cjs.path, f_pkg_cjs), true);

    // 4.6 StatHash determinism & sensitivity
    const sh1 = try StatHash.fromPath(f_helper);
    const sh2 = try StatHash.fromPath(f_helper);
    const sh3 = try StatHash.fromPath(f_idx);
    expect_eq(sh1.eql(sh2), true);
    expect_eq(sh1.eql(sh3), false);

    const meta1 = StatHash.fromMetadata(100, 0o100644, 1700000000, 7, "/a.cp");
    const meta2 = StatHash.fromMetadata(101, 0o100644, 1700000000, 7, "/a.cp");
    expect_eq(meta1.eql(meta2), false);
}

// ============================================================================
// 5. cap:bundler — CompileTarget
// ============================================================================
fn testCompileTarget(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    const t1 = try CompileTarget.parse("linux-x64-musl");
    expect_eq(t1.os, TargetOs.Linux);
    expect_eq(t1.arch, TargetArch.X64);
    expect_eq(t1.abi, TargetAbi.Musl);
    expect_eq(t1.isUnix(), true);
    expect_eq(std.mem.eql(u8, t1.exeExt(), ""), true);
    expect_eq(std.mem.eql(u8, t1.sharedLibExt(), ".so"), true);
    expect_eq(std.mem.eql(u8, try t1.formatAlloc(alloc), "linux-x64-musl"), true);

    const t2 = try CompileTarget.parse("bun-darwin-arm64");
    expect_eq(t2.os, TargetOs.Darwin);
    expect_eq(t2.arch, TargetArch.Arm64);
    expect_eq(t2.abi, TargetAbi.None);
    expect_eq(t2.isUnix(), true);
    expect_eq(std.mem.eql(u8, t2.sharedLibExt(), ".dylib"), true);
    expect_eq(std.mem.eql(u8, try t2.formatAlloc(alloc), "darwin-arm64"), true);

    const t3 = try CompileTarget.parse("windows-x86_64-msvc");
    expect_eq(t3.os, TargetOs.Windows);
    expect_eq(t3.arch, TargetArch.X64);
    expect_eq(t3.abi, TargetAbi.Msvc);
    expect_eq(t3.isUnix(), false);
    expect_eq(std.mem.eql(u8, t3.exeExt(), ".exe"), true);
    expect_eq(std.mem.eql(u8, t3.sharedLibExt(), ".dll"), true);

    const t4 = try CompileTarget.parse("wasi-wasm32");
    expect_eq(t4.os, TargetOs.Wasi);
    expect_eq(t4.arch, TargetArch.Wasm32);
    expect_eq(t4.isUnix(), false);
    expect_eq(std.mem.eql(u8, t4.exeExt(), ".wasm"), true);

    // Invalid combinations rejected
    let invalid_musl = false;
    * = CompileTarget.parse("darwin-arm64-musl") catch {
        invalid_musl = true;
    };
    expect_eq(invalid_musl, true);
}

// ============================================================================
// 6. cap:bundler — StandaloneModuleGraph (Serialize & Zero-Copy Deserialize)
// ============================================================================
fn testStandaloneModuleGraph(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    let sg = StandaloneModuleGraph.init(alloc);
    defer sg.deinit();

    * = try sg.addModule(StandaloneModuleEntry{
        path: "/$cpfs/main.cp",
        contents: "import { greet } from './util.cp'; greet();",
        sourcemap: "{\"version\":3}",
        format: StandaloneModuleFormat.Esm,
        side: StandaloneFileSide.Server,
        encoding: StandaloneEncoding.Utf8,
        is_entry: true,
    });
    * = try sg.addModule(StandaloneModuleEntry{
        path: "/$cpfs/util.cp",
        contents: "pub fn greet() {}",
        sourcemap: "",
        format: StandaloneModuleFormat.Esm,
        side: StandaloneFileSide.Client,
        encoding: StandaloneEncoding.Latin1,
        is_entry: false,
    });

    const blob = try sg.serialize(alloc);
    expect_eq(std.mem.startsWith(u8, blob, STANDALONE_HEADER_MAGIC), true);
    expect_eq(std.mem.endsWith(u8, blob, STANDALONE_TRAILER_MAGIC), true);

    let loaded = try StandaloneModuleGraph.deserialize(alloc, blob);
    defer loaded.deinit();

    expect_eq(loaded.count(), 2);
    const entry = loaded.entryPoint().?;
    expect_eq(std.mem.eql(u8, entry.path, "/$cpfs/main.cp"), true);
    expect_eq(std.mem.eql(u8, entry.sourcemap, "{\"version\":3}"), true);

    const util_entry = loaded.find("/$cpfs/util.cp").?;
    expect_eq(util_entry.side, StandaloneFileSide.Client);
    expect_eq(util_entry.encoding, StandaloneEncoding.Latin1);
    expect_eq(std.mem.eql(u8, loaded.findContent("/$cpfs/util.cp").?, "pub fn greet() {}"), true);
    expect_eq(loaded.find("/$cpfs/missing.cp") == null, true);

    // Zero-copy check: deserialized slice pointer falls strictly within `blob` address range
    const blob_start = unsafe { @intFromPtr(blob.ptr) };
    const blob_end = blob_start + blob.len;
    const content_ptr = unsafe { @intFromPtr(entry.contents.ptr) };
    expect_eq(content_ptr >= blob_start && content_ptr < blob_end, true);

    // Corrupt magic & truncated bounds protection
    let bad_blob = try alloc.dupe(u8, blob);
    bad_blob[0] = 'X';
    let caught_magic = false;
    * = StandaloneModuleGraph.deserialize(alloc, bad_blob) catch {
        caught_magic = true;
    };
    expect_eq(caught_magic, true);

    let bad_trailer = try alloc.dupe(u8, blob);
    bad_trailer[bad_trailer.len - 1] = 'Z';
    let caught_trailer = false;
    * = StandaloneModuleGraph.deserialize(alloc, bad_trailer) catch {
        caught_trailer = true;
    };
    expect_eq(caught_trailer, true);

    let caught_short = false;
    * = StandaloneModuleGraph.deserialize(alloc, blob[0..16]) catch {
        caught_short = true;
    };
    expect_eq(caught_short, true);
}

// ============================================================================
// 7. cap:bundler — ModuleGraph (TopoSort, Cycle Detection & Tree-Shaking)
// ============================================================================
fn testModuleGraphAndCompat(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // Diamond DAG: entry -> [a, b], a -> shared, b -> shared; plus unreachable dead node
    let mg = ModuleGraph.init(alloc);
    defer mg.deinit();

    const id_entry = try mg.addModule("entry.cp", true, 500);
    const id_a = try mg.addModule("a.cp", false, 200);
    const id_b = try mg.addModule("b.cp", false, 300);
    const id_shared = try mg.addModule("shared.cp", false, 150);
    const id_dead = try mg.addModule("dead.cp", false, 100);

    try mg.addDependency(id_entry, id_a, "./a", ImportKind.ImportStmt);
    try mg.addDependency(id_entry, id_b, "./b", ImportKind.DynamicImport);
    try mg.addDependency(id_a, id_shared, "./shared", ImportKind.ImportStmt);
    try mg.addDependency(id_b, id_shared, "./shared", ImportKind.RequireCall);
    try mg.addImport(id_entry, ImportRecord{
        specifier: "cap:io",
        kind: ImportKind.ImportStmt,
        resolved_index: INVALID_MODULE_INDEX,
        is_external: true,
    });

    expect_eq(mg.hasCycle(), false);

    // Deterministic dependency-first topological sort:
    // shared must precede a and b; a and b must precede entry
    const topo_order = try mg.topoSort(alloc);
    expect_eq(topo_order.len, 5);
    expect_eq(topo_order[0], id_shared);
    expect_eq(topo_order[1], id_a);
    expect_eq(topo_order[2], id_b);
    expect_eq(topo_order[3], id_entry);
    expect_eq(topo_order[4], id_dead);

    // Tree-shaking reachability
    mg.markAllEntryPointsReachable();
    expect_eq(mg.isReachable(id_entry), true);
    expect_eq(mg.isReachable(id_a), true);
    expect_eq(mg.isReachable(id_b), true);
    expect_eq(mg.isReachable(id_shared), true);
    expect_eq(mg.isReachable(id_dead), false);
    expect_eq(mg.reachableCount(), 4);

    // Introduce cycle: shared -> a
    try mg.addDependency(id_shared, id_a, "./a", ImportKind.ImportStmt);
    expect_eq(mg.hasCycle(), true);
    let caught_cycle = false;
    * = mg.topoSort(alloc) catch {
        caught_cycle = true;
    };
    expect_eq(caught_cycle, true);

    // BundleV2 compatibility sanity check
    let bv2 = BundleV2.init(BundleConfig.defaultConfig());
    bv2.addSourceBytes(1000);
    expect_eq(bv2.emitChunk(ChunkEntry{
        name: "app.js",
        kind: ChunkKind.EntryPoint,
        source_index: 0,
        size_bytes: 500,
        is_executable: true,
    }), true);
    expect_eq(bv2.compressionRatioPermille(), 500);
}

fn main() {
    test("1. cap:resolver DataURL RFC 2397 Parse & DecodeAlloc", () => {
        try testDataUrl();
    });
    test("2. cap:resolver PackageJson Exports, Imports & Dependencies", () => {
        try testPackageJson();
    });
    test("3. cap:resolver TsConfigJson JSONC & Path Alias Rewrite", () => {
        try testTsConfigJson();
    });
    test("4. cap:resolver ModuleResolver & StatHash Full Resolution", () => {
        try testModuleResolverAndStatHash();
    });
    test("5. cap:bundler CompileTarget Parse, Format & Extensions", () => {
        try testCompileTarget();
    });
    test("6. cap:bundler StandaloneModuleGraph Serialize & Zero-Copy Deserialize", () => {
        try testStandaloneModuleGraph();
    });
    test("7. cap:bundler ModuleGraph TopoSort, Cycle Detection & Tree-Shaking", () => {
        try testModuleGraphAndCompat();
    });
}
