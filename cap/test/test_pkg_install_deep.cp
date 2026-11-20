// cap/test/test_pkg_install_deep.cp
// Deep End-to-End Verification of cap:pkg (lib/pkg/)
const std = @import("std");
import cap_hub from "cap";
import { test, expect_eq } from "cap:test";
import {
    makeTempDir,
    rmdirRecursive,
    readFile,
    writeFile,
    exists,
    stat,
    readlink,
} from "cap:sys";
import { UnifiedDiff } from "cap:patch";
import { GZIP_MAGIC_ID1, GZIP_MAGIC_ID2 } from "cap:pack";
import {
    Integrity,
    NodeLinker,
    StringBuf,
    SemverString,
    ExternalString,
    SlicedString,
    ExternalSlice,
    Behavior,
    InstallFeatures,
    hashPackageName,
    DependencyVersionTag,
    DependencyVersion,
    Dependency,
    DependencyList,
    ResolutionTag,
    Resolution,
    OperatingSystem,
    Architecture,
    PackageMeta,
    BinLink,
    BinLinkKind,
    RegistryConfig,
    NpmPackageManifest,
    Lockfile,
    TarEntryInput,
    TarballBuilder,
    TarballExtractor,
    TarballExtractOptions,
    InstallStrategy,
    CopyMethod,
    PackageManager,
    PackageManagerConfig,
    BINARY_LOCKFILE_MAGIC,
} from "cap:pkg";

fn test_types_and_cap_hub(): !void {
    const allocator = std.heap.page_allocator;

    // Verify `import cap_hub from "cap"` lazy mounting for core & semver
    expect_eq(cap_hub.Environment.is64Bit, true);
    const parsed_v = cap_hub.semver.Version.parse("1.2.3") catch unreachable;
    expect_eq(parsed_v.major, 1 as u32);
    const linker_mode = NodeLinker.fromStr("isolated");
    expect_eq(linker_mode != null, true);
    expect_eq(linker_mode.? == NodeLinker.Isolated, true);
    expect_eq(std.mem.eql(u8, NodeLinker.Hoisted.asStr(), "hoisted"), true);

    // Compact SemverString inline vs external
    let sbuf = StringBuf.init();
    defer sbuf.deinit(allocator);

    let short_ref = try sbuf.append(allocator, "1.2.3");
    expect_eq(short_ref.isInline(), true);
    expect_eq(short_ref.len(), 5);
    expect_eq(std.mem.eql(u8, SemverString.sliceOf(&short_ref, sbuf.items()), "1.2.3"), true);

    let long_ref = try sbuf.append(allocator, "@scope/very-long-package-name");
    expect_eq(long_ref.isInline(), false);
    expect_eq(long_ref.len(), 29);
    expect_eq(std.mem.eql(u8, SemverString.sliceOf(&long_ref, sbuf.items()), "@scope/very-long-package-name"), true);

    // String deduplication in StringBuf
    const long_ref2 = try sbuf.append(allocator, "@scope/very-long-package-name");
    expect_eq(long_ref.eql(long_ref2, sbuf.items(), sbuf.items()), true);

    const ext = ExternalString.from("lodash");
    expect_eq(ext.isInline(), true);
    expect_eq(ext.hash != 0, true);

    const backing = "prefix_hello_world_suffix";
    const sliced = SlicedString.init(backing, backing[7..18]);
    expect_eq(sliced.len(), 11);
    expect_eq(sliced.hash() != 0, true);

    const eslice = ExternalSlice(u32).fromOffsets(2, 5);
    expect_eq(eslice.contains(4), true);
    expect_eq(eslice.contains(7), false);
    const nums: [8]u32 = [10, 20, 30, 40, 50, 60, 70, 80];
    const sub = eslice.get(&nums);
    expect_eq(sub.len, 5);
    expect_eq(sub[0], 30);

    // Behavior bitfield & canonical ordering
    const b_prod = Behavior.normal();
    const b_dev = Behavior.devOnly();
    expect_eq(b_prod.isProd(), true);
    expect_eq(b_dev.isDev(), true);
    expect_eq(b_dev.cmp(b_prod) == .lt, true);
    expect_eq(b_prod.isEnabled(InstallFeatures{ dev_dependencies: false }), true);
    expect_eq(b_dev.isEnabled(InstallFeatures{ dev_dependencies: false }), false);
}

