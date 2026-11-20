// cap/test/test_phase_b3_pkg_production.cp
// Phase B-3 Deep End-to-End Verification of cap:pkg Production Subsystems:
//   1. Monorepo Workspaces (cap:glob + cap:sys scan, workspace:*/^/~, catalog:, DAG topoSortMembers & cycle detection)
//   2. RegistryHttpClient (scoped Bearer/Basic auth, Accept header, ETag/304 disk cache hit, RetryPolicy 429/503 + Retry-After, Packument parse)
//   3. LifecycleRunner (zero-trust trustedDependencies block + real PosixSpawn postinstall execution, env vars, stdout/stderr capture & artifact creation)
//   4. LockfileMigrator (package-lock.json v3, yarn.lock v1, pnpm-lock.yaml v9 via cap:conf YAML 1.2) + DependencyAnalyzer (why, findDuplicates, auditIntegrity)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { GZIP_MAGIC_ID1 } from "cap:pack";
import {
    makeTempDir,
    rmdirRecursive,
    mkdirAll,
    writeFile,
    readFile,
    exists,
} from "cap:sys";
import {
    RegistryConfig,
    Lockfile,
    CatalogMap,
    WorkspaceGraph,
    RegistryCacheEntry,
    RetryPolicy,
    MockRegistryTransport,
    RegistryHttpClient,
    LifecyclePhase,
    PackageScripts,
    TrustedPolicy,
    LifecycleRunner,
    ExternalLockfileFormat,
    LockfileMigrator,
    DependencyAnalyzer,
} from "cap:pkg";

