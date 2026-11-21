// Source: cp/cap/test/test_dockerfile_parse_production_deep.cp (Production deep tests for cap:dockerfile_parse)

import { test, expect_eq } from "cap:test";
import {
    DockerInstruction,
    DockerfileIter,
    FromSpec,
    parseFromArgs,
    ExposePort,
    parseExposeToken,
} from "cap:docker";

test("dockerfile_parse: multi-instruction stream with comments, blank lines and backslash continuation") {
    const df =
        "\xEF\xBB\xBF" ++
        "# syntax=docker/dockerfile:1\n" ++
        "\n" ++
        "FROM --platform=linux/amd64 node:22-alpine AS builder\n" ++
        "WORKDIR /app\n" ++
        "RUN apk add --no-cache \\\n" ++
        "    # comment inside continuation\n" ++
        "    curl \\\n" ++
        "    git\n" ++
        "ENV NODE_ENV=production\n" ++
        "EXPOSE 8080/tcp\n";

    let it = DockerfileIter.init(df);
    let buf: [256]u8 = [0; 256];

    const i1 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i1.keyword, "FROM"), true);
    expect_eq(i1.line_no, 3);
    const from_spec = parseFromArgs(i1.args).?;
    expect_eq(std.mem.eql(u8, from_spec.platform.?, "linux/amd64"), true);
    expect_eq(std.mem.eql(u8, from_spec.image, "node:22-alpine"), true);
    expect_eq(std.mem.eql(u8, from_spec.stage_alias.?, "builder"), true);

    const i2 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i2.keyword, "WORKDIR"), true);
    expect_eq(std.mem.eql(u8, i2.args, "/app"), true);
    expect_eq(i2.line_no, 4);

    const i3 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i3.keyword, "RUN"), true);
    expect_eq(std.mem.eql(u8, i3.args, "apk add --no-cache curl git"), true);
    expect_eq(i3.line_no, 5);

    const i4 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i4.keyword, "ENV"), true);
    expect_eq(std.mem.eql(u8, i4.args, "NODE_ENV=production"), true);
    expect_eq(i4.line_no, 9);

    const i5 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i5.keyword, "EXPOSE"), true);
    expect_eq(i5.line_no, 10);
    const port = parseExposeToken(i5.args).?;
    expect_eq(port.port, 8080);
    expect_eq(port.is_udp, false);

    expect_eq(it.next(buf[0..]) == null, true);
}

test("dockerfile_parse: # escape=` directive for Windows-style paths and backtick continuation") {
    const df =
        "# escape=`\r\n" ++
        "FROM mcr.microsoft.com/windows/nanoserver:ltsc2022\r\n" ++
        "COPY C:\\app\\bin\\ `\r\n" ++
        "     C:\\dest\\\r\n";

    let it = DockerfileIter.init(df);
    let buf: [256]u8 = [0; 256];

    const i1 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i1.keyword, "FROM"), true);
    expect_eq(i1.line_no, 2);

    const i2 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, i2.keyword, "COPY"), true);
    expect_eq(std.mem.eql(u8, i2.args, "C:\\app\\bin\\ C:\\dest\\"), true);
    expect_eq(i2.line_no, 3);

    expect_eq(it.next(buf[0..]) == null, true);
}

test("dockerfile_parse: parseFromArgs variants and invalid syntax guards") {
    const f1 = parseFromArgs("alpine:3.20").?;
    expect_eq(f1.platform == null, true);
    expect_eq(std.mem.eql(u8, f1.image, "alpine:3.20"), true);
    expect_eq(f1.stage_alias == null, true);

    const f2 = parseFromArgs("rust:1.80 as build").?;
    expect_eq(f2.platform == null, true);
    expect_eq(std.mem.eql(u8, f2.image, "rust:1.80"), true);
    expect_eq(std.mem.eql(u8, f2.stage_alias.?, "build"), true);

    const f3 = parseFromArgs("--PLATFORM=linux/arm64 scratch").?;
    expect_eq(std.mem.eql(u8, f3.platform.?, "linux/arm64"), true);
    expect_eq(std.mem.eql(u8, f3.image, "scratch"), true);
    expect_eq(f3.stage_alias == null, true);

    // 非法 FROM 参数
    expect_eq(parseFromArgs("") == null, true);
    expect_eq(parseFromArgs("--platform=") == null, true);
    expect_eq(parseFromArgs("--platform=linux/amd64") == null, true);
    expect_eq(parseFromArgs("alpine AS") == null, true);
    expect_eq(parseFromArgs("alpine AS a extra") == null, true);
    expect_eq(parseFromArgs("alpine NOTAS a") == null, true);
}

test("dockerfile_parse: parseExposeToken TCP/UDP and range validation") {
    const p1 = parseExposeToken("8080").?;
    expect_eq(p1.port, 8080);
    expect_eq(p1.is_udp, false);

    const p2 = parseExposeToken("443/TCP").?;
    expect_eq(p2.port, 443);
    expect_eq(p2.is_udp, false);

    const p3 = parseExposeToken("53/udp").?;
    expect_eq(p3.port, 53);
    expect_eq(p3.is_udp, true);

    const p4 = parseExposeToken("65535").?;
    expect_eq(p4.port, 65535);

    expect_eq(parseExposeToken("") == null, true);
    expect_eq(parseExposeToken("0") == null, true);
    expect_eq(parseExposeToken("65536") == null, true);
    expect_eq(parseExposeToken("8080/sctp") == null, true);
    expect_eq(parseExposeToken("8080/") == null, true);
    expect_eq(parseExposeToken("abc") == null, true);
}

test("dockerfile_parse: empty file, comments-only file and trailing continuation EOF guard") {
    let buf: [64]u8 = [0; 64];

    let empty_it = DockerfileIter.init("# only comment\n   \n# another\n");
    expect_eq(empty_it.next(buf[0..]) == null, true);

    // 末尾悬空续行符不死循环、安全返回
    let dangling = DockerfileIter.init("ARG VERSION=1.0 \\\n");
    const instr = dangling.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, instr.keyword, "ARG"), true);
    expect_eq(std.mem.eql(u8, instr.args, "VERSION=1.0"), true);
    expect_eq(dangling.next(buf[0..]) == null, true);
}