fn test_dependency_all_protocols(): !void {
    const allocator = std.heap.page_allocator;

    // 1. Standard semver range
    const d1 = try DependencyVersion.parse("lodash", "^4.17.21");
    expect_eq(d1.tag == .Npm, true);
    expect_eq(d1.satisfiesVersion("4.17.21"), true);
    expect_eq(d1.satisfiesVersion("4.18.0"), true);
    expect_eq(d1.satisfiesVersion("5.0.0"), false);

    // 2. npm: alias protocol
    const d2 = try DependencyVersion.parse("my-lodash", "npm:@acme/lodash-fork@^2.1.0");
    expect_eq(d2.tag == .Npm, true);
    expect_eq(d2.is_alias, true);
    expect_eq(std.mem.eql(u8, d2.package_name, "@acme/lodash-fork"), true);
    expect_eq(std.mem.eql(u8, d2.npm_range, "^2.1.0"), true);

    // 3. workspace: protocol
    const d3 = try DependencyVersion.parse("@app/shared", "workspace:*");
    expect_eq(d3.tag == .Workspace, true);
    expect_eq(std.mem.eql(u8, d3.workspace_spec, "*"), true);

    // 4. file: folder & local tarball
    const d4 = try DependencyVersion.parse("local-mod", "file:../packages/local-mod");
    expect_eq(d4.tag == .Folder, true);
    expect_eq(std.mem.eql(u8, d4.path, "../packages/local-mod"), true);

    const d4b = try DependencyVersion.parse("local-tgz", "file:./vendor/pkg-1.0.0.tgz");
    expect_eq(d4b.tag == .Tarball, true);
    expect_eq(d4b.tarball_kind == .Local, true);

    // 5. link: symlink protocol
    const d5 = try DependencyVersion.parse("linked-lib", "link:../linked-lib");
    expect_eq(d5.tag == .Symlink, true);
    expect_eq(std.mem.eql(u8, d5.path, "../linked-lib"), true);

    // 6. Remote tarball URL
    const d6 = try DependencyVersion.parse("remote-tgz", "https://cdn.example.com/foo-1.2.3.tgz");
    expect_eq(d6.tag == .Tarball, true);
    expect_eq(d6.tarball_kind == .Remote, true);
    expect_eq(std.mem.eql(u8, d6.tarball_url, "https://cdn.example.com/foo-1.2.3.tgz"), true);

    // 7. GitHub shortcut
    const d7 = try DependencyVersion.parse("bun-pkg", "github:oven-sh/bun#main");
    expect_eq(d7.tag == .GitHub, true);
    expect_eq(std.mem.eql(u8, d7.git_owner, "oven-sh"), true);
    expect_eq(std.mem.eql(u8, d7.git_repo, "bun"), true);
    expect_eq(std.mem.eql(u8, d7.git_committish, "main"), true);

    // 8. git+ssh URL
    const d8 = try DependencyVersion.parse("priv-git", "git+ssh://git@gitlab.example.com/team/core.git#v1.2.0");
    expect_eq(d8.tag == .Git, true);
    expect_eq(std.mem.eql(u8, d8.git_committish, "v1.2.0"), true);

    // 9. dist-tag
    const d9 = try DependencyVersion.parse("react", "next");
    expect_eq(d9.tag == .DistTag, true);
    expect_eq(std.mem.eql(u8, d9.dist_tag, "next"), true);

    // 10. catalog:
    const d10 = try DependencyVersion.parse("zod", "catalog:core");
    expect_eq(d10.tag == .Catalog, true);
    expect_eq(std.mem.eql(u8, d10.catalog_name, "core"), true);

    // DependencyList sorting & merging
    let dlist = DependencyList.init();
    defer dlist.deinit(allocator);
    try dlist.addSpec(allocator, "zlib-wrap", "^1.0.0", Behavior.normal());
    try dlist.addSpec(allocator, "eslint", "^9.0.0", Behavior.devOnly());
    try dlist.addSpec(allocator, "@app/ws", "workspace:*", Behavior.workspaceDep());
    dlist.sort();
    expect_eq(dlist.len(), 3);
    expect_eq(std.mem.eql(u8, dlist.slice()[0].name, "@app/ws"), true);
    expect_eq(std.mem.eql(u8, dlist.slice()[1].name, "eslint"), true);
    expect_eq(std.mem.eql(u8, dlist.slice()[2].name, "zlib-wrap"), true);
}

