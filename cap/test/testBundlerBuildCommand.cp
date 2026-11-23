import { test, assert, assertEq } from "cap:test";
import {
    BuildCommandError,
    formatHumanByteSize,
    BuildCmdTarget,
    BuildCmdFormat,
    BuildCmdSourceMap,
    BuildCmdKeyValue,
    BuildCommandConfig,
    BuildCmdParseBuffers,
    parseBuildCommandArgs,
    BuildValidationIssueKind,
    isSafeBuildOutputPath,
    validateBuildNamingTemplate,
    validateBuildConfig,
    CrossOsKind,
    CrossArchKind,
    CrossAbiKind,
    parseCrossCompileTarget,
    formatCrossArtifactPath,
    BuildArtifactKind,
    BuildArtifactEntry,
    BuildSummaryMetrics,
    formatBuildArtifactTable,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("parseBuildCommandArgs parses multi-entry, flags, minify, sourcemap, define, loader, external, and naming templates", () => {
    const argv: [18][]const u8 = [
        "src/app.cp",
        "src/worker.cp",
        "--outdir",
        "dist",
        "--target=linux-arm64",
        "--format",
        "esm",
        "--minify-whitespace",
        "--minify-syntax",
        "--sourcemap=external",
        "--splitting",
        "--external",
        "fsevents",
        "--define",
        "NODE_ENV=\"production\"",
        "--loader=.svg:file",
        "--public-path=/static/",
        "--watch",
    ];
    let entries: [4][]const u8 = [""; 4];
    let exts: [4][]const u8 = [""; 4];
    let defs: [4]BuildCmdKeyValue = [BuildCmdKeyValue.empty(); 4];
    let ldrs: [4]BuildCmdKeyValue = [BuildCmdKeyValue.empty(); 4];
    let bufs = BuildCmdParseBuffers{
        entry_points: entries[0..],
        externals: exts[0..],
        defines: defs[0..],
        loaders: ldrs[0..],
    };

    const cfg = parseBuildCommandArgs(argv[0..], &bufs) catch unreachable;
    assertEq(cfg.entry_points.len, @as(usize, 2));
    assert(strEql(cfg.entry_points[0], "src/app.cp"));
    assert(strEql(cfg.entry_points[1], "src/worker.cp"));
    assert(strEql(cfg.outdir, "dist"));
    assert(cfg.target == BuildCmdTarget.LinuxArm64);
    assert(cfg.format == BuildCmdFormat.Esm);
    assert(cfg.minify_whitespace && cfg.minify_syntax && !cfg.minify_identifiers);
    assert(cfg.isMinified());
    assert(cfg.sourcemap == BuildCmdSourceMap.External);
    assert(cfg.splitting);
    assert(cfg.watch);
    assertEq(cfg.externals.len, @as(usize, 1));
    assert(strEql(cfg.externals[0], "fsevents"));
    assertEq(cfg.defines.len, @as(usize, 1));
    assert(strEql(cfg.defines[0].key, "NODE_ENV"));
    assertEq(cfg.loaders.len, @as(usize, 1));
    assert(strEql(cfg.loaders[0].key, ".svg"));
    assert(strEql(cfg.loaders[0].value, "file"));
    assert(strEql(cfg.public_path, "/static/"));

    const val = validateBuildConfig(cfg);
    assert(val.valid);
});

test("parseBuildCommandArgs rejects unknown flags, missing values, and invalid define/loader/target syntax", () => {
    let entries: [4][]const u8 = [""; 4];
    let exts: [4][]const u8 = [""; 4];
    let defs: [4]BuildCmdKeyValue = [BuildCmdKeyValue.empty(); 4];
    let ldrs: [4]BuildCmdKeyValue = [BuildCmdKeyValue.empty(); 4];
    let bufs = BuildCmdParseBuffers{
        entry_points: entries[0..],
        externals: exts[0..],
        defines: defs[0..],
        loaders: ldrs[0..],
    };

    const bad_flag: [1][]const u8 = ["--bogus-option"];
    let unk_err = false;
    * = parseBuildCommandArgs(bad_flag[0..], &bufs) catch |e| {
        unk_err = (e == BuildCommandError.UnknownFlag);
    };
    assert(unk_err);

    const bad_target: [2][]const u8 = ["--target", "plan9-mips"];
    let tgt_err = false;
    * = parseBuildCommandArgs(bad_target[0..], &bufs) catch |e| {
        tgt_err = (e == BuildCommandError.InvalidTarget);
    };
    assert(tgt_err);

    const bad_define: [2][]const u8 = ["--define", "NO_EQUALS"];
    let def_err = false;
    * = parseBuildCommandArgs(bad_define[0..], &bufs) catch |e| {
        def_err = (e == BuildCommandError.InvalidDefineSyntax);
    };
    assert(def_err);

    const bad_loader: [2][]const u8 = ["--loader", "svg:file"];
    let ldr_err = false;
    * = parseBuildCommandArgs(bad_loader[0..], &bufs) catch |e| {
        ldr_err = (e == BuildCommandError.InvalidLoaderSyntax);
    };
    assert(ldr_err);

    const missing_val: [1][]const u8 = ["--outdir"];
    let miss_err = false;
    * = parseBuildCommandArgs(missing_val[0..], &bufs) catch |e| {
        miss_err = (e == BuildCommandError.MissingFlagValue);
    };
    assert(miss_err);
});

test("validateBuildConfig catches mutually exclusive flags and reserved standalone binary names", () => {
    const one_entry: [1][]const u8 = ["src/main.cp"];
    const two_entries: [2][]const u8 = ["src/a.cp", "src/b.cp"];

    // 1. No entry points
    const v0 = validateBuildConfig(BuildCommandConfig.defaults());
    assert(!v0.valid && v0.issue == BuildValidationIssueKind.NoEntryPoints);

    // 2. --outdir + --outfile conflict
    const v1 = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        outdir: "dist",
        outfile: "dist/out.js",
    });
    assert(!v1.valid && v1.issue == BuildValidationIssueKind.OutdirAndOutfileConflict);

    // 3. Multi-entry without --outdir
    const v2 = validateBuildConfig(BuildCommandConfig{
        entry_points: two_entries[0..],
        outfile: "bundle.js",
    });
    assert(!v2.valid && v2.issue == BuildValidationIssueKind.MultiEntryRequiresOutdir);

    // 4. --splitting + cjs
    const v3 = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        outdir: "dist",
        format: BuildCmdFormat.Cjs,
        splitting: true,
    });
    assert(!v3.valid && v3.issue == BuildValidationIssueKind.SplittingIncompatibleWithCjs);

    // 5. --compile + --splitting
    const v4 = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        compile: true,
        splitting: true,
    });
    assert(!v4.valid && v4.issue == BuildValidationIssueKind.CompileIncompatibleWithSplitting);

    // 6. --compile + --outdir
    const v5 = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        compile: true,
        outdir: "bin",
    });
    assert(!v5.valid && v5.issue == BuildValidationIssueKind.CompileIncompatibleWithOutdir);

    // 7. --sourcemap=external without --outdir
    const v6 = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        sourcemap: BuildCmdSourceMap.External,
    });
    assert(!v6.valid && v6.issue == BuildValidationIssueKind.ExternalSourcemapRequiresOutdir);

    // 8. Reserved standalone binary name "cap"
    const v7 = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        compile: true,
        outfile: "bin/cap",
    });
    assert(!v7.valid && v7.issue == BuildValidationIssueKind.ReservedStandaloneBinaryName);
});

