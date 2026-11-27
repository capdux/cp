// cap/test/testCapMongoUri.cp
// Comprehensive unit tests for @cap/mongo-uri

import { test, expect_eq } from "cap:test";
import {
    parseMongoUri,
    getQueryOption,
    formatMongoUri,
} from "../../packages/mongodb-connection-string-url/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn main() {
    test("1. @cap/mongo-uri — Standard mongodb:// with default port 27017", () => {
        const cfg = parseMongoUri("mongodb://localhost/appdb");
        expect_eq(cfg.valid, true);
        expect_eq(cfg.is_srv, false);
        expect_eq(cfg.tls, false);
        expect_eq(cfg.host_count, 1 as usize);
        expect_eq(sliceEq(cfg.hosts[0].host, "localhost"), true);
        expect_eq(cfg.hosts[0].port, 27017 as u16);
        expect_eq(sliceEq(cfg.database, "appdb"), true);
    });

    test("2. @cap/mongo-uri — Replica set multi-host, credentials & core query options", () => {
        const uri = "mongodb://admin:p4ss@db1.internal:27017,db2.internal:27018,db3.internal/orders?replicaSet=rs0&authSource=admin&tls=true";
        const cfg = parseMongoUri(uri);
        expect_eq(cfg.valid, true);
        expect_eq(sliceEq(cfg.username, "admin"), true);
        expect_eq(sliceEq(cfg.password, "p4ss"), true);
        expect_eq(cfg.host_count, 3 as usize);
        expect_eq(sliceEq(cfg.hosts[0].host, "db1.internal"), true);
        expect_eq(cfg.hosts[0].port, 27017 as u16);
        expect_eq(sliceEq(cfg.hosts[1].host, "db2.internal"), true);
        expect_eq(cfg.hosts[1].port, 27018 as u16);
        expect_eq(sliceEq(cfg.hosts[2].host, "db3.internal"), true);
        expect_eq(cfg.hosts[2].port, 27017 as u16);
        expect_eq(sliceEq(cfg.database, "orders"), true);
        expect_eq(sliceEq(cfg.replica_set, "rs0"), true);
        expect_eq(sliceEq(cfg.auth_source, "admin"), true);
        expect_eq(cfg.tls, true);
    });

    test("3. @cap/mongo-uri — mongodb+srv:// default TLS & SRV spec enforcement", () => {
        const srv = parseMongoUri("mongodb+srv://svc:tok@cluster0.atlas.example/prod");
        expect_eq(srv.valid, true);
        expect_eq(srv.is_srv, true);
        expect_eq(srv.tls, true);
        expect_eq(srv.host_count, 1 as usize);
        expect_eq(sliceEq(srv.hosts[0].host, "cluster0.atlas.example"), true);
        expect_eq(srv.hosts[0].port, 0 as u16);

        // Explicit tls=false overrides SRV default.
        const srvNoTls = parseMongoUri("mongodb+srv://cluster0.atlas.example/prod?tls=false");
        expect_eq(srvNoTls.valid, true);
        expect_eq(srvNoTls.tls, false);

        // SRV forbids explicit port or multiple hosts.
        expect_eq(parseMongoUri("mongodb+srv://cluster0.atlas.example:27017/prod").valid, false);
        expect_eq(parseMongoUri("mongodb+srv://a.example,b.example/prod").valid, false);
    });

    test("4. @cap/mongo-uri — IPv6 host & getQueryOption extraction", () => {
        const uri = "mongodb://[::1]:27018/test?readPreference=secondaryPreferred&maxPoolSize=50&retryWrites=true";
        const cfg = parseMongoUri(uri);
        expect_eq(cfg.valid, true);
        expect_eq(sliceEq(cfg.hosts[0].host, "::1"), true);
        expect_eq(cfg.hosts[0].port, 27018 as u16);

        expect_eq(sliceEq(getQueryOption(uri, "readPreference"), "secondaryPreferred"), true);
        expect_eq(sliceEq(getQueryOption(uri, "MAXPOOLSIZE"), "50"), true);
        expect_eq(getQueryOption(uri, "missingOpt").len, 0 as usize);
    });

    test("5. @cap/mongo-uri — formatMongoUri verbatim & password-redacted output", () => {
        const cfg = parseMongoUri("mongodb://app:supersecret@db1.local:27017,db2.local:27018/shop?replicaSet=rs1&tls=true");
        let rawBuf: [192]u8 = undefined;
        const nRaw = formatMongoUri(cfg, false, rawBuf[0..192]);
        expect_eq(sliceEq(rawBuf[0..nRaw], "mongodb://app:supersecret@db1.local:27017,db2.local:27018/shop?replicaSet=rs1&tls=true"), true);

        let redBuf: [192]u8 = undefined;
        const nRed = formatMongoUri(cfg, true, redBuf[0..192]);
        expect_eq(sliceEq(redBuf[0..nRed], "mongodb://app:***@db1.local:27017,db2.local:27018/shop?replicaSet=rs1&tls=true"), true);
    });

    test("6. @cap/mongo-uri — Reject invalid scheme, port, empty host & tiny buffer", () => {
        expect_eq(parseMongoUri("postgres://localhost/db").valid, false);
        expect_eq(parseMongoUri("mongodb://").valid, false);
        expect_eq(parseMongoUri("mongodb://localhost:0/db").valid, false);
        expect_eq(parseMongoUri("mongodb://localhost:99999/db").valid, false);
        expect_eq(parseMongoUri("mongodb://a.local,/db").valid, false);

        const cfg = parseMongoUri("mongodb://localhost/app");
        let tiny: [8]u8 = undefined;
        expect_eq(formatMongoUri(cfg, false, tiny[0..8]), 0 as usize);
    });
}