fn test_npm_packument_and_npmrc(): !void {
    const allocator = std.heap.page_allocator;

    // 1. .npmrc scoped registry + Bearer & Basic auth
    let reg_cfg = RegistryConfig.init();
    defer reg_cfg.deinit(allocator);

    const npmrc = "registry=https://registry.npmjs.org/\n@acme:registry=https://npm.acme.internal/\n//npm.acme.internal/:_authToken=secret-bearer-token-99\n//private.example.org/:username=alice\n//private.example.org/:_password=hunter2\n@priv:registry=https://private.example.org/\nalways-auth=true\n";
    try reg_cfg.parseNpmrc(allocator, npmrc);

    const pub_scope = reg_cfg.resolveForPackage("lodash");
    expect_eq(std.mem.eql(u8, pub_scope.registry_url, "https://registry.npmjs.org/"), true);
    expect_eq(pub_scope.hasAuth(), false);

    const acme_scope = reg_cfg.resolveForPackage("@acme/sdk");
    expect_eq(std.mem.eql(u8, acme_scope.registry_url, "https://npm.acme.internal/"), true);
    const acme_auth = (try acme_scope.authorizationHeaderAlloc(allocator)).?;
    defer allocator.free(acme_auth);
    expect_eq(std.mem.eql(u8, acme_auth, "Bearer secret-bearer-token-99"), true);

    const acme_meta_url = try acme_scope.packageMetadataUrlAlloc(allocator, "@acme/sdk");
    defer allocator.free(acme_meta_url);
    expect_eq(std.mem.eql(u8, acme_meta_url, "https://npm.acme.internal/@acme%2Fsdk"), true);

    const priv_scope = reg_cfg.resolveForPackage("@priv/core");
    const priv_auth = (try priv_scope.authorizationHeaderAlloc(allocator)).?;
    defer allocator.free(priv_auth);
    expect_eq(std.mem.startsWith(u8, priv_auth, "Basic "), true);

    // 2. Packument JSON parsing + Semver best-version resolution
    const packument_json = "{\"name\":\"tiny-math\",\"modified\":\"2026-11-20T00:00:00Z\",\"dist-tags\":{\"latest\":\"1.4.2\",\"next\":\"2.0.0-rc.1\"},\"versions\":{\"1.0.0\":{\"dist\":{\"tarball\":\"https://registry.npmjs.org/tiny-math/-/tiny-math-1.0.0.tgz\",\"shasum\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}},\"1.4.2\":{\"dependencies\":{\"fast-int\":\"^1.0.0\"},\"peerDependencies\":{\"logger\":\"^2.0.0\"},\"peerDependenciesMeta\":{\"logger\":{\"optional\":true}},\"os\":[\"linux\",\"darwin\"],\"cpu\":[\"x64\",\"arm64\"],\"bin\":{\"tmath\":\"./bin/tmath.js\"},\"dist\":{\"tarball\":\"https://registry.npmjs.org/tiny-math/-/tiny-math-1.4.2.tgz\",\"unpackedSize\":4096,\"fileCount\":5}},\"1.5.0\":{\"dist\":{\"tarball\":\"https://registry.npmjs.org/tiny-math/-/tiny-math-1.5.0.tgz\"}},\"2.0.0-rc.1\":{\"dist\":{\"tarball\":\"https://registry.npmjs.org/tiny-math/-/tiny-math-2.0.0-rc.1.tgz\"}}}}";

    let manifest = try NpmPackageManifest.parsePackument(allocator, packument_json);
    defer manifest.deinit(allocator);

    expect_eq(std.mem.eql(u8, manifest.name, "tiny-math"), true);
    expect_eq(manifest.versions.items.len, 4);

    // `^1.0.0` matches `latest` (1.4.2) first per npm/Bun policy
    const res_caret = manifest.resolveVersion("^1.0.0", false).?;
    expect_eq(std.mem.eql(u8, res_caret.version_str, "1.4.2"), true);
    expect_eq(res_caret.dependencies.items.len, 1);
    expect_eq(res_caret.peer_dependencies.items.len, 1);
    expect_eq(res_caret.peer_dependencies.items[0].optional, true);
    expect_eq(res_caret.bin.kind == .NamedFile, true);
    expect_eq(OperatingSystem.isMatch(res_caret.os_mask, OperatingSystem.LINUX), true);
    expect_eq(OperatingSystem.isMatch(res_caret.os_mask, OperatingSystem.WIN32), false);

    // `>=1.4.5 <2.0.0` skips `latest` (1.4.2) and selects highest match `1.5.0`
    const res_gt = manifest.resolveVersion(">=1.4.5 <2.0.0", false).?;
    expect_eq(std.mem.eql(u8, res_gt.version_str, "1.5.0"), true);

    // Dist-tag `next` resolves to prerelease `2.0.0-rc.1`
    const res_next = manifest.resolveVersion("next", false).?;
    expect_eq(std.mem.eql(u8, res_next.version_str, "2.0.0-rc.1"), true);
}

