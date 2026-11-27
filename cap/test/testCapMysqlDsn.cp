import { test, expect_eq } from "cap:test";
import {
    MysqlConfig,
    parse,
    parseUri,
    parseGoDsn,
    formatUri,
} from "../../packages/mysql2/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/mysql-dsn — full mysql:// URI with charset & TLS query", () => {
        let c: MysqlConfig = parse("mysql://root:s3cr3t@db.internal:3307/orders?charset=latin1&tls=true");
        expect_eq(c.valid, true);
        expect_eq(c.tls, true);
        expect_eq(eqStr(c.user, "root"), true);
        expect_eq(eqStr(c.password, "s3cr3t"), true);
        expect_eq(eqStr(c.host, "db.internal"), true);
        expect_eq(c.port, 3307);
        expect_eq(eqStr(c.database, "orders"), true);
        expect_eq(eqStr(c.charset, "latin1"), true);
    });

    test("2. @cap/mysql-dsn — mysqls:// default port 3306 & IPv6 host", () => {
        let c: MysqlConfig = parseUri("mysqls://app@[::1]:4000/tidb");
        expect_eq(c.valid, true);
        expect_eq(c.tls, true);
        expect_eq(eqStr(c.host, "::1"), true);
        expect_eq(c.port, 4000);
        expect_eq(eqStr(c.database, "tidb"), true);
        expect_eq(eqStr(c.charset, "utf8mb4"), true);
    });

    test("3. @cap/mysql-dsn — Go-MySQL style user:pass@tcp(host:port)/db", () => {
        let c: MysqlConfig = parse("admin:pw123@tcp(127.0.0.1:3306)/shop?ssl=1");
        expect_eq(c.valid, true);
        expect_eq(c.tls, true);
        expect_eq(eqStr(c.user, "admin"), true);
        expect_eq(eqStr(c.password, "pw123"), true);
        expect_eq(eqStr(c.host, "127.0.0.1"), true);
        expect_eq(c.port, 3306);
        expect_eq(eqStr(c.database, "shop"), true);
    });

    test("4. @cap/mysql-dsn — formatUri verbatim & password redaction", () => {
        let c: MysqlConfig = parse("mysql://dev:topsecret@localhost:3306/app");
        let buf: [128]u8 = undefined;
        let n1: usize = formatUri(c, false, buf[0..]);
        expect_eq(eqStr(buf[0..n1], "mysql://dev:topsecret@localhost:3306/app"), true);

        let n2: usize = formatUri(c, true, buf[0..]);
        expect_eq(eqStr(buf[0..n2], "mysql://dev:***@localhost:3306/app"), true);
    });

    test("5. @cap/mysql-dsn — reject invalid scheme, port & malformed Go DSN", () => {
        expect_eq(parseUri("postgres://localhost/db").valid, false);
        expect_eq(parseUri("mysql://localhost:99999/db").valid, false);
        expect_eq(parseGoDsn("user@unix(/tmp/mysql.sock)/db").valid, false);
        expect_eq(parse("").valid, false);
    });

    test("6. @cap/mysql-dsn — small output buffer protection", () => {
        let c: MysqlConfig = parse("mysql://root:pw@localhost/db");
        let tiny: [8]u8 = undefined;
        expect_eq(formatUri(c, true, tiny[0..]), 0);
    });
}