test("isSafeBuildOutputPath and validateBuildNamingTemplate block path traversal and invalid placeholders", () => {
    assert(isSafeBuildOutputPath("dist/assets/app.js"));
    assert(isSafeBuildOutputPath("dist/v1..2/app.js"));
    assert(!isSafeBuildOutputPath("../outside/app.js"));
    assert(!isSafeBuildOutputPath("dist/../../etc/passwd"));
    assert(!isSafeBuildOutputPath(".."));

    assert(validateBuildNamingTemplate("[dir]/[name]-[hash].[ext]"));
    assert(validateBuildNamingTemplate("chunks/[name].[ext]"));
    assert(!validateBuildNamingTemplate("[dir]/[unknown].[ext]"));
    assert(!validateBuildNamingTemplate("[dir]/[name.[ext]"));
    assert(!validateBuildNamingTemplate("../[name].[ext]"));
    assert(!validateBuildNamingTemplate(""));

    const one_entry: [1][]const u8 = ["src/main.cp"];
    const v_trav = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        outdir: "../escape",
    });
    assert(!v_trav.valid && v_trav.issue == BuildValidationIssueKind.PathTraversalInOutput);

    const v_tpl = validateBuildConfig(BuildCommandConfig{
        entry_points: one_entry[0..],
        outdir: "dist",
        chunk_naming: "[badtoken].[ext]",
    });
    assert(!v_tpl.valid && v_tpl.issue == BuildValidationIssueKind.InvalidNamingPlaceholder);
});