fn test_lockfile_graph_and_dual_persistence(): !void {
    const allocator = std.heap.page_allocator;

    let lf = Lockfile.init();
    defer lf.deinit(allocator);

    try lf.addWorkspace(allocator, "root-app", ".", "1.0.0");
    try lf.addOverride(allocator, "ms", "2.1.3");
    try lf.addTrustedDependency(allocator, "esbuild");
    try lf.addPatchedDependency(allocator, "helper@1.0.0", "patches/helper.patch", 0xDEADBEEF);

    const id_root = try lf.addPackage(allocator, "root-app", "1.0.0", Resolution.initRoot(), PackageMeta{});
    const id_a = try lf.addPackage(
        allocator,
        "pkg-a",
        "1.2.0",
        Resolution.initNpm("1.2.0", "https://registry.npmjs.org/pkg-a/-/pkg-a-1.2.0.tgz"),
        PackageMeta{ integrity: Integrity.computeSha256("pkg-a-bytes") },
    );
    const id_b = try lf.addPackage(
        allocator,
        "pkg-b",
        "2.0.0",
        Resolution.initNpm("2.0.0", "https://registry.npmjs.org/pkg-b/-/pkg-b-2.0.0.tgz"),
        PackageMeta{ integrity: Integrity.computeSha512("pkg-b-bytes") },
    );

    let bin_a = try BinLink.initNamedFile(allocator, "pkga", "bin/a.js");
    defer bin_a.deinit(allocator);
    try lf.setPackageBin(allocator, id_a, bin_a);

    try lf.addDependency(allocator, id_root, "pkg-a", "^1.2.0", Behavior.normal(), id_a);
    try lf.addDependency(allocator, id_a, "pkg-b", "^2.0.0", Behavior.normal(), id_b);
    lf.computeMetaHash();

    // Graph checks: acyclic + topological order (deps before dependents)
    expect_eq(try lf.hasCycle(allocator), false);
    const topo = try lf.topoOrder(allocator);
    defer allocator.free(topo);
    expect_eq(topo.len, 3);
    expect_eq(topo[0], id_b);
    expect_eq(topo[1], id_a);
    expect_eq(topo[2], id_root);

    // Text lockfile round-trip
    const text_json = try lf.toTextJsonAlloc(allocator);
    defer allocator.free(text_json);
    let lf_from_text = try Lockfile.fromTextJson(allocator, text_json);
    defer lf_from_text.deinit(allocator);
    expect_eq(lf_from_text.packageCount(), 3);
    expect_eq(lf_from_text.edgeCount(), 2);
    expect_eq(lf_from_text.overrides.items.len, 1);
    expect_eq(lf_from_text.trusted_dependencies.items.len, 1);
    expect_eq(lf_from_text.patched_dependencies.items.len, 1);
    expect_eq(lf_from_text.getPackage(id_a).?.bin.kind == .NamedFile, true);

    // Binary CPLK0001 round-trip
    const bin_bytes = try lf.toBinaryAlloc(allocator);
    defer allocator.free(bin_bytes);
    expect_eq(std.mem.eql(u8, bin_bytes[0..8], BINARY_LOCKFILE_MAGIC), true);

    let lf_from_bin = try Lockfile.fromBinary(allocator, bin_bytes);
    defer lf_from_bin.deinit(allocator);
    expect_eq(lf_from_bin.packageCount(), 3);
    expect_eq(lf_from_bin.edgeCount(), 2);
    expect_eq(lf_from_bin.getPackage(id_b).?.meta.integrity.verify("pkg-b-bytes"), true);

    // Binary corruption detection (flip last byte -> CRC32 mismatch)
    let corrupt_bytes = try allocator.dupe(u8, bin_bytes);
    defer allocator.free(corrupt_bytes);
    corrupt_bytes[corrupt_bytes.len - 1] ^= 0xFF;
    let corrupt_failed = false;
    * = Lockfile.fromBinary(allocator, corrupt_bytes) catch {
        corrupt_failed = true;
    };
    expect_eq(corrupt_failed, true);

    // Cycle detection
    try lf.addDependency(allocator, id_b, "pkg-a", "^1.0.0", Behavior.normal(), id_a);
    expect_eq(try lf.hasCycle(allocator), true);
    let cycle_err = false;
    const bad_topo = lf.topoOrder(allocator) catch {
        cycle_err = true;
        return;
    };
    allocator.free(bad_topo);
    expect_eq(cycle_err, true);
}

