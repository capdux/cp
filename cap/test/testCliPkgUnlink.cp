// cap/test/testCliPkgUnlink.cp — Deep production tests for cap:cli pkgUnlink
// Covers:
//   1. SIMD package name parsing & path normalization (scoped, unscoped, traversal/abs rejection)
//   2. GlobalSelfUnlink mode: global symlink + global `.bin/` cleanup + empty `@scope/` reclamation
//   3. ProjectDepUnlink mode: `cap_modules/<pkg>` detach + `.bin/` + empty `@scope/` + Registry restore
//   4. Symlink safety guard: real directory protection, symlink escape rejection, sibling-prefix bypass defense
//   5. Idempotent missing-link skip & `--strict` fail-on-unsafe enforcement
//   6. CLI argument parser (`parseUnlinkCliArgs`) mode switching & report formatter (`formatPkgUnlinkReport`)
//   7. 500-iteration deterministic malformed package name & symlink path Fuzz gate

import { expect, expectEqual, test } from "cap:test";
import {
    parseUnlinkPackageName,
    normalizeUnlinkPath,
    isUnlinkPathWithinRoot,
    isSafeUnlinkBinName,
    formatGlobalLinkPath,
    formatLocalLinkPath,
    UnlinkMode,
    UnlinkFsEntryKind,
    UnlinkTargetProbe,
    UnlinkGuardStatus,
    validateUnlinkTarget,
    UnlinkOpKind,
    getUnlinkOpText,
    UnlinkPlannedOp,
    UnlinkPlanOptions,
    planPkgUnlink,
    parseUnlinkCliArgs,
    formatPkgUnlinkReport,
} from "cap:cli";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("pkgUnlink: SIMD package name parsing, path normalization & root containment", () => {
    const p1 = parseUnlinkPackageName("zod") catch unreachable;
    expect(!p1.is_scoped);
    expect(strEql(p1.short_name, "zod"));

    const p2 = parseUnlinkPackageName("@cap-org/ui-kit") catch unreachable;
    expect(p2.is_scoped);
    expect(strEql(p2.scope, "cap-org"));
    expect(strEql(p2.short_name, "ui-kit"));

    // Reject invalid names
    let bad_empty = false;
    * = parseUnlinkPackageName("") catch { bad_empty = true; };
    expect(bad_empty);

    let bad_trav = false;
    * = parseUnlinkPackageName("../evil") catch { bad_trav = true; };
    expect(bad_trav);

    let bad_abs = false;
    * = parseUnlinkPackageName("/etc/passwd") catch { bad_abs = true; };
    expect(bad_abs);

    let bad_scope = false;
    * = parseUnlinkPackageName("@onlyscope") catch { bad_scope = true; };
    expect(bad_scope);

    let bad_multi = false;
    * = parseUnlinkPackageName("@org/a/b") catch { bad_multi = true; };
    expect(bad_multi);

    // Path normalization & sibling-prefix defense
    let pbuf: [256]u8 = [0; 256];
    const n1 = normalizeUnlinkPath("/home/dev\\.cap//./install/global/", pbuf[0..]) catch unreachable;
    expect(strEql(pbuf[0..n1], "/home/dev/.cap/install/global"));

    expect(isUnlinkPathWithinRoot("/home/dev/.cap/global/pkg", "/home/dev/.cap/global"));
    // Sibling prefix `/home/dev/.cap/global_evil` must NOT match `/home/dev/.cap/global`
    expect(!isUnlinkPathWithinRoot("/home/dev/.cap/global_evil/pkg", "/home/dev/.cap/global"));

    const gl = formatGlobalLinkPath("/home/dev/.cap/", "@acme/ui", pbuf[0..]) catch unreachable;
    expect(strEql(pbuf[0..gl], "/home/dev/.cap/install/global/node_modules/@acme/ui"));
    const ll = formatLocalLinkPath("/ws/app/", "@acme/ui", pbuf[0..]) catch unreachable;
    expect(strEql(pbuf[0..ll], "/ws/app/cap_modules/@acme/ui"));

    expect(isSafeUnlinkBinName("cap-cli"));
    expect(!isSafeUnlinkBinName("../bin"));
    expect(!isSafeUnlinkBinName("a/b"));
});

