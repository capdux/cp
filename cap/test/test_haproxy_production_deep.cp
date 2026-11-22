// Source: cp/cap/test/test_haproxy_production_deep.cp (Production deep tests for cap:haproxy)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HaSectionKind,
    HaSection,
    HaDirective,
    HaSectionIter,
    HaDirectiveIter,
    findHaSection,
    findHaDirective,
} from "cap:net";

const SAMPLE_HAPROXY: []const u8 =
    "# HAProxy production config\n" ++
    "\n" ++
    "global\n" ++
    "    log /dev/log local0 # syslog target\n" ++
    "    maxconn 4096\n" ++
    "    daemon\n" ++
    "\n" ++
    "defaults\n" ++
    "    mode http\n" ++
    "    timeout connect 5000ms\n" ++
    "    timeout client  50000ms\n" ++
    "    timeout server  50000ms\n" ++
    "\n" ++
    "frontend https_in # public ingress\n" ++
    "    bind *:443 ssl crt /etc/ssl/site.pem\n" ++
    "    http-request set-header X-Tag \"v1#prod\" # keep # inside quotes\n" ++
    "    http-request set-header X-Alt 'a#b' # keep # in single quotes\n" ++
    "    default_backend api_pool\n" ++
    "\n" ++
    "backend api_pool\n" ++
    "    balance roundrobin\n" ++
    "    option httpchk GET /health\n" ++
    "    server app1 10.0.0.11:8080 check\n" ++
    "    server app2 10.0.0.12:8080 check\n" ++
    "\n" ++
    "backend static_pool\n" ++
    "    balance leastconn\n" ++
    "    server cdn1 10.0.1.10:80 check\n" ++
    "\n" ++
    "listen stats\n" ++
    "    bind *:8404\n" ++
    "    stats enable\n" ++
    "    stats uri /metrics\n" ++
    "\n" ++
    "resolvers dns_corp\n" ++
    "    nameserver ns1 10.0.0.2:53\n" ++
    "\n" ++
    "peers cluster_peers\n" ++
    "    peer node1 10.0.0.1:10000\n" ++
    "\n" ++
    "userlist admins\n" ++
    "    user ops insecure-password secret\n" ++
    "\n" ++
    "cache micro_cache\n" ++
    "    total-max-size 64\n";

test("1. HaSectionIter splits all HAProxy section kinds including named and unnamed sections", () => {
    let it = HaSectionIter.init(SAMPLE_HAPROXY);

    const s0 = it.next() ?? unreachable;
    expect_eq(s0.kind == HaSectionKind.global, true);
    expect_eq(s0.name == null, true);

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.kind == HaSectionKind.defaults, true);
    expect_eq(s1.name == null, true);

    const s2 = it.next() ?? unreachable;
    expect_eq(s2.kind == HaSectionKind.frontend, true);
    expect_eq(std.mem.eql(u8, s2.name ?? "", "https_in"), true);

    const s3 = it.next() ?? unreachable;
    expect_eq(s3.kind == HaSectionKind.backend, true);
    expect_eq(std.mem.eql(u8, s3.name ?? "", "api_pool"), true);

    const s4 = it.next() ?? unreachable;
    expect_eq(s4.kind == HaSectionKind.backend, true);
    expect_eq(std.mem.eql(u8, s4.name ?? "", "static_pool"), true);

    const s5 = it.next() ?? unreachable;
    expect_eq(s5.kind == HaSectionKind.listen, true);
    expect_eq(std.mem.eql(u8, s5.name ?? "", "stats"), true);

    const s6 = it.next() ?? unreachable;
    expect_eq(s6.kind == HaSectionKind.resolvers, true);
    expect_eq(std.mem.eql(u8, s6.name ?? "", "dns_corp"), true);

    const s7 = it.next() ?? unreachable;
    expect_eq(s7.kind == HaSectionKind.peers, true);
    expect_eq(std.mem.eql(u8, s7.name ?? "", "cluster_peers"), true);

    const s8 = it.next() ?? unreachable;
    expect_eq(s8.kind == HaSectionKind.userlist, true);
    expect_eq(std.mem.eql(u8, s8.name ?? "", "admins"), true);

    const s9 = it.next() ?? unreachable;
    expect_eq(s9.kind == HaSectionKind.other, true);
    expect_eq(std.mem.eql(u8, s9.name ?? "", "micro_cache"), true);

    expect_eq(it.next() == null, true);
});