fn test_monorepo_workspaces_and_catalogs(): !void {
    const allocator = std.heap.page_allocator;

    let tmp_buf: [512]u8 = undefined;
    const root_dir = try makeTempDir("cp_pkg_ws", &tmp_buf);
    defer rmdirRecursive(root_dir) catch {};

    // Create monorepo layout:
    //   package.json: workspaces { packages: ["packages/*", "!packages/ignored"], catalog: { zod: "^3.23.0" }, catalogs: { ui: { react: "^18.3.0" } } }
    //   packages/core/package.json (name: "@acme/core", version: "1.2.0", dep: zod: "catalog:")
    //   packages/utils/cap.json (name: "@acme/utils", version: "2.1.0", dep: @acme/core: "workspace:^", react: "catalog:ui")
    //   packages/web/package.json (name: "@acme/web", version: "0.5.0", dep: @acme/utils: "workspace:*", @acme/core: "workspace:~")
    //   packages/ignored/package.json (excluded by !packages/ignored)
    const root_pj = "{\"name\":\"acme-monorepo\",\"version\":\"1.0.0\",\"workspaces\":{\"packages\":[\"packages/*\",\"!packages/ignored\"],\"catalog\":{\"zod\":\"^3.23.0\"},\"catalogs\":{\"ui\":{\"react\":\"^18.3.0\"}}}}";
    const p_root = try std.fmt.allocPrint(allocator, "{s}/package.json", .{root_dir});
    defer allocator.free(p_root);
    try writeFile(p_root, root_pj, 0o644);

    const d_core = try std.fmt.allocPrint(allocator, "{s}/packages/core", .{root_dir});
    defer allocator.free(d_core);
    const d_utils = try std.fmt.allocPrint(allocator, "{s}/packages/utils", .{root_dir});
    defer allocator.free(d_utils);
    const d_web = try std.fmt.allocPrint(allocator, "{s}/packages/web", .{root_dir});
    defer allocator.free(d_web);
    const d_ign = try std.fmt.allocPrint(allocator, "{s}/packages/ignored", .{root_dir});
    defer allocator.free(d_ign);

    try mkdirAll(d_core, 0o755);
    try mkdirAll(d_utils, 0o755);
    try mkdirAll(d_web, 0o755);
    try mkdirAll(d_ign, 0o755);

    const f_core = try std.fmt.allocPrint(allocator, "{s}/package.json", .{d_core});
    defer allocator.free(f_core);
    try writeFile(f_core, "{\"name\":\"@acme/core\",\"version\":\"1.2.0\",\"dependencies\":{\"zod\":\"catalog:\"}}", 0o644);

    const f_utils = try std.fmt.allocPrint(allocator, "{s}/cap.json", .{d_utils});
    defer allocator.free(f_utils);
    try writeFile(f_utils, "{\"name\":\"@acme/utils\",\"version\":\"2.1.0\",\"dependencies\":{\"@acme/core\":\"workspace:^\",\"react\":\"catalog:ui\"}}", 0o644);

    const f_web = try std.fmt.allocPrint(allocator, "{s}/package.json", .{d_web});
    defer allocator.free(f_web);
    try writeFile(f_web, "{\"name\":\"@acme/web\",\"version\":\"0.5.0\",\"dependencies\":{\"@acme/utils\":\"workspace:*\",\"@acme/core\":\"workspace:~\"}}", 0o644);

    const f_ign = try std.fmt.allocPrint(allocator, "{s}/package.json", .{d_ign});
    defer allocator.free(f_ign);
    try writeFile(f_ign, "{\"name\":\"@acme/ignored\",\"version\":\"9.9.9\"}", 0o644);

    let ws = try WorkspaceGraph.discover(allocator, root_dir);
    defer ws.deinit(allocator);

    // Excluded package must not be discovered
    expect_eq(ws.memberCount(), 3);
    expect_eq(ws.findMemberByName("@acme/ignored") == null, true);

    const m_core = ws.findMemberByName("@acme/core").?;
    const m_utils = ws.findMemberByName("@acme/utils").?;
    const m_web = ws.findMemberByName("@acme/web").?;

    expect_eq(m_utils.manifest_kind == .CapJson, true);
    expect_eq(std.mem.eql(u8, m_core.edges.items[0].resolved_spec, "^3.23.0"), true);
    expect_eq(m_core.edges.items[0].is_catalog, true);

    // Verify workspace:^, workspace:*, workspace:~ and catalog:ui resolution
    let saw_core_caret = false;
    let saw_react_cat = false;
    for (e of m_utils.edges.items) {
        if (std.mem.eql(u8, e.name, "@acme/core")) {
            expect_eq(std.mem.eql(u8, e.resolved_spec, "^1.2.0"), true);
            expect_eq(e.is_workspace_protocol, true);
            saw_core_caret = true;
        }
        if (std.mem.eql(u8, e.name, "react")) {
            expect_eq(std.mem.eql(u8, e.resolved_spec, "^18.3.0"), true);
            expect_eq(e.is_catalog, true);
            saw_react_cat = true;
        }
    }
    expect_eq(saw_core_caret, true);
    expect_eq(saw_react_cat, true);

    let saw_utils_star = false;
    let saw_core_tilde = false;
    for (e of m_web.edges.items) {
        if (std.mem.eql(u8, e.name, "@acme/utils")) {
            expect_eq(std.mem.eql(u8, e.resolved_spec, "2.1.0"), true);
            saw_utils_star = true;
        }
        if (std.mem.eql(u8, e.name, "@acme/core")) {
            expect_eq(std.mem.eql(u8, e.resolved_spec, "~1.2.0"), true);
            saw_core_tilde = true;
        }
    }
    expect_eq(saw_utils_star, true);
    expect_eq(saw_core_tilde, true);

    // Topological sort: @acme/core MUST precede @acme/utils, and @acme/utils MUST precede @acme/web
    expect_eq(try ws.hasCycle(allocator), false);
    const topo_order = try ws.topoSortMembers(allocator);
    defer allocator.free(topo_order);
    expect_eq(topo_order.len, 3);
    expect_eq(topo_order[0], m_core.index);
    expect_eq(topo_order[1], m_utils.index);
    expect_eq(topo_order[2], m_web.index);

    // Materialize into Lockfile
    let lf = Lockfile.init();
    defer lf.deinit(allocator);
    try ws.applyToLockfile(allocator, &lf);
    expect_eq(lf.workspaces.items.len, 3);
    expect_eq(lf.packageCount(), 3);
    expect_eq(try lf.hasCycle(allocator), false);

    // Cycle detection test: introduce @acme/core -> @acme/web
    try writeFile(f_core, "{\"name\":\"@acme/core\",\"version\":\"1.2.0\",\"dependencies\":{\"@acme/web\":\"workspace:*\"}}", 0o644);
    let ws_cyc = try WorkspaceGraph.discover(allocator, root_dir);
    defer ws_cyc.deinit(allocator);
    expect_eq(try ws_cyc.hasCycle(allocator), true);
    let cyc_err = false;
    const bad = ws_cyc.topoSortMembers(allocator) catch {
        cyc_err = true;
        return;
    };
    allocator.free(bad);
    expect_eq(cyc_err, true);
}

