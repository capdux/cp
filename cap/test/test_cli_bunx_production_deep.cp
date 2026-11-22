// cap/test/test_cli_bunx_production_deep.cp
// Production-deep regression tests for cap:cli bunx module
// Source: /data/dev/bun/src/cli/bunx_command.cp & /data/dev/bun/docs/pm/bunx.mdx

import { test, expect } from "cap:test";
import {
    BUNX_CACHE_TTL_SECS,
    BunxError,
    BunxRuntimeChoice,
    BunxPackageSpec,
    BunxOptions,
    formatBunxCreatePackageName,
    parseBunxPackageSpec,
    resolveBunxDefaultBinName,
    extractBinBasename,
    isBunxCacheStale,
    formatBunxCacheDirName,
    resolveBunxShebangRuntime,
} from "cap:cli";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("cli bunx: BunxOptions.parse flag ordering & passthrough semantics") {
    // --bun before target is consumed by bunx; trailing flags are passed through
    const argv1: [6]([]const u8) = ["bunx", "--bun", "--no-install", "cowsay", "--foo", "bar"];
    const o1 = BunxOptions.parse(argv1[0..], true) catch {
        expect(false);
        return;
    };
    expect(o1.run_in_bun);
    expect(o1.no_install);
    expect(!o1.verbose_install);
    expect(!o1.silent_install);
    expect(strEq(o1.package_name, "cowsay"));
    expect(o1.binary_name == null);

    const p1 = o1.passthroughSlice(argv1[0..]);
    expect(p1.len == 2);
    expect(strEq(p1[0], "--foo"));
    expect(strEq(p1[1], "bar"));

    // --bun AFTER target must NOT set run_in_bun — it is passed through to the binary
    const argv2: [3]([]const u8) = ["bunx", "my-cli", "--bun"];
    const o2 = BunxOptions.parse(argv2[0..], true) catch {
        expect(false);
        return;
    };
    expect(!o2.run_in_bun);
    expect(strEq(o2.package_name, "my-cli"));
    const p2 = o2.passthroughSlice(argv2[0..]);
    expect(p2.len == 1);
    expect(strEq(p2[0], "--bun"));
}

test("cli bunx: --package / -p forms & version/revision flags") {
    const a1: [5]([]const u8) = ["bunx", "-p", "renovate", "renovate-config-validator", "--strict"];
    const o1 = BunxOptions.parse(a1[0..], true) catch { expect(false); return; };
    expect(strEq(o1.package_name, "renovate"));
    if (bn := o1.binary_name) {
        expect(strEq(bn, "renovate-config-validator"));
    } else {
        expect(false);
    }
    expect(o1.passthroughSlice(a1[0..]).len == 1);

    const a2: [3]([]const u8) = ["bunx", "--package=@angular/cli@17", "ng"];
    const o2 = BunxOptions.parse(a2[0..], true) catch { expect(false); return; };
    expect(strEq(o2.package_name, "@angular/cli@17"));
    if (bn := o2.binary_name) {
        expect(strEq(bn, "ng"));
    } else {
        expect(false);
    }

    const a3: [3]([]const u8) = ["-b", "-p=typescript", "tsc"];
    const o3 = BunxOptions.parse(a3[0..], false) catch { expect(false); return; };
    expect(o3.run_in_bun);
    expect(strEq(o3.package_name, "typescript"));
    if (bn := o3.binary_name) {
        expect(strEq(bn, "tsc"));
    } else {
        expect(false);
    }

    // --version / --revision when no package provided
    const av: [2]([]const u8) = ["bunx", "--version"];
    const ov = BunxOptions.parse(av[0..], true) catch { expect(false); return; };
    expect(ov.show_version);
    expect(!ov.show_revision);

    const ar: [2]([]const u8) = ["bunx", "--revision"];
    const orv = BunxOptions.parse(ar[0..], true) catch { expect(false); return; };
    expect(orv.show_revision);
}