test("pkgUnlink: GlobalSelfUnlink removes global bins, package symlink & empty @scope dir", () => {
    const bins: [2][]const u8 = ["acme-cli", "acme-fmt"];
    const probes: [1]UnlinkTargetProbe = [
        UnlinkTargetProbe{
            package_name: "@acme/cli",
            link_path: "/home/dev/.cap/install/global/node_modules/@acme/cli",
            entry_kind: UnlinkFsEntryKind.Symlink,
            symlink_target: "/workspace/acme-cli",
            expected_root_dir: "/workspace",
            scope_remaining_siblings: 0,
            bin_names: bins[0..],
        },
    ];

    let ops: [8]UnlinkPlannedOp = [UnlinkPlannedOp.empty(); 8];
    let arena: [1024]u8 = [0; 1024];
    const sum = planPkgUnlink(
        probes[0..],
        UnlinkPlanOptions{
            mode: UnlinkMode.GlobalSelfUnlink,
            global_root_dir: "/home/dev/.cap",
            project_root_dir: "/workspace/acme-cli",
        },
        ops[0..],
        arena[0..],
    ) catch unreachable;

    expectEqual(sum.removed_bin_symlinks, @as(usize, 2));
    expectEqual(sum.removed_pkg_symlinks, @as(usize, 1));
    expectEqual(sum.removed_empty_scopes, @as(usize, 1));
    expectEqual(sum.planned_registry_restores, @as(usize, 0));
    expectEqual(sum.ops_count, @as(usize, 4));

    expect(ops[0].kind == UnlinkOpKind.RemoveBinSymlink);
    expect(strEql(getUnlinkOpText(ops[0].path, arena[0..]), "/home/dev/.cap/bin/acme-cli"));
    expect(ops[1].kind == UnlinkOpKind.RemoveBinSymlink);
    expect(strEql(getUnlinkOpText(ops[1].path, arena[0..]), "/home/dev/.cap/bin/acme-fmt"));
    expect(ops[2].kind == UnlinkOpKind.RemovePackageSymlink);
    expect(strEql(getUnlinkOpText(ops[2].path, arena[0..]), "/home/dev/.cap/install/global/node_modules/@acme/cli"));
    expect(ops[3].kind == UnlinkOpKind.RemoveEmptyScopeDir);
    expect(strEql(getUnlinkOpText(ops[3].path, arena[0..]), "/home/dev/.cap/install/global/node_modules/@acme"));
});