fn test_registry_http_client_cache_etag_and_retry(): !void {
    const allocator = std.heap.page_allocator;

    let tmp_buf: [512]u8 = undefined;
    const cache_dir = try makeTempDir("cp_pkg_reg", &tmp_buf);
    defer rmdirRecursive(cache_dir) catch {};

    let reg_cfg = RegistryConfig.init();
    defer reg_cfg.deinit(allocator);
    const npmrc = "registry=https://registry.npmjs.org/\n@acme:registry=https://npm.acme.internal/\n//npm.acme.internal/:_authToken=tok-acme-777\n";
    try reg_cfg.parseNpmrc(allocator, npmrc);

    let mock = MockRegistryTransport.init();
    defer mock.deinit(allocator);

    let client = RegistryHttpClient.init(allocator, &reg_cfg, cache_dir);
    defer client.deinit();
    client.setMockTransport(&mock);
    client.setRetryPolicy(RetryPolicy{
        max_retries: 3,
        base_delay_ms: 100,
        max_delay_ms: 5000,
        respect_retry_after: true,
    });

    // Verify RetryPolicy backoff math (429 Retry-After vs exponential backoff)
    expect_eq(RetryPolicy.isRetryableStatus(429), true);
    expect_eq(RetryPolicy.isRetryableStatus(503), true);
    expect_eq(RetryPolicy.isRetryableStatus(404), false);
    expect_eq(client.retry_policy.computeDelayMs(0, "2"), 2000);
    expect_eq(client.retry_policy.computeDelayMs(0, null), 100);
    expect_eq(client.retry_policy.computeDelayMs(1, null), 200);
    expect_eq(client.retry_policy.computeDelayMs(2, null), 400);

    const packument_body = "{\"name\":\"@acme/sdk\",\"dist-tags\":{\"latest\":\"1.5.0\"},\"versions\":{\"1.5.0\":{\"dist\":{\"tarball\":\"https://npm.acme.internal/@acme/sdk/-/sdk-1.5.0.tgz\"}}}}";

    // Attempt 1: 429 Too Many Requests with Retry-After: 1
    try mock.pushResponse(allocator, "@acme%2Fsdk", 429, null, null, "1", "rate limited", false);
    // Attempt 2: 503 Service Unavailable (exponential backoff attempt=1 -> 200ms)
    try mock.pushResponse(allocator, "@acme%2Fsdk", 503, null, null, null, "unavailable", false);
    // Attempt 3: 200 OK with ETag + Cache-Control: max-age=60
    try mock.pushResponse(allocator, "@acme%2Fsdk", 200, "\"etag-sdk-v1\"", "max-age=60", null, packument_body, false);
    // Attempt 4 (after TTL expiry): 304 Not Modified
    try mock.pushResponse(allocator, "@acme%2Fsdk", 304, "\"etag-sdk-v1\"", "max-age=120", null, "", false);

    // Fetch #1 at t=1000: retries 429 (1000ms) + 503 (200ms) -> succeeds 200 OK
    let res1 = try client.fetchPackument("@acme/sdk", 1000, false, false);
    defer res1.deinit(allocator);
    expect_eq(res1.status, 200);
    expect_eq(res1.from_cache, false);
    expect_eq(res1.attempts, 3);
    expect_eq(res1.total_backoff_ms, 1200);
    expect_eq(std.mem.eql(u8, res1.etag, "\"etag-sdk-v1\""), true);
    expect_eq(std.mem.eql(u8, res1.manifest.resolveVersion("^1.0.0", false).?.version_str, "1.5.0"), true);

    // Inspect recorded wire HTTP request headers (Accept + Authorization: Bearer tok-acme-777)
    expect_eq(mock.recorded_requests.items.len, 3);
    const req1_wire = mock.recorded_requests.items[0].wire_request;
    expect_eq(std.mem.indexOf(u8, req1_wire, "Accept: application/vnd.npm.install-v1+json") != null, true);
    expect_eq(std.mem.indexOf(u8, req1_wire, "Authorization: Bearer tok-acme-777") != null, true);

    // Fetch #2 at t=1020 (within max-age=60 TTL): fresh cache hit, zero network requests!
    let res2 = try client.fetchPackument("@acme/sdk", 1020, false, false);
    defer res2.deinit(allocator);
    expect_eq(res2.from_cache, true);
    expect_eq(res2.attempts, 0);
    expect_eq(mock.recorded_requests.items.len, 3);

    // Fetch #3 at t=1100 using a brand-new client instance (forces disk cache load + TTL expired -> sends If-None-Match -> 304!)
    let client2 = RegistryHttpClient.init(allocator, &reg_cfg, cache_dir);
    defer client2.deinit();
    client2.setMockTransport(&mock);

    let res3 = try client2.fetchPackument("@acme/sdk", 1100, false, false);
    defer res3.deinit(allocator);
    expect_eq(res3.status, 304);
    expect_eq(res3.from_cache, true);
    expect_eq(res3.not_modified_304, true);
    expect_eq(std.mem.eql(u8, res3.manifest.resolveVersion("^1.0.0", false).?.version_str, "1.5.0"), true);

    // Verify conditional If-None-Match header was sent on wire
    expect_eq(mock.recorded_requests.items.len, 4);
    const req4_wire = mock.recorded_requests.items[3].wire_request;
    expect_eq(std.mem.indexOf(u8, req4_wire, "If-None-Match: \"etag-sdk-v1\"") != null, true);
}