test("cli bunx: BunxOptions.parse malformed argument error guards") {
    // Missing value after --package
    const a1: [2]([]const u8) = ["bunx", "--package"];
    let e1 = false;
    * = BunxOptions.parse(a1[0..], true) catch { e1 = true; };
    expect(e1);

    // Empty value in --package=
    const a2: [3]([]const u8) = ["bunx", "--package=", "ng"];
    let e2 = false;
    * = BunxOptions.parse(a2[0..], true) catch { e2 = true; };
    expect(e2);

    // Missing binary name when --package is given
    const a3: [3]([]const u8) = ["bunx", "-p", "@angular/cli"];
    let e3 = false;
    * = BunxOptions.parse(a3[0..], true) catch { e3 = true; };
    expect(e3);

    // No package at all
    const a4: [1]([]const u8) = ["bunx"];
    let e4 = false;
    * = BunxOptions.parse(a4[0..], true) catch { e4 = true; };
    expect(e4);
}

test("cli bunx: formatBunxCreatePackageName scoped & unscoped templates") {
    let buf: [64]u8 = [0; 64];

    const n0 = formatBunxCreatePackageName("", buf[0..]) catch { expect(false); return; };
    expect(n0 == 0);

    const n1 = formatBunxCreatePackageName("vite", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "create-vite"));

    const n2 = formatBunxCreatePackageName("vite@5.0.0", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "create-vite@5.0.0"));

    const n3 = formatBunxCreatePackageName("@scope/app", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "@scope/create-app"));

    const n4 = formatBunxCreatePackageName("@scope/app@2", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n4], "@scope/create-app@2"));

    const n5 = formatBunxCreatePackageName("@scope", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n5], "@scope/create"));

    const n6 = formatBunxCreatePackageName("@scope@1.2.3", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n6], "@scope/create@1.2.3"));

    // BufferTooSmall guard
    let tiny: [4]u8 = [0; 4];
    let small_err = false;
    * = formatBunxCreatePackageName("vite", tiny[0..]) catch { small_err = true; };
    expect(small_err);
}

test("cli bunx: parseBunxPackageSpec, default bin & bin path basename") {
    const s1 = parseBunxPackageSpec("cowsay@1.5.0") catch { expect(false); return; };
    expect(!s1.is_scoped);
    expect(strEq(s1.package_name, "cowsay"));
    if (v := s1.version) { expect(strEq(v, "1.5.0")); } else { expect(false); }
    expect(strEq(resolveBunxDefaultBinName(s1.package_name), "cowsay"));

    const s2 = parseBunxPackageSpec("@angular/cli@latest") catch { expect(false); return; };
    expect(s2.is_scoped);
    expect(strEq(s2.package_name, "@angular/cli"));
    if (v := s2.version) { expect(strEq(v, "latest")); } else { expect(false); }
    expect(strEq(resolveBunxDefaultBinName(s2.package_name), "cli"));

    const s3 = parseBunxPackageSpec("@biomejs/biome") catch { expect(false); return; };
    expect(s3.is_scoped);
    expect(strEq(s3.package_name, "@biomejs/biome"));
    expect(s3.version == null);
    expect(strEq(resolveBunxDefaultBinName(s3.package_name), "biome"));

    // Invalid specs rejected
    let inv1 = false;
    * = parseBunxPackageSpec("") catch { inv1 = true; };
    expect(inv1);
    let inv2 = false;
    * = parseBunxPackageSpec("@scopeonly") catch { inv2 = true; };
    expect(inv2);
    let inv3 = false;
    * = parseBunxPackageSpec("@/pkg") catch { inv3 = true; };
    expect(inv3);
    let inv4 = false;
    * = parseBunxPackageSpec("@scope/") catch { inv4 = true; };
    expect(inv4);

    // Bin path basename extraction (POSIX & Windows)
    expect(strEq(extractBinBasename("dist/bin/my-cli.js"), "my-cli.js"));
    expect(strEq(extractBinBasename("dist\\bin\\tool.cmd"), "tool.cmd"));
    expect(strEq(extractBinBasename("cli.js"), "cli.js"));
}