test("pkgUnlink: ProjectDepUnlink detaches cap_modules symlink, keeps non-empty scope & restores Registry", () => {
    const bins_a: [1][]const u8 = ["ui-dev"];
    const no_bins: [0][]const u8 = [];
    const probes: [2]UnlinkTargetProbe = [
        UnlinkTargetProbe{
            package_name: "@acme/ui",
            link_path: "/ws/app/cap_modules/@acme/ui",
            entry_kind: UnlinkFsEntryKind.Symlink,
            symlink_target: "/home/dev/.cap/install/global/node_modules/@acme/ui",
            expected_root_dir: "/home/dev/.cap/install/global/node_modules",
            scope_remaining_siblings: 1, // another sibling exists -> do NOT rmdir @acme
            bin_names: bins_a[0..],
            registry_fallback_version: "^2.4.0",
        },
        UnlinkTargetProbe{
            package_name: "local-math",
            link_path: "/ws/app/cap_modules/local-math",
            entry_kind: UnlinkFsEntryKind.Symlink,
            symlink_target: "/home/dev/.cap/install/global/node_modules/local-math",
            expected_root_dir: "/home/dev/.cap/install/global/node_modules",
            scope_remaining_siblings: 0,
            bin_names: no_bins[0..],
            registry_fallback_version: "",
        },
    ];

    let ops: [12]UnlinkPlannedOp = [UnlinkPlannedOp.empty(); 12];
    let arena: [2048]u8 = [0; 2048];
    const sum = planPkgUnlink(
        probes[0..],
        UnlinkPlanOptions{
            mode: UnlinkMode.ProjectDepUnlink,
            global_root_dir: "/home/dev/.cap",
            project_root_dir: "/ws/app",
        },
        ops[0..],
        arena[0..],
    ) catch unreachable;

    expectEqual(sum.removed_pkg_symlinks, @as(usize, 2));
    expectEqual(sum.removed_bin_symlinks, @as(usize, 1));
    expectEqual(sum.removed_empty_scopes, @as(usize, 0));
    expectEqual(sum.planned_registry_restores, @as(usize, 2));

    expect(ops[0].kind == UnlinkOpKind.RemoveBinSymlink);
    expect(strEql(getUnlinkOpText(ops[0].path, arena[0..]), "/ws/app/cap_modules/.bin/ui-dev"));
    expect(ops[2].kind == UnlinkOpKind.RestoreRegistryDep);
    expect(strEql(getUnlinkOpText(ops[2].detail, arena[0..]), "^2.4.0"));
    expect(ops[4].kind == UnlinkOpKind.RestoreRegistryDep);
    expect(strEql(getUnlinkOpText(ops[4].detail, arena[0..]), "latest"));
});

test("pkgUnlink: safety guard protects real directories & blocks symlink target escapes", () => {
    // 1. Real directory (not a symlink) must be refused
    const v_dir = validateUnlinkTarget(
        UnlinkTargetProbe{
            package_name: "zod",
            link_path: "/ws/app/cap_modules/zod",
            entry_kind: UnlinkFsEntryKind.Directory,
            expected_root_dir: "/home/dev/.cap/install/global/node_modules",
        },
        UnlinkMode.ProjectDepUnlink,
    );
    expect(!v_dir.isSafe());
    expect(v_dir.status == UnlinkGuardStatus.RefusedNotSymlink);

    // 2. Symlink pointing outside expected global root must be refused
    const v_esc = validateUnlinkTarget(
        UnlinkTargetProbe{
            package_name: "zod",
            link_path: "/ws/app/cap_modules/zod",
            entry_kind: UnlinkFsEntryKind.Symlink,
            symlink_target: "/etc/shadow",
            expected_root_dir: "/home/dev/.cap/install/global/node_modules",
        },
        UnlinkMode.ProjectDepUnlink,
    );
    expect(!v_esc.isSafe());
    expect(v_esc.status == UnlinkGuardStatus.RefusedTargetEscape);

    // 3. Symlink target containing `..` traversal must be refused
    const v_trav = validateUnlinkTarget(
        UnlinkTargetProbe{
            package_name: "zod",
            link_path: "/ws/app/cap_modules/zod",
            entry_kind: UnlinkFsEntryKind.Symlink,
            symlink_target: "/home/dev/.cap/install/global/node_modules/../../../etc",
            expected_root_dir: "/home/dev/.cap/install/global/node_modules",
        },
        UnlinkMode.ProjectDepUnlink,
    );
    expect(v_trav.status == UnlinkGuardStatus.RefusedTargetEscape);
});

