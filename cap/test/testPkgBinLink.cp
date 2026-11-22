// Deep production regression tests for cap:pkg bin normalizer & link planner (lib/pkg/binLink.cp)

import { test, expect, expectError } from "cap:test";
import {
    BinLinkError,
    PkgBinKind,
    normalizedBinName,
    validateSafeBinName,
    normalizeBinTargetRel,
    PkgBinSpec,
    BinLinkPlanner,
} from "cap:pkg";

test("pkgBinLink: npm-normalize-package-bin name extraction and safety checks", () => {
    expect(normalizedBinName("@acme/my-cli")).toEqual("my-cli");
    expect(normalizedBinName("foo/bar:baz")).toEqual("baz");
    expect(normalizedBinName("tsc")).toEqual("tsc");

    expect(validateSafeBinName("  @types/node-cli  ")?).toEqual("node-cli");
    expectError(validateSafeBinName(""), BinLinkError.EmptyName);
    expectError(validateSafeBinName("foo/.."), BinLinkError.UnsafeBinName);
    expectError(validateSafeBinName("."), BinLinkError.UnsafeBinName);
    expectError(validateSafeBinName("bad\nname"), BinLinkError.UnsafeBinName);
});

test("pkgBinLink: target path normalization and traversal guards", () => {
    expect(normalizeBinTargetRel("././bin/cli.js")?).toEqual("bin/cli.js");
    expect(normalizeBinTargetRel("dist/index.mjs")?).toEqual("dist/index.mjs");

    expectError(normalizeBinTargetRel(""), BinLinkError.EmptyTarget);
    expectError(normalizeBinTargetRel("/etc/passwd"), BinLinkError.UnsafeTargetPath);
    expectError(normalizeBinTargetRel("C:/Windows/cmd.exe"), BinLinkError.UnsafeTargetPath);
    expectError(normalizeBinTargetRel("../../evil.js"), BinLinkError.UnsafeTargetPath);
    expectError(normalizeBinTargetRel("bin/../evil.js"), BinLinkError.UnsafeTargetPath);
    expectError(normalizeBinTargetRel("bin\\cli.js"), BinLinkError.UnsafeTargetPath);
});

test("pkgBinLink: PkgBinSpec single file, dir, map order-independent equality and JSON", () => {
    let buf: [128]u8 = [0; 128];

    const s_file = PkgBinSpec.fromSingleFile("@acme/tool", "./bin/tool.js")?;
    expect(s_file.kind == .File).toBe(true);
    expect(s_file.slice()[0].name).toEqual("tool");
    expect(s_file.slice()[0].target).toEqual("bin/tool.js");
    const n1 = s_file.toJson(buf[0..])?;
    expect(buf[0..n1]).toEqual("\"bin/tool.js\"");

    const s_dir = PkgBinSpec.fromDir("./scripts")?;
    expect(s_dir.kind == .Dir).toBe(true);
    expect(s_dir.dir_path).toEqual("scripts");

    let m1 = PkgBinSpec.init();
    * = m1.addMapEntry("babel", "./bin/babel.js")?;
    expect(m1.kind == .NamedFile).toBe(true);
    * = m1.addMapEntry("babel-cli", "./bin/cli.js")?;
    expect(m1.kind == .Map).toBe(true);

    // 逆序插入相同键值，Map 语义等价比较应返回 true
    let m2 = PkgBinSpec.init();
    * = m2.addMapEntry("babel-cli", "./bin/cli.js")?;
    * = m2.addMapEntry("babel", "./bin/babel.js")?;
    expect(m1.eql(m2)).toBe(true);

    const n2 = m1.toJson(buf[0..])?;
    expect(buf[0..n2]).toEqual("{ \"babel\": \"bin/babel.js\", \"babel-cli\": \"bin/cli.js\" }");
});

test("pkgBinLink: BinLinkPlanner local node_modules/.bin and global path formatting", () => {
    let planner = BinLinkPlanner.init();
    const spec = PkgBinSpec.fromSingleFile("@acme/cli", "./dist/cli.js")?;
    const added = planner.planSpec("@acme/cli", spec, false)?;
    expect(added).toBe(1 as usize);

    let buf: [128]u8 = [0; 128];
    const dlen = planner.formatDestPath(0, "node_modules", buf[0..])?;
    expect(buf[0..dlen]).toEqual("node_modules/.bin/cli");

    const tlen = planner.formatRelativeSymlinkTarget(0, buf[0..])?;
    expect(buf[0..tlen]).toEqual("../@acme/cli/dist/cli.js");

    // 全局安装模式不附加 `.bin/` 与 `../`
    const gspec = PkgBinSpec.fromSingleFile("esbuild", "./bin/esbuild")?;
    * = planner.planSpec("esbuild", gspec, true)?;
    const gdlen = planner.formatDestPath(1, "/usr/local/bin", buf[0..])?;
    expect(buf[0..gdlen]).toEqual("/usr/local/bin/esbuild");
    const gtlen = planner.formatRelativeSymlinkTarget(1, buf[0..])?;
    expect(buf[0..gtlen]).toEqual("esbuild/bin/esbuild");
});

test("pkgBinLink: conflict deduplication preserves first-registered direct dependency", () => {
    let planner = BinLinkPlanner.init();
    const direct = PkgBinSpec.fromSingleFile("tsc-direct", "./bin/tsc.js")?;
    let direct_map = PkgBinSpec.init();
    * = direct_map.addMapEntry("tsc", "./bin/tsc.js")?;

    let transitive_map = PkgBinSpec.init();
    * = transitive_map.addMapEntry("tsc", "./evil/tsc.js")?;
    * = transitive_map.addMapEntry("tsserver", "./bin/tsserver.js")?;

    * = direct;
    expect(planner.planSpec("typescript", direct_map, false)?).toBe(1 as usize);
    // 传递依赖的同名 `tsc` 被跳过，仅新增 `tsserver`
    expect(planner.planSpec("legacy-ts", transitive_map, false)?).toBe(1 as usize);
    expect(planner.skipped_conflicts).toBe(1 as usize);
    expect(planner.count).toBe(2 as usize);
    expect(planner.slice()[0].pkg_name).toEqual("typescript");
    expect(planner.slice()[1].bin_name).toEqual("tsserver");
});

test("pkgBinLink: invalid index, empty package name and small buffer guards", () => {
    let planner = BinLinkPlanner.init();
    const spec = PkgBinSpec.fromSingleFile("vite", "./bin/vite.js")?;

    expectError(planner.planSpec("   ", spec, false), BinLinkError.EmptyName);
    * = planner.planSpec("vite", spec, false)?;

    let tiny: [6]u8 = [0; 6];
    expectError(planner.formatDestPath(9, "node_modules", tiny[0..]), BinLinkError.InvalidIndex);
    expectError(planner.formatRelativeSymlinkTarget(9, tiny[0..]), BinLinkError.InvalidIndex);
    expectError(planner.formatDestPath(0, "node_modules", tiny[0..]), BinLinkError.BufferTooSmall);
    expectError(spec.toJson(tiny[0..]), BinLinkError.BufferTooSmall);
});