test("parseCrossCompileTarget parses canonical cap target triples, musl/baseline variants, wasm, and nest", () => {
    const t1 = parseCrossCompileTarget("cap-linux-x86_64") catch unreachable;
    assert(t1.os == CrossOsKind.Linux && t1.arch == CrossArchKind.X86_64 && t1.abi == CrossAbiKind.Gnu);
    let dbuf: [64]u8 = [0; 64];
    const d1 = t1.canonicalDirName(dbuf[0..]) catch unreachable;
    assert(strEql(dbuf[0..d1], "cap-linux-x86_64"));

    const t2 = parseCrossCompileTarget("cap-macos-aarch64") catch unreachable;
    assert(t2.os == CrossOsKind.Macos && t2.arch == CrossArchKind.Aarch64 && t2.abi == CrossAbiKind.Darwin);
    const d2 = t2.canonicalDirName(dbuf[0..]) catch unreachable;
    assert(strEql(dbuf[0..d2], "cap-macos-aarch64"));

    const t3 = parseCrossCompileTarget("linux-arm64-musl-baseline") catch unreachable;
    assert(t3.os == CrossOsKind.Linux && t3.arch == CrossArchKind.Aarch64 && t3.abi == CrossAbiKind.Musl && t3.is_baseline);
    const d3 = t3.canonicalDirName(dbuf[0..]) catch unreachable;
    assert(strEql(dbuf[0..d3], "cap-linux-aarch64-musl-baseline"));

    const t4 = parseCrossCompileTarget("cap-windows-x86_64") catch unreachable;
    assert(t4.os == CrossOsKind.Windows && t4.arch == CrossArchKind.X86_64 && t4.abi == CrossAbiKind.Msvc);

    const t5 = parseCrossCompileTarget("wasm32-wasi") catch unreachable;
    assert(t5.os == CrossOsKind.Wasm && t5.arch == CrossArchKind.Wasm32);

    const t6 = parseCrossCompileTarget("cap-nest") catch unreachable;
    assert(t6.os == CrossOsKind.Nest);

    let inv_err = false;
    * = parseCrossCompileTarget("linux-only") catch |e| {
        inv_err = (e == BuildCommandError.InvalidTarget);
    };
    assert(inv_err);
});

test("formatCrossArtifactPath appends .exe on Windows, .nest in nest mode, and avoids duplicate extensions", () => {
    let buf: [128]u8 = [0; 128];

    const win = parseCrossCompileTarget("cap-windows-x86_64") catch unreachable;
    const p1 = formatCrossArtifactPath("bin", "hello", win, false, true, buf[0..]) catch unreachable;
    assert(strEql(buf[0..p1], "bin/cap-windows-x86_64/hello.exe"));

    // No duplicate .exe.exe
    const p2 = formatCrossArtifactPath("bin", "hello.exe", win, false, false, buf[0..]) catch unreachable;
    assert(strEql(buf[0..p2], "bin/hello.exe"));

    // Nest container mode (-n) auto-appends .nest
    const lin = parseCrossCompileTarget("cap-linux-x86_64") catch unreachable;
    const p3 = formatCrossArtifactPath("dist", "agent", lin, true, false, buf[0..]) catch unreachable;
    assert(strEql(buf[0..p3], "dist/agent.nest"));

    // Wasm auto-appends .wasm
    const wasm = parseCrossCompileTarget("wasm") catch unreachable;
    const p4 = formatCrossArtifactPath("dist", "kernel", wasm, false, true, buf[0..]) catch unreachable;
    assert(strEql(buf[0..p4], "dist/cap-wasm/kernel.wasm"));

    // Path traversal rejected
    let trav_err = false;
    * = formatCrossArtifactPath("../evil", "hello", lin, false, false, buf[0..]) catch |e| {
        trav_err = (e == BuildCommandError.PathTraversalDetected);
    };
    assert(trav_err);
});

test("formatHumanByteSize and formatBuildArtifactTable render aligned columns, badges, and build summary", () => {
    let sbuf: [32]u8 = [0; 32];
    let spos: usize = 0;
    formatHumanByteSize(512, sbuf[0..], &spos) catch unreachable;
    assert(strEql(sbuf[0..spos], "512 B"));

    spos = 0;
    formatHumanByteSize(1536, sbuf[0..], &spos) catch unreachable;
    assert(strEql(sbuf[0..spos], "1.50 KB"));

    spos = 0;
    formatHumanByteSize(2 * 1024 * 1024 + 512 * 1024, sbuf[0..], &spos) catch unreachable;
    assert(strEql(sbuf[0..spos], "2.50 MB"));

    const artifacts: [4]BuildArtifactEntry = [
        BuildArtifactEntry{ path: "bin/cap-linux-x86_64/app", kind: BuildArtifactKind.Native, byte_size: 2621440 },
        BuildArtifactEntry{ path: "dist/chunk-a1b2c3d4.js", kind: BuildArtifactKind.Chunk, byte_size: 14540 },
        BuildArtifactEntry{ path: "dist/app.js.map", kind: BuildArtifactKind.SourceMap, byte_size: 32768 },
        BuildArtifactEntry{ path: "dist/agent.nest", kind: BuildArtifactKind.Nest, byte_size: 512 },
    ];
    const metrics = BuildSummaryMetrics{
        elapsed_ms: 142,
        input_module_count: 28,
        cache_hits: 25,
        warnings_count: 1,
    };

    let tbuf: [1024]u8 = [0; 1024];
    const tlen = formatBuildArtifactTable(artifacts[0..], metrics, false, tbuf[0..]) catch unreachable;
    const table = tbuf[0..tlen];
    assert(strContains(table, "Artifact"));
    assert(strContains(table, "bin/cap-linux-x86_64/app  native       2.50 MB"));
    assert(strContains(table, "dist/app.js.map           sourcemap   32.00 KB"));
    assert(strContains(table, "dist/agent.nest           nest           512 B"));
    assert(strContains(table, "Built 4 artifact(s)"));
    assert(strContains(table, "in 142ms | 28 module(s), 25 cached | 1 warning(s)"));
});
