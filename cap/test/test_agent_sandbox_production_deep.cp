// Origin: Pure-CP Stdlib Test Suite (lib/agent/sandbox.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent sandbox policy arbiter:
// workspace root confinement, read-only enforcement, parent traversal & prefix-boundary
// mismatch rejection (/data/app vs /data/app2), denied_prefixes (.env / .git/config),
// command allowlist/denylist & shell metacharacter filtering, sensitive env key scrubbing,
// and NUL-byte / empty-path / small-buffer boundary defenses.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PathAccessMode,
    SandboxDecision,
    PathRule,
    CommandPolicy,
    checkPathSandbox,
    checkCommandSandbox,
    isSensitiveEnvKey,
    filterEnvIndices,
} from "cap:agent";

fn testWorkspaceRootConfinementAndReadOnly()! {
    const roots: [2]PathRule = [
        PathRule{ root: "/data/app", allow_write: true, allow_delete: false },
        PathRule{ root: "/data/readonly", allow_write: false, allow_delete: false },
    ];
    const denied: [2]string = [".env", ".git/config"];

    let scratch_arr: [256]u8 = undefined;
    let scratch: []u8 = scratch_arr[0..];

    // 1. Valid read/write inside writable workspace (including normalized sub/.. path)
    expect_eq(
        checkPathSandbox("/data/app/src/./sub/../main.cp", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.Allow,
        true,
    );
    expect_eq(
        checkPathSandbox("/data/app/src/main.cp", PathAccessMode.Write, roots[0..], denied[0..], scratch) == SandboxDecision.Allow,
        true,
    );
    // Delete is disabled on /data/app -> DenyReadOnly
    expect_eq(
        checkPathSandbox("/data/app/src/main.cp", PathAccessMode.Delete, roots[0..], denied[0..], scratch) == SandboxDecision.DenyReadOnly,
        true,
    );

    // 2. Read-only root allows Read, rejects Write & Delete
    expect_eq(
        checkPathSandbox("/data/readonly/doc.md", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.Allow,
        true,
    );
    expect_eq(
        checkPathSandbox("/data/readonly/doc.md", PathAccessMode.Write, roots[0..], denied[0..], scratch) == SandboxDecision.DenyReadOnly,
        true,
    );
}

fn testTraversalPrefixConfusionAndDeniedPrefixes()! {
    const roots: [1]PathRule = [
        PathRule{ root: "/data/app", allow_write: true, allow_delete: true },
    ];
    const denied: [2]string = [".env", ".git/config"];

    let scratch_arr: [256]u8 = undefined;
    let scratch: []u8 = scratch_arr[0..];

    // 1. Parent traversal escaping /data/app -> DenyOutsideRoot
    expect_eq(
        checkPathSandbox("/data/app/../../etc/hosts", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.DenyOutsideRoot,
        true,
    );

    // 2. Prefix boundary confusion: /data/app2 must NOT match /data/app -> DenyOutsideRoot
    expect_eq(
        checkPathSandbox("/data/app2/secret.txt", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.DenyOutsideRoot,
        true,
    );

    // 3. Denied prefixes inside allowed workspace -> DenyDeniedPath
    expect_eq(
        checkPathSandbox("/data/app/.env", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.DenyDeniedPath,
        true,
    );
    expect_eq(
        checkPathSandbox("/data/app/sub/.git/config", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.DenyDeniedPath,
        true,
    );

    // 4. Segment-boundary accuracy: `.env.example` is NOT `.env` -> Allow
    expect_eq(
        checkPathSandbox("/data/app/.env.example", PathAccessMode.Read, roots[0..], denied[0..], scratch) == SandboxDecision.Allow,
        true,
    );
}

fn testCommandAllowlistDenylistAndShellMeta()! {
    const allowed: [3]string = ["git", "cap", "rg"];
    const denied: [2]string = ["sh", "rm"];
    const policy = CommandPolicy{
        allowed_binaries: allowed[0..],
        denied_binaries: denied[0..],
        forbid_shell_metachars: true,
    };

    // 1. Allowed binary with clean arguments -> Allow
    const ok_args: [2]string = ["status", "--short"];
    expect_eq(checkCommandSandbox("git", ok_args[0..], policy) == SandboxDecision.Allow, true);
    expect_eq(checkCommandSandbox("/usr/bin/git", ok_args[0..], policy) == SandboxDecision.Allow, true);

    // 2. Denied or unlisted binaries -> DenyCommandNotAllowed
    const empty_args: []const string = [];
    expect_eq(checkCommandSandbox("sh", empty_args, policy) == SandboxDecision.DenyCommandNotAllowed, true);
    expect_eq(checkCommandSandbox("/bin/sh", empty_args, policy) == SandboxDecision.DenyCommandNotAllowed, true);
    expect_eq(checkCommandSandbox("curl", empty_args, policy) == SandboxDecision.DenyCommandNotAllowed, true);

    // 3. Shell metacharacters in arguments -> DenyShellMeta
    const meta_semi: [1]string = ["foo;bar"];
    const meta_pipe: [1]string = ["a|b"];
    const meta_dollar: [1]string = ["$(id)"];
    const meta_newline: [1]string = ["line1\nline2"];
    expect_eq(checkCommandSandbox("rg", meta_semi[0..], policy) == SandboxDecision.DenyShellMeta, true);
    expect_eq(checkCommandSandbox("rg", meta_pipe[0..], policy) == SandboxDecision.DenyShellMeta, true);
    expect_eq(checkCommandSandbox("rg", meta_dollar[0..], policy) == SandboxDecision.DenyShellMeta, true);
    expect_eq(checkCommandSandbox("rg", meta_newline[0..], policy) == SandboxDecision.DenyShellMeta, true);
}

fn testSensitiveEnvKeyScrubbingAndFiltering()! {
    expect_eq(isSensitiveEnvKey("API_KEY"), true);
    expect_eq(isSensitiveEnvKey("openai_api_key"), true);
    expect_eq(isSensitiveEnvKey("DB_PASSWORD"), true);
    expect_eq(isSensitiveEnvKey("GITHUB_TOKEN"), true);
    expect_eq(isSensitiveEnvKey("AWS_SECRET_ACCESS_KEY"), true);
    expect_eq(isSensitiveEnvKey("PATH"), false);
    expect_eq(isSensitiveEnvKey("HOME"), false);
    expect_eq(isSensitiveEnvKey("LANG"), false);

    const env_keys: [6]string = [
        "PATH",
        "API_KEY",
        "HOME",
        "DB_PASSWORD",
        "USER",
        "AUTH_TOKEN",
    ];
    const empty_allow: []const string = [];
    let out_arr: [6]usize = undefined;
    let out: []usize = out_arr[0..];

    // Strip sensitive without allowlist -> keeps PATH(0), HOME(2), USER(4)
    const n1 = filterEnvIndices(env_keys[0..], empty_allow, true, out);
    expect_eq(n1, 3);
    expect_eq(out[0], 0);
    expect_eq(out[1], 2);
    expect_eq(out[2], 4);

    // Allowlist + strip_sensitive: even if API_KEY is listed, strip_sensitive still drops it
    const allow_list: [3]string = ["PATH", "HOME", "API_KEY"];
    const n2 = filterEnvIndices(env_keys[0..], allow_list[0..], true, out);
    expect_eq(n2, 2);
    expect_eq(out[0], 0);
    expect_eq(out[1], 2);
}

fn testNullByteEmptyPathSmallScratchAndNamespace()! {
    const roots: [1]PathRule = [
        PathRule{ root: "/data/app", allow_write: true, allow_delete: false },
    ];
    const empty_denied: []const string = [];
    let scratch_arr: [64]u8 = undefined;
    let scratch: []u8 = scratch_arr[0..];

    // 1. NUL byte in path or command -> DenyNullByte
    const nul_path = "/data/app/a\x00b";
    expect_eq(
        checkPathSandbox(nul_path, PathAccessMode.Read, roots[0..], empty_denied, scratch) == SandboxDecision.DenyNullByte,
        true,
    );

    const allowed: [1]string = ["git"];
    const empty_denied_bins: []const string = [];
    const pol = CommandPolicy{
        allowed_binaries: allowed[0..],
        denied_binaries: empty_denied_bins,
        forbid_shell_metachars: true,
    };
    const nul_args: [1]string = ["ok\x00bad"];
    expect_eq(checkCommandSandbox("git", nul_args[0..], pol) == SandboxDecision.DenyNullByte, true);

    // 2. Empty path & undersized scratch -> DenyOutsideRoot
    expect_eq(
        checkPathSandbox("", PathAccessMode.Read, roots[0..], empty_denied, scratch) == SandboxDecision.DenyOutsideRoot,
        true,
    );
    let tiny_scratch_arr: [4]u8 = undefined;
    let tiny_scratch: []u8 = tiny_scratch_arr[0..];
    expect_eq(
        checkPathSandbox("/data/app/file.cp", PathAccessMode.Read, roots[0..], empty_denied, tiny_scratch) == SandboxDecision.DenyOutsideRoot,
        true,
    );

    // 3. Namespace access via `cap.agent.*`
    expect_eq(
        cap.agent.checkPathSandbox("/data/app/ok.cp", cap.agent.PathAccessMode.Read, roots[0..], empty_denied, scratch) == cap.agent.SandboxDecision.Allow,
        true,
    );
    expect_eq(cap.agent.isSensitiveEnvKey("MY_SECRET"), true);
}

pub fn main()! {
    test("1. agent/sandbox: workspace root confinement and read-only enforcement", () => {
        testWorkspaceRootConfinementAndReadOnly()?;
    });
    test("2. agent/sandbox: traversal, prefix boundary mismatch & denied_prefixes", () => {
        testTraversalPrefixConfusionAndDeniedPrefixes()?;
    });
    test("3. agent/sandbox: command allowlist, denylist & shell metacharacter filtering", () => {
        testCommandAllowlistDenylistAndShellMeta()?;
    });
    test("4. agent/sandbox: sensitive env key detection and filterEnvIndices scrubbing", () => {
        testSensitiveEnvKeyScrubbingAndFiltering()?;
    });
    test("5. agent/sandbox: NUL byte, empty path, small scratch & cap.agent namespace", () => {
        testNullByteEmptyPathSmallScratchAndNamespace()?;
    });
}