test("cli bunx: 24h cache TTL boundary & safe cache directory naming") {
    expect(BUNX_CACHE_TTL_SECS == 86400);
    // Exact 24h boundary: 86400 is still fresh, 86401 is stale
    expect(!isBunxCacheStale(1_000_000, 1_000_000 + 86400));
    expect(isBunxCacheStale(1_000_000, 1_000_000 + 86401));
    expect(isBunxCacheStale(0, 100));
    expect(isBunxCacheStale(-5, 100));

    let buf: [128]u8 = [0; 128];
    const n1 = formatBunxCacheDirName(1000, "@angular/cli@17.0.0", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "bunx-1000-@angular+cli@17.0.0"));

    // Path traversal & control characters rejected
    let trav_err = false;
    * = formatBunxCacheDirName(0, "../etc/passwd", buf[0..]) catch { trav_err = true; };
    expect(trav_err);

    let ctrl_err = false;
    * = formatBunxCacheDirName(0, "pkg\nname", buf[0..]) catch { ctrl_err = true; };
    expect(ctrl_err);
}

test("cli bunx: shebang runtime resolution & --bun override") {
    // Respects #!/usr/bin/env node by default
    expect(resolveBunxShebangRuntime("#!/usr/bin/env node\nconsole.log(1);", false) == BunxRuntimeChoice.Node);
    expect(resolveBunxShebangRuntime("#!/usr/bin/env -S node --harmony\n", false) == BunxRuntimeChoice.Node);
    expect(resolveBunxShebangRuntime("#!/usr/local/bin/node\r\n", false) == BunxRuntimeChoice.Node);

    // --bun overrides node shebang
    expect(resolveBunxShebangRuntime("#!/usr/bin/env node\nconsole.log(1);", true) == BunxRuntimeChoice.Bun);

    // Explicit bun shebang & no shebang -> Bun
    expect(resolveBunxShebangRuntime("#!/usr/bin/env bun\nconsole.log(1);", false) == BunxRuntimeChoice.Bun);
    expect(resolveBunxShebangRuntime("#!/usr/local/bin/bun\n", false) == BunxRuntimeChoice.Bun);
    expect(resolveBunxShebangRuntime("console.log('no shebang');", false) == BunxRuntimeChoice.Bun);

    // Non-JS shebang -> Shebang (unless --bun forced)
    expect(resolveBunxShebangRuntime("#!/bin/sh\necho hi", false) == BunxRuntimeChoice.Shebang);
    expect(resolveBunxShebangRuntime("#!/usr/bin/env python3\nprint(1)", false) == BunxRuntimeChoice.Shebang);
    expect(resolveBunxShebangRuntime("#!/bin/sh\necho hi", true) == BunxRuntimeChoice.Bun);
}

test("cli bunx: end-to-end command resolution round-trip") {
    const argv: [6]([]const u8) = ["bunx", "--verbose", "--bun", "@biomejs/biome@1.9.4", "check", "./src"];
    const opts = BunxOptions.parse(argv[0..], true) catch { expect(false); return; };
    expect(opts.verbose_install);
    expect(opts.run_in_bun);

    const spec = parseBunxPackageSpec(opts.package_name) catch { expect(false); return; };
    expect(spec.is_scoped);
    expect(strEq(spec.package_name, "@biomejs/biome"));
    if (v := spec.version) { expect(strEq(v, "1.9.4")); } else { expect(false); }

    const bin_name = opts.binary_name ?? resolveBunxDefaultBinName(spec.package_name);
    expect(strEq(bin_name, "biome"));

    let cache_buf: [128]u8 = [0; 128];
    const cn = formatBunxCacheDirName(501, opts.package_name, cache_buf[0..]) catch { expect(false); return; };
    expect(strEq(cache_buf[0..cn], "bunx-501-@biomejs+biome@1.9.4"));

    const rt = resolveBunxShebangRuntime("#!/usr/bin/env node\n", opts.run_in_bun);
    expect(rt == BunxRuntimeChoice.Bun);

    const pass = opts.passthroughSlice(argv[0..]);
    expect(pass.len == 2);
    expect(strEq(pass[0], "check"));
    expect(strEq(pass[1], "./src"));
}
