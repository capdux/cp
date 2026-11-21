// Source: Absorbed from /data/dev/bun/src/install/npmrc.zig (.npmrc Config & Scoped Registry Resolver)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_NPM_REGISTRY,
    NpmrcEntry,
    NpmrcIter,
    getNpmrcValue,
    getNpmrcBool,
    resolveRegistryForScope,
    findRegistrySetting,
} from "cap:npmrc_config";

test("1. NpmrcIter parses key=value, strips outer quotes, skips comments (# and ;), and tracks line_no", () => {
    const src =
        "# top comment\n"
        ++ "  ; semicolon comment\n"
        ++ "\n"
        ++ "registry = \"https://registry.example.org/\"\n"
        ++ "cache = '/var/cache/npm'\r\n"
        ++ "extra = a=b=c\n"
        ++ "not_a_pair_line\n"
        ++ "empty_val =\n";

    let it = NpmrcIter.init(src);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "registry"), true);
    expect_eq(std.mem.eql(u8, e1.value, "https://registry.example.org/"), true);
    expect_eq(e1.line_no, 4);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "cache"), true);
    expect_eq(std.mem.eql(u8, e2.value, "/var/cache/npm"), true);
    expect_eq(e2.line_no, 5);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "extra"), true);
    expect_eq(std.mem.eql(u8, e3.value, "a=b=c"), true);
    expect_eq(e3.line_no, 6);

    const e4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.key, "empty_val"), true);
    expect_eq(e4.value.len, 0);
    expect_eq(e4.line_no, 8);

    expect_eq(it.next() == null, true);
});

test("2. getNpmrcValue applies last-wins override and getNpmrcBool parses case-insensitive booleans", () => {
    const src =
        "\xEF\xBB\xBF"
        ++ "loglevel=warn\n"
        ++ "loglevel=silent\n"
        ++ "save-exact = TRUE\n"
        ++ "strict-ssl = False\n"
        ++ "invalid-bool = yes\n";

    expect_eq(std.mem.eql(u8, getNpmrcValue(src, "loglevel") ?? unreachable, "silent"), true);
    expect_eq(getNpmrcValue(src, "missing") == null, true);
    expect_eq(getNpmrcValue(src, "") == null, true);

    expect_eq(getNpmrcBool(src, "save-exact") ?? unreachable, true);
    expect_eq(getNpmrcBool(src, "strict-ssl") ?? unreachable, false);
    expect_eq(getNpmrcBool(src, "invalid-bool") == null, true);
    expect_eq(getNpmrcBool(src, "missing") == null, true);
});

test("3. resolveRegistryForScope prioritizes scoped registry, then global registry, then default npmjs", () => {
    const cfg =
        "registry=https://mirror.example.com/\n"
        ++ "@myorg:registry=https://npm.myorg.dev/\n"
        ++ "@team:registry=https://old.team.dev/\n"
        ++ "@team:registry=https://new.team.dev/\n";

    // 带 @ 与不带 @ 均能命中作用域仓库
    expect_eq(std.mem.eql(u8, resolveRegistryForScope(cfg, "@myorg"), "https://npm.myorg.dev/"), true);
    expect_eq(std.mem.eql(u8, resolveRegistryForScope(cfg, "myorg"), "https://npm.myorg.dev/"), true);
    expect_eq(std.mem.eql(u8, resolveRegistryForScope(cfg, "team"), "https://new.team.dev/"), true);

    // 未配置作用域或 null 回退到全局 registry
    expect_eq(std.mem.eql(u8, resolveRegistryForScope(cfg, "@other"), "https://mirror.example.com/"), true);
    expect_eq(std.mem.eql(u8, resolveRegistryForScope(cfg, null), "https://mirror.example.com/"), true);

    // 无任何配置时回退默认官方仓库
    expect_eq(std.mem.eql(u8, resolveRegistryForScope("# empty\n", "@myorg"), DEFAULT_NPM_REGISTRY), true);
    expect_eq(std.mem.eql(u8, resolveRegistryForScope("", null), "https://registry.npmjs.org/"), true);
});

test("4. findRegistrySetting matches host, subpath, port, and trailing-slash variations", () => {
    const cfg =
        "//registry.npmjs.org/:always-auth=true\n"
        ++ "//npm.acme.dev/team/pkg/:email=dev@acme.dev\n"
        ++ "//localhost:4873/:username=localuser\n"
        ++ "//no-slash.example.com:certfile=/etc/ssl/custom.pem\n"
        ++ "//registry.npmjs.org/:always-auth=false\n";

    // 同名后者覆盖
    expect_eq(
        std.mem.eql(u8, findRegistrySetting(cfg, "https://registry.npmjs.org/", "always-auth") ?? unreachable, "false"),
        true,
    );
    // 带子路径与有无尾斜杠均可匹配
    expect_eq(
        std.mem.eql(u8, findRegistrySetting(cfg, "HTTPS://npm.acme.dev/team/pkg", "email") ?? unreachable, "dev@acme.dev"),
        true,
    );
    // 带端口号的 HTTP 本地仓库
    expect_eq(
        std.mem.eql(u8, findRegistrySetting(cfg, "http://localhost:4873/", "username") ?? unreachable, "localuser"),
        true,
    );
    // 无尾斜杠键形式
    expect_eq(
        std.mem.eql(u8, findRegistrySetting(cfg, "https://no-slash.example.com/", "certfile") ?? unreachable, "/etc/ssl/custom.pem"),
        true,
    );
    // 不匹配的后缀或域名返回 null
    expect_eq(findRegistrySetting(cfg, "https://other.example.com/", "email") == null, true);
    expect_eq(findRegistrySetting(cfg, "https://registry.npmjs.org/", "missing") == null, true);
});

test("5. Malformed lines, mismatched quotes, empty keys, and invalid registry URLs are safely handled", () => {
    const weird =
        "=orphan_value\n"
        ++ "   =   \n"
        ++ "mismatched=\"abc'\n"
        ++ "single_char_quote=\"\n"
        ++ "last_line_no_nl=ok";

    let it = NpmrcIter.init(weird);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "mismatched"), true);
    expect_eq(std.mem.eql(u8, e1.value, "\"abc'"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "single_char_quote"), true);
    expect_eq(std.mem.eql(u8, e2.value, "\""), true);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.key, "last_line_no_nl"), true);
    expect_eq(std.mem.eql(u8, e3.value, "ok"), true);
    expect_eq(it.next() == null, true);

    expect_eq(findRegistrySetting(weird, "", "email") == null, true);
    expect_eq(findRegistrySetting(weird, "https://", "email") == null, true);
    expect_eq(findRegistrySetting(weird, "https://example.com/", "") == null, true);
});