test("pkgUnlink: idempotent missing-link skip & strict fail_on_unsafe mode", () => {
    const probes: [2]UnlinkTargetProbe = [
        UnlinkTargetProbe{
            package_name: "not-linked",
            link_path: "/home/dev/.cap/install/global/node_modules/not-linked",
            entry_kind: UnlinkFsEntryKind.Missing,
        },
        UnlinkTargetProbe{
            package_name: "real-dir",
            link_path: "/home/dev/.cap/install/global/node_modules/real-dir",
            entry_kind: UnlinkFsEntryKind.Directory,
        },
    ];

    let ops: [8]UnlinkPlannedOp = [UnlinkPlannedOp.empty(); 8];
    let arena: [1024]u8 = [0; 1024];

    // Non-strict mode records Skip + Blocked cleanly
    const sum = planPkgUnlink(
        probes[0..],
        UnlinkPlanOptions{
            mode: UnlinkMode.GlobalSelfUnlink,
            global_root_dir: "/home/dev/.cap",
            fail_on_unsafe: false,
        },
        ops[0..],
        arena[0..],
    ) catch unreachable;
    expectEqual(sum.skipped_not_linked, @as(usize, 1));
    expectEqual(sum.blocked_unsafe, @as(usize, 1));
    expect(ops[0].kind == UnlinkOpKind.SkipAlreadyUnlinked);
    expect(ops[1].kind == UnlinkOpKind.BlockedUnsafeTarget);

    // Strict mode returns NotASymlink error
    let strict_err = false;
    * = planPkgUnlink(
        probes[0..],
        UnlinkPlanOptions{
            mode: UnlinkMode.GlobalSelfUnlink,
            global_root_dir: "/home/dev/.cap",
            fail_on_unsafe: true,
        },
        ops[0..],
        arena[0..],
    ) catch {
        strict_err = true;
    };
    expect(strict_err);
});

test("pkgUnlink: CLI argument parsing & human-readable report formatting", () => {
    const argv_self: [2][]const u8 = ["--dry-run", "--verbose"];
    let pkgs: [4][]const u8 = [""; 4];
    const parsed_self = parseUnlinkCliArgs(argv_self[0..], pkgs[0..]) catch unreachable;
    expect(parsed_self.mode == UnlinkMode.GlobalSelfUnlink);
    expect(parsed_self.dry_run);
    expect(parsed_self.verbose);
    expectEqual(parsed_self.pkg_count, @as(usize, 0));

    const argv_deps: [3][]const u8 = ["--strict", "zod", "@acme/ui"];
    const parsed_deps = parseUnlinkCliArgs(argv_deps[0..], pkgs[0..]) catch unreachable;
    expect(parsed_deps.mode == UnlinkMode.ProjectDepUnlink);
    expect(parsed_deps.fail_on_unsafe);
    expectEqual(parsed_deps.pkg_count, @as(usize, 2));
    expect(strEql(pkgs[0], "zod"));
    expect(strEql(pkgs[1], "@acme/ui"));

    // Format execution summary report
    const probes: [1]UnlinkTargetProbe = [
        UnlinkTargetProbe{
            package_name: "@acme/ui",
            link_path: "/ws/app/cap_modules/@acme/ui",
            entry_kind: UnlinkFsEntryKind.Symlink,
            symlink_target: "/home/dev/.cap/install/global/node_modules/@acme/ui",
            expected_root_dir: "/home/dev/.cap/install/global/node_modules",
            scope_remaining_siblings: 0,
            registry_fallback_version: "^1.0.0",
        },
    ];
    let ops: [8]UnlinkPlannedOp = [UnlinkPlannedOp.empty(); 8];
    let arena: [1024]u8 = [0; 1024];
    const sum = planPkgUnlink(
        probes[0..],
        UnlinkPlanOptions{
            mode: UnlinkMode.ProjectDepUnlink,
            global_root_dir: "/home/dev/.cap",
            project_root_dir: "/ws/app",
        },
        ops[0..],
        arena[0..],
    ) catch unreachable;

    let rep: [1024]u8 = [0; 1024];
    const rep_len = formatPkgUnlinkReport(sum, ops[0..sum.ops_count], arena[0..], rep[0..]) catch unreachable;
    const text = rep[0..rep_len];
    expect(containsSubstr(text, "cap unlink [mode=project-dep]"));
    expect(containsSubstr(text, "unlinked=1"));
    expect(containsSubstr(text, "scopes_reclaimed=1"));
    expect(containsSubstr(text, "restores=1"));
    expect(containsSubstr(text, "RMDIR_SCOPE"));
    expect(containsSubstr(text, "RESTORE_REG"));
});