fn test_tarball_sri_and_traversal_defense(): !void {
    const allocator = std.heap.page_allocator;

    let tmp_buf: [512]u8 = undefined;
    const tmp_root = try makeTempDir("cp_pkg_tgz", &tmp_buf);
    defer rmdirRecursive(tmp_root) catch {};

    const pkg_json_str = "{\"name\":\"demo-tgz\",\"version\":\"1.0.0\"}";
    const index_js_str = "export const hello = 42;\n";
    const cli_js_str = "#!/usr/bin/env node\nconsole.log('hi');\n";

    const entries: [3]TarEntryInput = [
        TarEntryInput{ name: "package/package.json", data: pkg_json_str, mode: 0o644 },
        TarEntryInput{ name: "package/index.js", data: index_js_str, mode: 0o644 },
        TarEntryInput{ name: "package/bin/cli.js", data: cli_js_str, mode: 0o755 },
    ];

    const tgz_bytes = try TarballBuilder.buildTgzAlloc(allocator, &entries);
    defer allocator.free(tgz_bytes);
    expect_eq(tgz_bytes[0], GZIP_MAGIC_ID1);
    expect_eq(tgz_bytes[1], GZIP_MAGIC_ID2);
    const sri = TarballBuilder.computeSha512(tgz_bytes);

    const tgz_path = try std.fmt.allocPrint(allocator, "{s}/demo.tgz", .{tmp_root});
    defer allocator.free(tgz_path);
    try writeFile(tgz_path, tgz_bytes, 0o644);

    const out_dir = try std.fmt.allocPrint(allocator, "{s}/extracted", .{tmp_root});
    defer allocator.free(out_dir);

    const stats = try TarballExtractor.extractFileToDir(allocator, tgz_path, out_dir, TarballExtractOptions{
        strip_components: 1,
        verify_integrity: true,
        expected_integrity: sri,
    });
    expect_eq(stats.files_extracted, 3);
    expect_eq(stats.computed_integrity.eql(sri), true);

    const ext_idx_path = try std.fmt.allocPrint(allocator, "{s}/index.js", .{out_dir});
    defer allocator.free(ext_idx_path);
    const read_idx = try readFile(allocator, ext_idx_path);
    defer allocator.free(read_idx);
    expect_eq(std.mem.eql(u8, read_idx, index_js_str), true);

    const ext_cli_path = try std.fmt.allocPrint(allocator, "{s}/bin/cli.js", .{out_dir});
    defer allocator.free(ext_cli_path);
    const cli_st = try stat(ext_cli_path);
    expect_eq((cli_st.permissions() & 0o100) != 0, true);

    // Tampered tarball must fail SRI verification
    let bad_tgz = try allocator.dupe(u8, tgz_bytes);
    defer allocator.free(bad_tgz);
    bad_tgz[bad_tgz.len / 2] ^= 0x01;
    let sri_failed = false;
    * = TarballExtractor.extractBytesToDir(allocator, bad_tgz, out_dir, TarballExtractOptions{
        verify_integrity: true,
        expected_integrity: sri,
    }) catch {
        sri_failed = true;
    };
    expect_eq(sri_failed, true);

    // Path traversal defense: `..` escape must be rejected
    const evil_entries: [1]TarEntryInput = [
        TarEntryInput{ name: "package/../../evil.txt", data: "nope", mode: 0o644 },
    ];
    const evil_tar = try TarballBuilder.buildTarAlloc(allocator, &evil_entries);
    defer allocator.free(evil_tar);

    let traversal_blocked = false;
    * = TarballExtractor.extractTarBytesToDir(allocator, evil_tar, out_dir, TarballExtractOptions{}) catch {
        traversal_blocked = true;
    };
    expect_eq(traversal_blocked, true);

    // Absolute path defense
    expect_eq(TarballExtractor.isSafeArchivePath("/etc/passwd"), false);
    expect_eq(TarballExtractor.isSafeArchivePath("a/../b"), false);
    expect_eq(TarballExtractor.isSafeSymlinkTarget("bin/tool", "../../escape"), false);
    expect_eq(TarballExtractor.isSafeSymlinkTarget("bin/tool", "../lib/main.js"), true);
}

