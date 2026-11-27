import { test, expect_eq } from "cap:test";
import { AmqpConfig, parseAmqpUrl, formatAmqpUrl } from "../../packages/amqplib/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/amqp-url — Default amqp://localhost fills 5672, guest:guest, vhost='/'", () => {
        const cfg = parseAmqpUrl("amqp://localhost");
        expect_eq(cfg.valid, true);
        expect_eq(cfg.tls, false);
        expect_eq(testEq(cfg.host, "localhost"), true);
        expect_eq(cfg.port, 5672 as u16);
        expect_eq(testEq(cfg.username, "guest"), true);
        expect_eq(testEq(cfg.password, "guest"), true);
        expect_eq(testEq(cfg.vhost, "/"), true);
        expect_eq(cfg.heartbeat, 0 as u16);
    });

    test("2. @cap/amqp-url — amqps:// default 5671 with custom credentials & vhost", () => {
        const cfg = parseAmqpUrl("amqps://alice:s3cr3t@mq.cap.dev/prod_vhost");
        expect_eq(cfg.valid, true);
        expect_eq(cfg.tls, true);
        expect_eq(testEq(cfg.host, "mq.cap.dev"), true);
        expect_eq(cfg.port, 5671 as u16);
        expect_eq(testEq(cfg.username, "alice"), true);
        expect_eq(testEq(cfg.password, "s3cr3t"), true);
        expect_eq(testEq(cfg.vhost, "prod_vhost"), true);
    });

    test("3. @cap/amqp-url — Encoded /%2f root vhost normalization", () => {
        const cfg1 = parseAmqpUrl("amqp://broker.local:5672/%2f");
        expect_eq(cfg1.valid, true);
        expect_eq(testEq(cfg1.vhost, "/"), true);

        const cfg2 = parseAmqpUrl("amqp://broker.local:5672/%2F");
        expect_eq(cfg2.valid, true);
        expect_eq(testEq(cfg2.vhost, "/"), true);
    });

    test("4. @cap/amqp-url — IPv6 [::1]:5673 & ?heartbeat=30 query parameter", () => {
        const cfg = parseAmqpUrl("amqp://admin:pw@[::1]:5673/dev?heartbeat=30&frame_max=131072");
        expect_eq(cfg.valid, true);
        expect_eq(testEq(cfg.host, "::1"), true);
        expect_eq(cfg.port, 5673 as u16);
        expect_eq(testEq(cfg.vhost, "dev"), true);
        expect_eq(cfg.heartbeat, 30 as u16);
    });

    test("5. @cap/amqp-url — formatAmqpUrl verbatim roundtrip & password redaction", () => {
        const cfg = parseAmqpUrl("amqps://svc:topsecret@[::1]:5671/orders?heartbeat=60");
        expect_eq(cfg.valid, true);

        let buf: [256]u8 = undefined;
        const n_raw = formatAmqpUrl(cfg, false, buf[0..256]);
        expect_eq(testEq(buf[0..n_raw], "amqps://svc:topsecret@[::1]:5671/orders?heartbeat=60"), true);

        const reparsed = parseAmqpUrl(buf[0..n_raw]);
        expect_eq(reparsed.valid, true);
        expect_eq(testEq(reparsed.password, "topsecret"), true);
        expect_eq(reparsed.heartbeat, 60 as u16);

        const n_red = formatAmqpUrl(cfg, true, buf[0..256]);
        expect_eq(testEq(buf[0..n_red], "amqps://svc:***@[::1]:5671/orders?heartbeat=60"), true);
    });

    test("6. @cap/amqp-url — Rejects invalid scheme, bad port, bad heartbeat & undersized buffer", () => {
        expect_eq(parseAmqpUrl("http://localhost:5672").valid, false);
        expect_eq(parseAmqpUrl("amqp://").valid, false);
        expect_eq(parseAmqpUrl("amqp://localhost:99999").valid, false);
        expect_eq(parseAmqpUrl("amqp://localhost:0").valid, false);
        expect_eq(parseAmqpUrl("amqp://localhost?heartbeat=abc").valid, false);
        expect_eq(parseAmqpUrl("amqp://[::1").valid, false);

        const ok = parseAmqpUrl("amqp://localhost");
        let tiny: [8]u8 = undefined;
        expect_eq(formatAmqpUrl(ok, false, tiny[0..8]), 0 as usize);
    });
}
