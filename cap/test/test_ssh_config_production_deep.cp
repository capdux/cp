// Source: cp/cap/test/test_ssh_config_production_deep.cp (Production deep tests for cap:ssh_config)

import { test, expect_eq } from "cap:test";
import {
    SshHostConfig,
    matchHostPatterns,
    resolveSshConfig,
    getSshOption,
} from "cap:ssh";

test("ssh_config: matchHostPatterns wildcards and ! negation veto rule") {
    expect_eq(matchHostPatterns("*", "anything"), true);
    expect_eq(matchHostPatterns("prod-* staging-?", "prod-web-1"), true);
    expect_eq(matchHostPatterns("prod-* staging-?", "staging-a"), true);
    expect_eq(matchHostPatterns("prod-* staging-?", "staging-ab"), false);

    // 否定模式一票否决（无论出现在通配前还是后）
    expect_eq(matchHostPatterns("*.example.com !bastion.example.com", "app.example.com"), true);
    expect_eq(matchHostPatterns("*.example.com !bastion.example.com", "bastion.example.com"), false);
    expect_eq(matchHostPatterns("!bastion.example.com *.example.com", "bastion.example.com"), false);
    // 仅有否定模式而无肯定模式匹配，按 ssh_config(5) 不匹配
    expect_eq(matchHostPatterns("!bastion.example.com", "app.example.com"), false);
    expect_eq(matchHostPatterns("*", ""), false);
}

test("ssh_config: resolveSshConfig first-obtained-value-wins with Host * fallback") {
    const content =
        "\xEF\xBB\xBF" ++
        "# Production cluster\n" ++
        "Host prod-* !prod-bastion\n" ++
        "  HostName 10.0.1.50\n" ++
        "  User=deploy\n" ++
        "  Port 2222\n" ++
        "  ProxyJump prod-bastion\n" ++
        "  IdentityFile \"~/.ssh/id_ed25519_prod\"\n" ++
        "  ForwardAgent yes\n" ++
        "\n" ++
        "Host *\n" ++
        "  User ubuntu\n" ++
        "  Port=22\n" ++
        "  ForwardAgent no\n" ++
        "  StrictHostKeyChecking accept-new\n";

    // prod-api 命中首块，后续 Host * 仅补全未设置字段（StrictHostKeyChecking）
    const c_api = resolveSshConfig(content, "prod-api");
    expect_eq(std.mem.eql(u8, c_api.hostname.?, "10.0.1.50"), true);
    expect_eq(std.mem.eql(u8, c_api.user.?, "deploy"), true);
    expect_eq(c_api.port.?, 2222);
    expect_eq(std.mem.eql(u8, c_api.proxy_jump.?, "prod-bastion"), true);
    expect_eq(std.mem.eql(u8, c_api.identity_file.?, "~/.ssh/id_ed25519_prod"), true);
    expect_eq(c_api.forward_agent.?, true);
    expect_eq(std.mem.eql(u8, c_api.strict_host_key_checking.?, "accept-new"), true);

    // prod-bastion 被 !prod-bastion 否决，落入 Host * 全局回退
    const c_bastion = resolveSshConfig(content, "prod-bastion");
    expect_eq(c_bastion.hostname == null, true);
    expect_eq(std.mem.eql(u8, c_bastion.user.?, "ubuntu"), true);
    expect_eq(c_bastion.port.?, 22);
    expect_eq(c_bastion.forward_agent.?, false);
    expect_eq(std.mem.eql(u8, c_bastion.strict_host_key_checking.?, "accept-new"), true);
}

test("ssh_config: getSshOption custom options, case-insensitivity and Key=Value syntax") {
    const content =
        "ServerAliveInterval 60\n" ++
        "Host db-*\n" ++
        "  Compression=yes\n" ++
        "  ServerAliveInterval=15\n" ++
        "Host *\n" ++
        "  Compression no\n";

    // 全局前置 ServerAliveInterval 60 先出现，优先生效
    expect_eq(std.mem.eql(u8, getSshOption(content, "db-primary", "serveraliveinterval").?, "60"), true);
    expect_eq(std.mem.eql(u8, getSshOption(content, "db-primary", "COMPRESSION").?, "yes"), true);
    expect_eq(std.mem.eql(u8, getSshOption(content, "web-1", "compression").?, "no"), true);
    expect_eq(getSshOption(content, "web-1", "UnknownOption") == null, true);
}

test("ssh_config: Match all block and CRLF line endings") {
    const content =
        "Match all\r\n" ++
        "  User global_user\r\n" ++
        "  Port 2200\r\n";

    const c = resolveSshConfig(content, "any-host");
    expect_eq(std.mem.eql(u8, c.user.?, "global_user"), true);
    expect_eq(c.port.?, 2200);
}

test("ssh_config: invalid port and invalid boolean values safely ignored") {
    const content =
        "Host test\n" ++
        "  Port 99999\n" ++
        "  Port 2222\n" ++
        "  ForwardAgent maybe\n" ++
        "  ForwardAgent no\n";

    const c = resolveSshConfig(content, "test");
    expect_eq(c.port.?, 2222);
    expect_eq(c.forward_agent.?, false);
}