fn test_e2e_package_manager_hoisted_and_isolated(): !void {
    const allocator = std.heap.page_allocator;

    // Build real tarballs for `app-lib@1.2.0` (with bin + dep on `helper-util`) and `helper-util@2.3.0`
    const app_entries: [2]TarEntryInput = [
        TarEntryInput{
            name: "package/package.json",
            data: "{\"name\":\"app-lib\",\"version\":\"1.2.0\",\"bin\":{\"apputil\":\"./bin/cli.js\"}}",
            mode: 0o644,
        },
        TarEntryInput{
            name: "package/bin/cli.js",
            data: "#!/usr/bin/env node\nconsole.log('apputil ok');\n",
            mode: 0o644,
        },
    ];
    const app_tgz = try TarballBuilder.buildTgzAlloc(allocator, &app_entries);
    defer allocator.free(app_tgz);
    const app_sri = TarballBuilder.computeSha512(app_tgz);

    const helper_entries: [2]TarEntryInput = [
        TarEntryInput{
            name: "package/package.json",
            data: "{\"name\":\"helper-util\",\"version\":\"2.3.0\"}",
            mode: 0o644,
        },
        TarEntryInput{
            name: "package/index.js",
            data: "export const val = 100;\n",
            mode: 0o644,
        },
    ];
    const helper_tgz = try TarballBuilder.buildTgzAlloc(allocator, &helper_entries);
    defer allocator.free(helper_tgz);
    const helper_sri = TarballBuilder.computeSha512(helper_tgz);

    const app_packument = "{\"name\":\"app-lib\",\"dist-tags\":{\"latest\":\"1.2.0\"},\"versions\":{\"1.2.0\":{\"dependencies\":{\"helper-util\":\"^2.0.0\"},\"bin\":{\"apputil\":\"./bin/cli.js\"},\"dist\":{\"tarball\":\"https://registry.npmjs.org/app-lib/-/app-lib-1.2.0.tgz\"}}}}";
    const helper_packument = "{\"name\":\"helper-util\",\"dist-tags\":{\"latest\":\"2.3.0\"},\"versions\":{\"2.1.0\":{\"dist\":{\"tarball\":\"https://registry.npmjs.org/helper-util/-/helper-util-2.1.0.tgz\"}},\"2.3.0\":{\"dist\":{\"tarball\":\"https://registry.npmjs.org/helper-util/-/helper-util-2.3.0.tgz\"}}}}";

    const helper_patch = "--- a/index.js\n+++ b/index.js\n@@ -1 +1 @@\n-export const val = 100;\n+export const val = 999;\n";

    // --------------------------------------------------------------------
    // 1. End-to-End Hoisted Install + Cache Reuse + Patch + .bin Link
    // --------------------------------------------------------------------
    let hoisted_tmp_buf: [512]u8 = undefined;
    const hoisted_dir = try makeTempDir("cp_pkg_hoisted", &hoisted_tmp_buf);
    defer rmdirRecursive(hoisted_dir) catch {};

    const cache_dir = try std.fmt.allocPrint(allocator, "{s}/.cache", .{hoisted_dir});
    defer allocator.free(cache_dir);
    const hoisted_pj = try std.fmt.allocPrint(allocator, "{s}/package.json", .{hoisted_dir});
    defer allocator.free(hoisted_pj);
    try writeFile(hoisted_pj, "{\"name\":\"demo-app\",\"version\":\"1.0.0\",\"dependencies\":{\"app-lib\":\"^1.0.0\"}}", 0o644);

    let pm_h = PackageManager.init(allocator, PackageManagerConfig{
        project_dir: hoisted_dir,
        cache_dir: cache_dir,
        strategy: .Hoisted,
        copy_method: .Auto,
        save_text_lockfile: true,
        save_binary_lockfile: true,
    });
    defer pm_h.deinit();

    try pm_h.registerPackument("app-lib", app_packument);
    try pm_h.registerPackument("helper-util", helper_packument);
    try pm_h.registerTarballBytes("app-lib", "1.2.0", app_tgz, app_sri, BinLink{});
    try pm_h.registerTarballBytes("helper-util", "2.3.0", helper_tgz, helper_sri, BinLink{});
    try pm_h.registerPatch("helper-util@2.3.0", "patches/helper-util@2.3.0.patch", helper_patch);

    const plan1 = try pm_h.planInstall();
    defer {
        let p = plan1;
        p.deinit(allocator);
    }
    expect_eq(plan1.total_dependencies, 1);
    expect_eq(plan1.extracts_needed, 1);

    const rep1 = try pm_h.install();
    expect_eq(rep1.strategy == .Hoisted, true);
    expect_eq(rep1.installed_packages, 2);
    expect_eq(rep1.tarballs_extracted, 2);
    expect_eq(rep1.cache_hits, 0);
    expect_eq(rep1.bins_linked, 1);
    expect_eq(rep1.patches_applied, 1);
    expect_eq(rep1.lockfile_written, true);
    expect_eq(rep1.lockfile_packages, 3);
    expect_eq(rep1.has_cycle, false);

    // Verify hoisted files, patched content, .bin symlink & 0o755 executable permission, and lockfiles
    const h_helper_idx = try std.fmt.allocPrint(allocator, "{s}/node_modules/helper-util/index.js", .{hoisted_dir});
    defer allocator.free(h_helper_idx);
    const patched_content = try readFile(allocator, h_helper_idx);
    defer allocator.free(patched_content);
    expect_eq(std.mem.eql(u8, patched_content, "export const val = 999;\n"), true);

    const h_bin_link = try std.fmt.allocPrint(allocator, "{s}/node_modules/.bin/apputil", .{hoisted_dir});
    defer allocator.free(h_bin_link);
    expect_eq(exists(h_bin_link), true);
    let rl_buf: [512]u8 = undefined;
    const rl_target = try readlink(h_bin_link, &rl_buf);
    const bin_st = try stat(rl_target);
    expect_eq((bin_st.permissions() & 0o111) != 0, true);

    const h_lock_txt = try std.fmt.allocPrint(allocator, "{s}/cap.lock", .{hoisted_dir});
    defer allocator.free(h_lock_txt);
    const h_lock_bin = try std.fmt.allocPrint(allocator, "{s}/cap.lockb", .{hoisted_dir});
    defer allocator.free(h_lock_bin);
    expect_eq(exists(h_lock_txt), true);
    expect_eq(exists(h_lock_bin), true);

    // Second install should achieve 100% cache hit
    const rep2 = try pm_h.install();
    expect_eq(rep2.cache_hits, 2);
    expect_eq(rep2.tarballs_extracted, 0);

    // --------------------------------------------------------------------
    // 2. End-to-End Isolated Install (pnpm/Bun `.cap/` store + transitive symlinks)
    // --------------------------------------------------------------------
    let iso_tmp_buf: [512]u8 = undefined;
    const iso_dir = try makeTempDir("cp_pkg_isolated", &iso_tmp_buf);
    defer rmdirRecursive(iso_dir) catch {};

    const iso_pj = try std.fmt.allocPrint(allocator, "{s}/package.json", .{iso_dir});
    defer allocator.free(iso_pj);
    try writeFile(iso_pj, "{\"name\":\"iso-app\",\"version\":\"1.0.0\",\"dependencies\":{\"app-lib\":\"^1.0.0\"}}", 0o644);

    let pm_iso = PackageManager.init(allocator, PackageManagerConfig{
        project_dir: iso_dir,
        cache_dir: cache_dir,
        strategy: .Isolated,
        copy_method: .Auto,
        save_text_lockfile: true,
    });
    defer pm_iso.deinit();

    try pm_iso.registerPackument("app-lib", app_packument);
    try pm_iso.registerPackument("helper-util", helper_packument);
    try pm_iso.registerTarballBytes("app-lib", "1.2.0", app_tgz, app_sri, BinLink{});
    try pm_iso.registerTarballBytes("helper-util", "2.3.0", helper_tgz, helper_sri, BinLink{});
    try pm_iso.registerPatch("helper-util@2.3.0", "patches/helper-util@2.3.0.patch", helper_patch);

    const rep_iso = try pm_iso.install();
    expect_eq(rep_iso.strategy == .Isolated, true);
    expect_eq(rep_iso.installed_packages, 2);
    expect_eq(rep_iso.cache_hits, 2); // Reuses cache populated above!
    expect_eq(rep_iso.bins_linked, 1);
    expect_eq(rep_iso.patches_applied, 1);

    // Direct dep `app-lib` is symlinked at `node_modules/app-lib`
    const iso_direct = try std.fmt.allocPrint(allocator, "{s}/node_modules/app-lib/package.json", .{iso_dir});
    defer allocator.free(iso_direct);
    expect_eq(exists(iso_direct), true);

    // Transitive dep `helper-util` is isolated inside `.cap/app-lib@1.2.0/node_modules/helper-util`
    const iso_trans = try std.fmt.allocPrint(allocator, "{s}/node_modules/.cap/app-lib@1.2.0/node_modules/helper-util/index.js", .{iso_dir});
    defer allocator.free(iso_trans);
    expect_eq(exists(iso_trans), true);
    const iso_patched = try readFile(allocator, iso_trans);
    defer allocator.free(iso_patched);
    expect_eq(std.mem.eql(u8, iso_patched, "export const val = 999;\n"), true);

    // Isolated .bin link works
    const iso_bin = try std.fmt.allocPrint(allocator, "{s}/node_modules/.bin/apputil", .{iso_dir});
    defer allocator.free(iso_bin);
    expect_eq(exists(iso_bin), true);
}

fn main() {
    test("cap:pkg core types, strings, slices, behaviors & cap hub", () => {
        test_types_and_cap_hub() catch unreachable;
    });

    test("cap:pkg DependencyVersion.parse all protocols & DependencyList", () => {
        test_dependency_all_protocols() catch unreachable;
    });

    test("cap:pkg RegistryConfig (.npmrc), Auth & NpmPackageManifest Packument", () => {
        test_npm_packument_and_npmrc() catch unreachable;
    });

    test("cap:pkg Lockfile graph, cycle detection, topoOrder & text/binary CPLK0001 round-trip", () => {
        test_lockfile_graph_and_dual_persistence() catch unreachable;
    });

    test("cap:pkg TarballBuilder -> gzip -> SRI verify -> safe extract & path traversal defense", () => {
        test_tarball_sri_and_traversal_defense() catch unreachable;
    });

    test("cap:pkg E2E PackageManager Hoisted & Isolated install + .bin + patch_install", () => {
        test_e2e_package_manager_hoisted_and_isolated() catch unreachable;
    });
}
