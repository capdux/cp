// Source: Absorbed from /data/dev/bun/src/env_loader.zig (XDG Base Directory Spec Resolver)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_CONFIG_DIRS,
    DEFAULT_DATA_DIRS,
    XdgEnv,
    XdgDirIter,
    resolveConfigHome,
    resolveDataHome,
    resolveCacheHome,
    resolveStateHome,
    resolveRuntimeDir,
} from "cap:xdg_basedir";

test("1. Default fallbacks from HOME for config, data, cache, state, and null runtime_dir", () => {
    const env = XdgEnv.empty("/home/alice/");
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(std.mem.eql(u8, resolveConfigHome(env, null, &buf) ?? unreachable, "/home/alice/.config"), true);
    expect_eq(std.mem.eql(u8, resolveConfigHome(env, "cp", &buf) ?? unreachable, "/home/alice/.config/cp"), true);
    expect_eq(std.mem.eql(u8, resolveDataHome(env, "cp", &buf) ?? unreachable, "/home/alice/.local/share/cp"), true);
    expect_eq(std.mem.eql(u8, resolveCacheHome(env, "cp", &buf) ?? unreachable, "/home/alice/.cache/cp"), true);
    expect_eq(std.mem.eql(u8, resolveStateHome(env, "cp", &buf) ?? unreachable, "/home/alice/.local/state/cp"), true);
    expect_eq(resolveRuntimeDir(env, "cp", &buf) == null, true);
});

test("2. Explicit absolute XDG_* overrides take precedence over HOME defaults", () => {
    let env = XdgEnv.empty("/home/alice");
    env.config_home = "/etc/mycfg/";
    env.data_home = "/var/lib/mydata";
    env.cache_home = "/tmp/mycache";
    env.state_home = "/var/state/mystate";
    env.runtime_dir = "/run/user/1000/";

    let buf: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(std.mem.eql(u8, resolveConfigHome(env, "app", &buf) ?? unreachable, "/etc/mycfg/app"), true);
    expect_eq(std.mem.eql(u8, resolveDataHome(env, "app", &buf) ?? unreachable, "/var/lib/mydata/app"), true);
    expect_eq(std.mem.eql(u8, resolveCacheHome(env, "app", &buf) ?? unreachable, "/tmp/mycache/app"), true);
    expect_eq(std.mem.eql(u8, resolveStateHome(env, "app", &buf) ?? unreachable, "/var/state/mystate/app"), true);
    expect_eq(std.mem.eql(u8, resolveRuntimeDir(env, "app", &buf) ?? unreachable, "/run/user/1000/app"), true);
    expect_eq(std.mem.eql(u8, resolveRuntimeDir(env, null, &buf) ?? unreachable, "/run/user/1000"), true);
});

test("3. Empty or relative XDG_* values are ignored per XDG spec and fall back cleanly", () => {
    let env = XdgEnv.empty("/home/bob");
    env.config_home = "relative/config";
    env.data_home = "";
    env.cache_home = "./cache";
    env.state_home = "../state";
    env.runtime_dir = "run/relative";

    let buf: [128]u8 = std.mem.zeroes([128]u8);

    expect_eq(std.mem.eql(u8, resolveConfigHome(env, "tool", &buf) ?? unreachable, "/home/bob/.config/tool"), true);
    expect_eq(std.mem.eql(u8, resolveDataHome(env, "tool", &buf) ?? unreachable, "/home/bob/.local/share/tool"), true);
    expect_eq(std.mem.eql(u8, resolveCacheHome(env, "tool", &buf) ?? unreachable, "/home/bob/.cache/tool"), true);
    expect_eq(std.mem.eql(u8, resolveStateHome(env, "tool", &buf) ?? unreachable, "/home/bob/.local/state/tool"), true);
    expect_eq(resolveRuntimeDir(env, "tool", &buf) == null, true);
});

test("4. XdgDirIter splits colon-separated dirs, skips empty/relative entries, and falls back to defaults", () => {
    // 默认值回退：config_dirs 与 data_dirs
    let it_cfg = XdgDirIter.init(null, DEFAULT_CONFIG_DIRS);
    expect_eq(std.mem.eql(u8, it_cfg.next() ?? unreachable, "/etc/xdg"), true);
    expect_eq(it_cfg.next() == null, true);

    let it_data = XdgDirIter.init("", DEFAULT_DATA_DIRS);
    expect_eq(std.mem.eql(u8, it_data.next() ?? unreachable, "/usr/local/share"), true);
    expect_eq(std.mem.eql(u8, it_data.next() ?? unreachable, "/usr/share"), true);
    expect_eq(it_data.next() == null, true);

    // 混合相对路径、空段与带尾斜杠的绝对路径
    let it_mix = XdgDirIter.init(":rel/dir:/opt/xdg/::./bad:/etc/custom:", DEFAULT_CONFIG_DIRS);
    expect_eq(std.mem.eql(u8, it_mix.next() ?? unreachable, "/opt/xdg"), true);
    expect_eq(std.mem.eql(u8, it_mix.next() ?? unreachable, "/etc/custom"), true);
    expect_eq(it_mix.next() == null, true);

    // 全是非法相对路径时回退默认目录
    let it_all_rel = XdgDirIter.init("foo:bar/baz:", DEFAULT_CONFIG_DIRS);
    expect_eq(std.mem.eql(u8, it_all_rel.next() ?? unreachable, "/etc/xdg"), true);
    expect_eq(it_all_rel.next() == null, true);
});

test("5. Missing/relative HOME, root HOME, traversal app_name rejection, and tiny buffer guards", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    // 无 HOME 或相对 HOME 返回 null
    expect_eq(resolveConfigHome(XdgEnv.empty(null), "app", &buf) == null, true);
    expect_eq(resolveConfigHome(XdgEnv.empty("relative/home"), "app", &buf) == null, true);

    // 根 HOME "/" 不产生双斜杠
    expect_eq(std.mem.eql(u8, resolveConfigHome(XdgEnv.empty("/"), "app", &buf) ?? unreachable, "/.config/app"), true);

    // 非法 app_name（路径穿越或前导斜杠）被拒绝
    const env = XdgEnv.empty("/home/alice");
    expect_eq(resolveConfigHome(env, "../escape", &buf) == null, true);
    expect_eq(resolveConfigHome(env, "a/../b", &buf) == null, true);
    expect_eq(resolveConfigHome(env, "/abs/app", &buf) == null, true);

    // 极小缓冲区返回 null
    let tiny: [8]u8 = std.mem.zeroes([8]u8);
    expect_eq(resolveConfigHome(env, "app", &tiny) == null, true);
});
