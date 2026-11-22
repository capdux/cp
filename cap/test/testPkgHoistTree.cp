// Deep production regression tests for cap:pkg HoistTreeBuilder (lib/pkg/hoistTree.cp)

import { test, expect, expectError } from "cap:test";
import {
    HOIST_INVALID_ID,
    HOIST_DEP_NORMAL,
    HOIST_DEP_DEV,
    HOIST_DEP_OPTIONAL,
    HOIST_DEP_PEER,
    HOIST_DEP_BUNDLED,
    HOIST_DEP_FOLDER,
    HoistTreeError,
    HoistPathStyle,
    HoistBuildMode,
    HoistOmitPolicy,
    HoistTreeBuilder,
} from "cap:pkg";

test("hoistTree: flat hoisting and deduplication prunes empty subtrees", () => {
    let b = HoistTreeBuilder.init();
    const root = b.addPackage("app", "1.0.0", false)?;
    const pkg_a = b.addPackage("pkg-a", "1.0.0", false)?;
    const pkg_b = b.addPackage("pkg-b", "1.0.0", false)?;
    const shared = b.addPackage("shared", "2.0.0", false)?;

    * = b.addDependency(root, "pkg-a", pkg_a, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(root, "pkg-b", pkg_b, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(pkg_a, "shared", shared, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(pkg_b, "shared", shared, HOIST_DEP_NORMAL, false)?;

    const count = b.build(.Resolvable, HoistOmitPolicy{})?;
    // pkg-a 和 pkg-b 的 shared 均提升到根 node_modules，空子树被弹出，只剩根子树
    expect(count).toBe(1 as usize);
    expect(b.treeDepCount(0)?).toBe(3 as usize);

    let buf: [128]u8 = [0; 128];
    const p_nm = b.formatRelativePath(0, .NodeModules, buf[0..])?;
    expect(p_nm.depth).toBe(0 as usize);
    expect(buf[0..p_nm.path_len]).toEqual("node_modules");

    const p_pkg = b.formatRelativePath(0, .PkgPath, buf[0..])?;
    expect(p_pkg.depth).toBe(0 as usize);
    expect(p_pkg.path_len).toBe(0 as usize);
});

test("hoistTree: version conflict nests into child node_modules with path formatting", () => {
    let b = HoistTreeBuilder.init();
    const root = b.addPackage("app", "1.0.0", false)?;
    const lodash4 = b.addPackage("lodash", "4.17.21", false)?;
    const lodash3 = b.addPackage("lodash", "3.10.1", false)?;
    const legacy = b.addPackage("legacy", "1.0.0", false)?;

    * = b.addDependency(root, "lodash", lodash4, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(root, "legacy", legacy, HOIST_DEP_NORMAL, false)?;
    const nested_dep = b.addDependency(legacy, "lodash", lodash3, HOIST_DEP_NORMAL, false)?;

    const count = b.build(.Resolvable, HoistOmitPolicy{})?;
    expect(count).toBe(2 as usize);
    expect(b.treeDepCount(0)?).toBe(2 as usize);
    expect(b.treeDepCount(1)?).toBe(1 as usize);
    expect(b.treeDepAt(1, 0)?).toBe(nested_dep);

    let buf: [128]u8 = [0; 128];
    const nm = b.formatRelativePath(1, .NodeModules, buf[0..])?;
    expect(nm.depth).toBe(1 as usize);
    expect(buf[0..nm.path_len]).toEqual("node_modules/legacy/node_modules");

    const pp = b.formatRelativePath(1, .PkgPath, buf[0..])?;
    expect(pp.depth).toBe(1 as usize);
    expect(buf[0..pp.path_len]).toEqual("legacy");

    const install_len = b.formatPackageInstallPath(1, "lodash", buf[0..])?;
    expect(buf[0..install_len]).toEqual("node_modules/legacy/node_modules/lodash");
});

test("hoistTree: bundled and folder dependencies stay scoped to hoist_root_id", () => {
    let b = HoistTreeBuilder.init();
    const root = b.addPackage("app", "1.0.0", false)?;
    const host = b.addPackage("host", "1.0.0", false)?;
    const bundled = b.addPackage("bundled-lib", "1.0.0", false)?;
    const inner = b.addPackage("inner-util", "1.0.0", false)?;
    const local_pkg = b.addPackage("local-dir", "1.0.0", false)?;

    * = b.addDependency(root, "host", host, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(host, "bundled-lib", bundled, HOIST_DEP_BUNDLED, false)?;
    * = b.addDependency(bundled, "inner-util", inner, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(host, "local-dir", local_pkg, HOIST_DEP_FOLDER, false)?;

    const count = b.build(.Resolvable, HoistOmitPolicy{})?;
    // tree 0: root (host)
    // tree 1: host (bundled-lib, local-dir, inner-util 无法越过 host 向上提升到 root)
    expect(count).toBe(2 as usize);
    expect(b.treeDepCount(0)?).toBe(1 as usize);
    expect(b.treeDepCount(1)?).toBe(3 as usize);
});

test("hoistTree: optional peer deferred resolution and placeholder replacement", () => {
    let b = HoistTreeBuilder.init();
    const root = b.addPackage("app", "1.0.0", false)?;
    const ui_a = b.addPackage("ui-a", "1.0.0", false)?;
    const ui_b = b.addPackage("ui-b", "1.0.0", false)?;
    const react18 = b.addPackage("react", "18.2.0", false)?;

    * = b.addDependency(root, "ui-a", ui_a, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(root, "ui-b", ui_b, HOIST_DEP_NORMAL, false)?;
    // ui-a 声明未解析的 optional peer react
    const opt_peer_a = b.addDependency(
        ui_a,
        "react",
        HOIST_INVALID_ID,
        HOIST_DEP_OPTIONAL | HOIST_DEP_PEER,
        true,
    )?;
    // ui-b 随后提供真正的 react@18.2.0，触发 ResolveReplace 回填 opt_peer_a
    const real_react = b.addDependency(ui_b, "react", react18, HOIST_DEP_NORMAL, false)?;

    const count = b.build(.Resolvable, HoistOmitPolicy{})?;
    expect(count).toBe(1 as usize);
    expect(b.resolvedPkg(opt_peer_a)?).toBe(react18);
    expect(b.resolvedPkg(real_react)?).toBe(react18);
});

test("hoistTree: filter mode skips disabled packages and omitted dev dependencies", () => {
    let b = HoistTreeBuilder.init();
    const root = b.addPackage("app", "1.0.0", false)?;
    const win_only = b.addPackage("fsevents-win", "1.0.0", true)?;
    const dev_tool = b.addPackage("vitest", "1.0.0", false)?;
    const prod_lib = b.addPackage("zod", "3.22.0", false)?;
    const zod_old = b.addPackage("zod", "3.20.0", false)?;

    * = b.addDependency(root, "fsevents-win", win_only, HOIST_DEP_OPTIONAL, false)?;
    * = b.addDependency(root, "vitest", dev_tool, HOIST_DEP_DEV, false)?;
    * = b.addDependency(root, "zod", zod_old, HOIST_DEP_DEV, false)?;
    * = b.addDependency(dev_tool, "zod", prod_lib, HOIST_DEP_NORMAL, false)?;

    // Resolvable 模式保留所有依赖
    const res_count = b.build(.Resolvable, HoistOmitPolicy{})?;
    expect(res_count).toBe(2 as usize);
    expect(b.treeDepCount(0)?).toBe(3 as usize);

    // Filter + omit_dev 模式跳过 win_only (disabled) 和 vitest/zod_old (dev)
    const flt_count = b.build(.Filter, HoistOmitPolicy{ omit_dev: true })?;
    expect(flt_count).toBe(1 as usize);
    expect(b.treeDepCount(0)?).toBe(0 as usize);
});

test("hoistTree: boundary checks, dependency loop and buffer overflow guards", () => {
    let b = HoistTreeBuilder.init();
    expectError(b.addPackage("", "1.0.0", false), HoistTreeError.EmptyName);
    expectError(b.build(.Resolvable, HoistOmitPolicy{}), HoistTreeError.InvalidPackageId);

    const root = b.addPackage("app", "1.0.0", false)?;
    const v1 = b.addPackage("dup", "1.0.0", false)?;
    const v2 = b.addPackage("dup", "2.0.0", false)?;
    expectError(b.addDependency(99, "dup", v1, HOIST_DEP_NORMAL, false), HoistTreeError.InvalidPackageId);
    expectError(b.addDependency(root, "", v1, HOIST_DEP_NORMAL, false), HoistTreeError.EmptyName);

    // 同级两个非 peer 同名不同版本依赖触发 DependencyLoop
    * = b.addDependency(root, "dup", v1, HOIST_DEP_NORMAL, false)?;
    * = b.addDependency(root, "dup", v2, HOIST_DEP_NORMAL, false)?;
    expectError(b.build(.Resolvable, HoistOmitPolicy{}), HoistTreeError.DependencyLoop);

    // 单独构造合法树验证 InvalidTreeId / BufferTooSmall
    let b2 = HoistTreeBuilder.init();
    const r2 = b2.addPackage("app", "1.0.0", false)?;
    const a2 = b2.addPackage("alpha", "1.0.0", false)?;
    * = b2.addDependency(r2, "alpha", a2, HOIST_DEP_NORMAL, false)?;
    * = b2.build(.Resolvable, HoistOmitPolicy{})?;

    let tiny: [4]u8 = [0; 4];
    expectError(b2.formatRelativePath(0, .NodeModules, tiny[0..]), HoistTreeError.BufferTooSmall);
    expectError(b2.formatRelativePath(9, .NodeModules, tiny[0..]), HoistTreeError.InvalidTreeId);
    expectError(b2.treeDepAt(0, 99), HoistTreeError.InvalidDepId);
});