fn test_lifecycle_runner_zero_trust_and_execution(): !void {
    const allocator = std.heap.page_allocator;

    let tmp_buf: [512]u8 = undefined;
    const tmp_root = try makeTempDir("cp_pkg_life", &tmp_buf);
    defer rmdirRecursive(tmp_root) catch {};

    const pkg_dir = try std.fmt.allocPrint(allocator, "{s}/node_modules/native-addon", .{tmp_root});
    defer allocator.free(pkg_dir);
    const bin_dir = try std.fmt.allocPrint(allocator, "{s}/node_modules/.bin", .{tmp_root});
    defer allocator.free(bin_dir);
    try mkdirAll(pkg_dir, 0o755);
    try mkdirAll(bin_dir, 0o755);

    const pj_path = try std.fmt.allocPrint(allocator, "{s}/package.json", .{pkg_dir});
    defer allocator.free(pj_path);
    try writeFile(pj_path, "{\"name\":\"native-addon\",\"version\":\"2.4.0\"}", 0o644);

    let runner = LifecycleRunner.init(allocator, tmp_root, bin_dir);
    defer runner.deinit();

    // 1. Zero-Trust default: untrusted package must be blocked & counted in blocked_scripts!
    let blocked_res = try runner.runScript(
        "untrusted-evil",
        "1.0.0",
        pkg_dir,
        LifecyclePhase.Postinstall,
        "echo SHOULD_NOT_RUN > blocked.txt",
    );
    defer blocked_res.deinit(allocator);
    expect_eq(blocked_res.allowed, false);
    expect_eq(blocked_res.executed, false);
    expect_eq(runner.stats.blocked_scripts, 1);

    const blocked_file = try std.fmt.allocPrint(allocator, "{s}/blocked.txt", .{pkg_dir});
    defer allocator.free(blocked_file);
    expect_eq(exists(blocked_file), false);

    // 2. Trusted package: executes real postinstall script in pkg_dir with npm_* env vars & PATH prepended!
    try runner.addTrusted("native-addon");

    const script_cmd = "printf '%s|%s|%s|%s|%s' \"$npm_package_name\" \"$npm_package_version\" \"$npm_lifecycle_event\" \"$INIT_CWD\" \"$PATH\" > env_dump.txt && printf 'build_ok'";
    let exec_res = try runner.runScript(
        "native-addon",
        "2.4.0",
        pkg_dir,
        LifecyclePhase.Postinstall,
        script_cmd,
    );
    defer exec_res.deinit(allocator);

    expect_eq(exec_res.allowed, true);
    expect_eq(exec_res.executed, true);
    expect_eq(exec_res.ok(), true);
    expect_eq(exec_res.exit_code, 0);
    expect_eq(std.mem.eql(u8, exec_res.stdout, "build_ok"), true);
    expect_eq(runner.stats.executed_scripts, 1);
    expect_eq(runner.stats.succeeded_scripts, 1);
    expect_eq(runner.stats.blocked_scripts, 1);

    const dump_path = try std.fmt.allocPrint(allocator, "{s}/env_dump.txt", .{pkg_dir});
    defer allocator.free(dump_path);
    expect_eq(exists(dump_path), true);
    const dump_text = try readFile(allocator, dump_path);
    defer allocator.free(dump_text);

    expect_eq(std.mem.indexOf(u8, dump_text, "native-addon|2.4.0|postinstall|") != null, true);
    expect_eq(std.mem.indexOf(u8, dump_text, tmp_root) != null, true);
    expect_eq(std.mem.indexOf(u8, dump_text, bin_dir) != null, true);
}