test("2. HaDirectiveIter strips trailing comments outside quotes, preserves quoted/escaped #, and handles CRLF", () => {
    const fe = findHaSection(SAMPLE_HAPROXY, HaSectionKind.frontend, "https_in") ?? unreachable;
    let dit = HaDirectiveIter.init(fe.body);

    const d0 = dit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d0.keyword, "bind"), true);
    expect_eq(std.mem.eql(u8, d0.args, "*:443 ssl crt /etc/ssl/site.pem"), true);

    const d1 = dit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d1.keyword, "http-request"), true);
    expect_eq(std.mem.eql(u8, d1.args, "set-header X-Tag \"v1#prod\""), true);

    const d2 = dit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d2.keyword, "http-request"), true);
    expect_eq(std.mem.eql(u8, d2.args, "set-header X-Alt 'a#b'"), true);

    const d3 = dit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d3.keyword, "default_backend"), true);
    expect_eq(std.mem.eql(u8, d3.args, "api_pool"), true);
    expect_eq(dit.next() == null, true);

    // Escaped \# and CRLF handling + bare keyword without args
    const crlf_body: []const u8 = "  daemon\r\n  # full comment\r\n  http-request set-var(req.x) str(foo\\#bar) # tail\r\n";
    let cit = HaDirectiveIter.init(crlf_body);
    const c0 = cit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, c0.keyword, "daemon"), true);
    expect_eq(c0.args.len, 0);

    const c1 = cit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, c1.keyword, "http-request"), true);
    expect_eq(std.mem.eql(u8, c1.args, "set-var(req.x) str(foo\\#bar)"), true);
    expect_eq(cit.next() == null, true);
});

test("3. findHaSection locates named and unnamed sections accurately", () => {
    const g = findHaSection(SAMPLE_HAPROXY, HaSectionKind.global, null) ?? unreachable;
    expect_eq(std.mem.eql(u8, findHaDirective(g.body, "maxconn") ?? "", "4096"), true);

    const b1 = findHaSection(SAMPLE_HAPROXY, HaSectionKind.backend, "api_pool") ?? unreachable;
    expect_eq(std.mem.eql(u8, findHaDirective(b1.body, "balance") ?? "", "roundrobin"), true);

    const b2 = findHaSection(SAMPLE_HAPROXY, HaSectionKind.backend, "static_pool") ?? unreachable;
    expect_eq(std.mem.eql(u8, findHaDirective(b2.body, "balance") ?? "", "leastconn"), true);

    // Missing section name or kind
    expect_eq(findHaSection(SAMPLE_HAPROXY, HaSectionKind.backend, "missing_pool") == null, true);
    expect_eq(findHaSection(SAMPLE_HAPROXY, HaSectionKind.frontend, "api_pool") == null, true);
});

test("4. findHaDirective returns first matching directive in section and null on miss", () => {
    const api = findHaSection(SAMPLE_HAPROXY, HaSectionKind.backend, "api_pool") ?? unreachable;
    expect_eq(std.mem.eql(u8, findHaDirective(api.body, "server") ?? "", "app1 10.0.0.11:8080 check"), true);
    expect_eq(std.mem.eql(u8, findHaDirective(api.body, "option") ?? "", "httpchk GET /health"), true);
    expect_eq(findHaDirective(api.body, "bind") == null, true);

    const stats = findHaSection(SAMPLE_HAPROXY, HaSectionKind.listen, "stats") ?? unreachable;
    expect_eq(std.mem.eql(u8, findHaDirective(stats.body, "bind") ?? "", "*:8404"), true);
    expect_eq(std.mem.eql(u8, findHaDirective(stats.body, "stats") ?? "", "enable"), true);
});

test("5. Boundary protection: empty config, comment-only config, empty section, no trailing newline", () => {
    let it_empty = HaSectionIter.init("");
    expect_eq(it_empty.next() == null, true);

    let it_comments = HaSectionIter.init("# only comments\n   # another\n\n");
    expect_eq(it_comments.next() == null, true);

    // Empty section followed by section without trailing newline
    const compact: []const u8 = "global\nbackend tiny\n  balance random";
    let it = HaSectionIter.init(compact);
    const s0 = it.next() ?? unreachable;
    expect_eq(s0.kind == HaSectionKind.global, true);
    expect_eq(s0.body.len, 0);

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.kind == HaSectionKind.backend, true);
    expect_eq(std.mem.eql(u8, s1.name ?? "", "tiny"), true);
    expect_eq(std.mem.eql(u8, findHaDirective(s1.body, "balance") ?? "", "random"), true);
    expect_eq(it.next() == null, true);
});