test("pkgUnlink: 500-iteration deterministic malformed package name & symlink path Fuzz gate", () => {
    let name_buf: [256]u8 = [0; 256];
    let path_buf: [256]u8 = [0; 256];
    let norm_out: [512]u8 = [0; 512];

    let rng: u64 = 0xD15EA5E123456789;
    let blocked_or_rejected: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const r = rng *% 0x2545F4914F6CDD1D;
        const mode = iter % 5;

        if (mode == 0) {
            // Random non-ASCII / control / separator bytes in package name
            const len = 1 + @as(usize, @intCast(r % 48));
            let k: usize = 0;
            while (k < len) {
                rng ^= rng >> 12;
                rng ^= rng << 25;
                rng ^= rng >> 27;
                name_buf[k] = @as(u8, @truncate(rng));
                k += 1;
            }
            // Force at least one forbidden byte (NUL, space, backslash, or high bit)
            name_buf[len / 2] = if ((r & 1) == 0) '\\' else 0x80;
            let err_hit = false;
            * = parseUnlinkPackageName(name_buf[0..len]) catch { err_hit = true; };
            expect(err_hit);
            if (err_hit) blocked_or_rejected += 1;
        } else if (mode == 1) {
            // Traversal payloads in link_path / symlink_target
            const raw = "/home/dev/.cap/install/global/node_modules/../../etc/passwd";
            let err_hit = false;
            * = normalizeUnlinkPath(raw, norm_out[0..]) catch { err_hit = true; };
            expect(err_hit);
            if (err_hit) blocked_or_rejected += 1;
        } else if (mode == 2) {
            // Overlength package name (> 214 bytes)
            let k: usize = 0;
            while (k < 220) {
                name_buf[k] = 'a';
                k += 1;
            }
            let err_hit = false;
            * = parseUnlinkPackageName(name_buf[0..220]) catch { err_hit = true; };
            expect(err_hit);
            if (err_hit) blocked_or_rejected += 1;
        } else if (mode == 3) {
            // Malformed scoped names (`@/a`, `@a/`, `@a/b/c`, `@.a/b`, `@a/.b`)
            const pick = r % 5;
            const bad_name = if (pick == 0) "@/a"
                else if (pick == 1) "@a/"
                else if (pick == 2) "@a/b/c"
                else if (pick == 3) "@.a/b"
                else "@a/.b";
            let err_hit = false;
            * = parseUnlinkPackageName(bad_name) catch { err_hit = true; };
            expect(err_hit);
            if (err_hit) blocked_or_rejected += 1;
        } else {
            // Full validateUnlinkTarget fuzz with random escape/NUL/non-symlink combos
            const p_len = 8 + @as(usize, @intCast(r % 32));
            let k: usize = 0;
            while (k < p_len) {
                path_buf[k] = @as(u8, 'a') + @as(u8, @intCast((r +% @as(u64, k)) % 26));
                k += 1;
            }
            path_buf[0] = '/';
            path_buf[p_len - 1] = 0; // embedded NUL
            const v = validateUnlinkTarget(
                UnlinkTargetProbe{
                    package_name: "valid-pkg",
                    link_path: path_buf[0..p_len],
                    entry_kind: UnlinkFsEntryKind.Symlink,
                    symlink_target: path_buf[0..p_len],
                    expected_root_dir: "/home/dev/.cap/install/global/node_modules",
                },
                UnlinkMode.ProjectDepUnlink,
            );
            expect(!v.isSafe());
            if (!v.isSafe()) blocked_or_rejected += 1;
        }

        iter += 1;
    }

    expectEqual(blocked_or_rejected, @as(usize, 500));
});