fn test_lockfile_migrations_and_dependency_analyzer(): !void {
    const allocator = std.heap.page_allocator;

    // 1. Migrate npm package-lock.json (v3) -> Lockfile + DependencyAnalyzer (why / findDuplicates / auditIntegrity)
    const npm_lock_json = "{\"name\":\"web-app\",\"version\":\"1.0.0\",\"lockfileVersion\":3,\"packages\":{\"\":{\"name\":\"web-app\",\"version\":\"1.0.0\",\"dependencies\":{\"express\":\"^4.18.0\",\"legacy\":\"^1.0.0\"}},\"node_modules/express\":{\"version\":\"4.18.2\",\"resolved\":\"https://registry.npmjs.org/express/-/express-4.18.2.tgz\",\"integrity\":\"sha512-5/PsL6iGPdfQ/lKM1UuielYgv3BUoJfz1aUwU9vHZ+J7gyvwdQXFEBIEIaxeGf0GIcreATNyBExtalisDbuMqQ==\",\"dependencies\":{\"ms\":\"2.1.2\"}},\"node_modules/ms\":{\"version\":\"2.1.2\",\"resolved\":\"https://registry.npmjs.org/ms/-/ms-2.1.2.tgz\",\"integrity\":\"sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=\"},\"node_modules/legacy\":{\"version\":\"1.0.0\",\"resolved\":\"https://registry.npmjs.org/legacy/-/legacy-1.0.0.tgz\",\"integrity\":\"sha1-2jmj7l5rSw0yVb/vlWAYkK/YBwk=\",\"dependencies\":{\"ms\":\"2.0.0\"}},\"node_modules/legacy/node_modules/ms\":{\"version\":\"2.0.0\",\"resolved\":\"https://registry.npmjs.org/ms/-/ms-2.0.0.tgz\"}}}";

    let lf_npm = try LockfileMigrator.fromNpmPackageLockJson(allocator, npm_lock_json);
    defer lf_npm.deinit(allocator);
    expect_eq(lf_npm.packageCount(), 5);
    expect_eq(try lf_npm.hasCycle(allocator), false);

    // DependencyAnalyzer.why(ms): should find 2 paths (via express@4.18.2 and legacy@1.0.0)
    let why_ms = try DependencyAnalyzer.why(allocator, &lf_npm, "ms");
    defer why_ms.deinit(allocator);
    expect_eq(why_ms.isInstalled(), true);
    expect_eq(why_ms.paths.items.len, 2);
    expect_eq(why_ms.matched_versions.items.len, 2);

    const path0_str = try why_ms.paths.items[0].formatAlloc(allocator);
    defer allocator.free(path0_str);
    expect_eq(std.mem.indexOf(u8, path0_str, "web-app@1.0.0 -> ") != null, true);
    expect_eq(std.mem.indexOf(u8, path0_str, "ms@") != null, true);

    // DependencyAnalyzer.findDuplicates: `ms` installed twice (2.1.2 and 2.0.0)
    let dups = try DependencyAnalyzer.findDuplicates(allocator, &lf_npm);
    defer {
        for (*d of dups.items) d.deinit(allocator);
        dups.deinit(allocator);
    }
    expect_eq(dups.items.len, 1);
    expect_eq(std.mem.eql(u8, dups.items[0].name, "ms"), true);
    expect_eq(dups.items[0].versions.items.len, 2);

    // DependencyAnalyzer.auditIntegrity: legacy@1.0.0 has weak sha1, nested ms@2.0.0 is missing integrity
    let audit = try DependencyAnalyzer.auditIntegrity(allocator, &lf_npm);
    defer audit.deinit(allocator);
    expect_eq(audit.isClean(), false);
    expect_eq(audit.strong_count, 2);
    expect_eq(audit.weak_sha1_count, 1);
    expect_eq(audit.missing_count, 1);

    // 2. Migrate Yarn v1 yarn.lock -> Lockfile
    const yarn_lock = "# yarn lockfile v1\n\n\"@acme/cli@^1.0.0\":\n  version \"1.3.0\"\n  resolved \"https://registry.yarnpkg.com/@acme/cli/-/cli-1.3.0.tgz\"\n  integrity sha512-5/PsL6iGPdfQ/lKM1UuielYgv3BUoJfz1aUwU9vHZ+J7gyvwdQXFEBIEIaxeGf0GIcreATNyBExtalisDbuMqQ==\n  dependencies:\n    kleur \"^4.1.5\"\n\nkleur@^4.1.5:\n  version \"4.1.5\"\n  resolved \"https://registry.yarnpkg.com/kleur/-/kleur-4.1.5.tgz\"\n  integrity sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=\n";
    const yarn_pj = "{\"name\":\"yarn-proj\",\"version\":\"0.1.0\",\"dependencies\":{\"@acme/cli\":\"^1.0.0\"}}";

    let lf_yarn = try LockfileMigrator.fromYarnLock(allocator, yarn_lock, yarn_pj);
    defer lf_yarn.deinit(allocator);
    expect_eq(lf_yarn.packageCount(), 3);
    expect_eq(lf_yarn.findPackageByName("@acme/cli") != null, true);
    expect_eq(lf_yarn.findPackageByName("kleur") != null, true);

    let why_kleur = try DependencyAnalyzer.why(allocator, &lf_yarn, "kleur");
    defer why_kleur.deinit(allocator);
    expect_eq(why_kleur.paths.items.len, 1);

    // 3. Migrate pnpm-lock.yaml (v9 via cap:conf YAML 1.2) -> Lockfile
    const pnpm_yaml = "lockfileVersion: '9.0'\noverrides:\n  ms: 2.1.3\nimporters:\n  .:\n    dependencies:\n      chalk:\n        specifier: ^5.3.0\n        version: 5.3.0\npackages:\n  chalk@5.3.0:\n    resolution:\n      integrity: sha512-d3/example+hash==\nsnapshots:\n  chalk@5.3.0:\n    dependencies:\n      ansi-styles: 6.2.1\n  ansi-styles@6.2.1: {}\n";
    const pnpm_pj = "{\"name\":\"pnpm-proj\",\"version\":\"0.2.0\"}";

    let lf_pnpm = try LockfileMigrator.fromPnpmLockYaml(allocator, pnpm_yaml, pnpm_pj);
    defer lf_pnpm.deinit(allocator);
    expect_eq(lf_pnpm.overrides.items.len, 1);
    expect_eq(lf_pnpm.findPackageByName("chalk") != null, true);

    let why_chalk = try DependencyAnalyzer.why(allocator, &lf_pnpm, "chalk");
    defer why_chalk.deinit(allocator);
    expect_eq(why_chalk.paths.items.len, 1);
}

fn main() {
    test("Phase B-3 (1): Monorepo Workspaces, Catalogs, workspace:*^~ & DAG topoSort", () => {
        test_monorepo_workspaces_and_catalogs() catch unreachable;
    });

    test("Phase B-3 (2): RegistryHttpClient Auth, ETag/304 Disk Cache & RetryPolicy", () => {
        test_registry_http_client_cache_etag_and_retry() catch unreachable;
    });

    test("Phase B-3 (3): LifecycleRunner Zero-Trust Policy & PosixSpawn Execution", () => {
        test_lifecycle_runner_zero_trust_and_execution() catch unreachable;
    });

    test("Phase B-3 (4): LockfileMigrator (npm/yarn/pnpm) & DependencyAnalyzer (why/dups/audit)", () => {
        test_lockfile_migrations_and_dependency_analyzer() catch unreachable;
    });
}
