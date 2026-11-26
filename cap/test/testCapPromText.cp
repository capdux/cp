// cap/test/testCapPromText.cp
// Comprehensive unit tests for @cap/prom-text (Prometheus Exposition Format)

import { test, expect_eq } from "cap:test";
import {
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM,
    Label,
    PromWriter,
    isValidMetricName,
    isValidLabelName,
} from "../../packages/@cap/prom-text/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/prom-text — isValidMetricName & isValidLabelName boundaries", () => {
        expect_eq(isValidMetricName("http_requests_total"), true);
        expect_eq(isValidMetricName(":custom:metric_1"), true);
        expect_eq(isValidMetricName(""), false);
        expect_eq(isValidMetricName("1bad_start"), false);
        expect_eq(isValidMetricName("bad-dash"), false);

        expect_eq(isValidLabelName("method"), true);
        expect_eq(isValidLabelName("_internal_2"), true);
        expect_eq(isValidLabelName(""), false);
        expect_eq(isValidLabelName("2num"), false);
        expect_eq(isValidLabelName("no:colon"), false);
    });

    test("2. @cap/prom-text — unlabeled & multi-label writeSampleInt output", () => {
        let buf: [256]u8 = undefined;
        let w: PromWriter = PromWriter.init(buf[0..256]);
        const dummy: [1]Label = [Label { key: "a", val: "b" }];
        const labels: [2]Label = [
            Label { key: "method", val: "GET" },
            Label { key: "status", val: "200" },
        ];

        expect_eq(w.writeSampleInt("up", dummy[0..0], 1), true);
        expect_eq(w.writeSampleInt("http_req", labels[0..2], 42), true);
        expect_eq(
            sliceEq(w.written(), "up 1\nhttp_req{method=\"GET\",status=\"200\"} 42\n"),
            true,
        );
    });

    test("3. @cap/prom-text — # HELP and # TYPE formatting", () => {
        let buf: [256]u8 = undefined;
        let w: PromWriter = PromWriter.init(buf[0..256]);
        expect_eq(w.writeHelp("req_total", "Line1\nPath\\bin"), true);
        expect_eq(w.writeType("req_total", METRIC_COUNTER), true);
        expect_eq(w.writeType("mem_bytes", METRIC_GAUGE), true);
        expect_eq(w.writeType("lat_sec", METRIC_HISTOGRAM), true);

        expect_eq(
            sliceEq(
                w.written(),
                "# HELP req_total Line1\\nPath\\\\bin\n# TYPE req_total counter\n# TYPE mem_bytes gauge\n# TYPE lat_sec histogram\n",
            ),
            true,
        );
    });

    test("4. @cap/prom-text — label value escaping for quote, backslash & newline", () => {
        let buf: [256]u8 = undefined;
        let w: PromWriter = PromWriter.init(buf[0..256]);
        const labels: [1]Label = [
            Label { key: "msg", val: "a\"b\\c\nd" },
        ];
        expect_eq(w.writeSampleInt("err_count", labels[0..1], 7), true);
        expect_eq(
            sliceEq(w.written(), "err_count{msg=\"a\\\"b\\\\c\\nd\"} 7\n"),
            true,
        );
    });

    test("5. @cap/prom-text — writeSampleWithTs includes millisecond timestamp", () => {
        let buf: [256]u8 = undefined;
        let w: PromWriter = PromWriter.init(buf[0..256]);
        const labels: [1]Label = [Label { key: "env", val: "prod" }];
        expect_eq(w.writeSampleWithTs("cpu_temp", labels[0..1], -5, 1732620000000), true);
        expect_eq(
            sliceEq(w.written(), "cpu_temp{env=\"prod\"} -5 1732620000000\n"),
            true,
        );
    });

    test("6. @cap/prom-text — rejects invalid metric/label names & buffer overflow", () => {
        let buf: [64]u8 = undefined;
        let w: PromWriter = PromWriter.init(buf[0..64]);
        const bad_lbl: [1]Label = [Label { key: "bad-key", val: "1" }];
        const ok_lbl: [1]Label = [Label { key: "ok", val: "1" }];

        expect_eq(w.writeHelp("9bad", "x"), false);
        expect_eq(w.writeType("ok_metric", 99), false);
        expect_eq(w.writeSampleInt("ok_metric", bad_lbl[0..1], 1), false);
        expect_eq(w.len(), 0 as usize);
        expect_eq(w.isOverflow(), false);

        let tiny: [10]u8 = undefined;
        let w2: PromWriter = PromWriter.init(tiny[0..10]);
        expect_eq(w2.writeSampleInt("very_long_metric_name", ok_lbl[0..1], 1), false);
        expect_eq(w2.isOverflow(), true);
        expect_eq(w2.len(), 0 as usize);
    });
}
