import { test, expect_eq } from "cap:test";
import {
    TracePhase,
    TraceEvent,
    phaseCode,
    formatTraceEvent,
    formatTraceArray,
} from "../../packages/@cap/chrome-trace-event/src/index.cp";

fn main() {
    test("1. @cap/chrome-trace-event — phaseCode maps all TracePhase variants", () => {
        expect_eq(phaseCode(.Begin), 'B');
        expect_eq(phaseCode(.End), 'E');
        expect_eq(phaseCode(.Complete), 'X');
        expect_eq(phaseCode(.Instant), 'i');
        expect_eq(phaseCode(.Counter), 'C');
        expect_eq(phaseCode(.Metadata), 'M');
    });

    test("2. @cap/chrome-trace-event — formats Begin event without dur field", () => {
        let buf: [256]u8 = undefined;
        const ev = TraceEvent{
            name: "parse",
            cat: "compiler",
            ph: .Begin,
            ts_us: 1000,
            dur_us: 0,
            pid: 1,
            tid: 2,
        };
        const n = formatTraceEvent(ev, buf[0..]);
        expect_eq(buf[0..n], "{\"name\":\"parse\",\"cat\":\"compiler\",\"ph\":\"B\",\"ts\":1000,\"pid\":1,\"tid\":2}");
    });

    test("3. @cap/chrome-trace-event — formats Complete event with dur field", () => {
        let buf: [256]u8 = undefined;
        const ev = TraceEvent{
            name: "codegen",
            cat: "build",
            ph: .Complete,
            ts_us: 5000,
            dur_us: 250,
            pid: 42,
            tid: 7,
        };
        const n = formatTraceEvent(ev, buf[0..]);
        expect_eq(buf[0..n], "{\"name\":\"codegen\",\"cat\":\"build\",\"ph\":\"X\",\"ts\":5000,\"dur\":250,\"pid\":42,\"tid\":7}");
    });

    test("4. @cap/chrome-trace-event — escapes quotes, backslashes, and control chars", () => {
        let buf: [256]u8 = undefined;
        const ev = TraceEvent{
            name: "a\"b\\c\nd\te",
            cat: "io",
            ph: .Instant,
            ts_us: 0,
            dur_us: 0,
            pid: 0,
            tid: 0,
        };
        const n = formatTraceEvent(ev, buf[0..]);
        expect_eq(buf[0..n], "{\"name\":\"a\\\"b\\\\c\\nd\\te\",\"cat\":\"io\",\"ph\":\"i\",\"ts\":0,\"pid\":0,\"tid\":0}");
    });

    test("5. @cap/chrome-trace-event — formatTraceArray serializes multiple events", () => {
        let buf: [512]u8 = undefined;
        const evs: [2]TraceEvent = [
            TraceEvent{ name: "f", cat: "c", ph: .Begin, ts_us: 10, dur_us: 0, pid: 1, tid: 1 },
            TraceEvent{ name: "f", cat: "c", ph: .End, ts_us: 20, dur_us: 0, pid: 1, tid: 1 },
        ];
        const n = formatTraceArray(evs[0..], buf[0..]);
        expect_eq(
            buf[0..n],
            "[{\"name\":\"f\",\"cat\":\"c\",\"ph\":\"B\",\"ts\":10,\"pid\":1,\"tid\":1},{\"name\":\"f\",\"cat\":\"c\",\"ph\":\"E\",\"ts\":20,\"pid\":1,\"tid\":1}]"
        );
    });

    test("6. @cap/chrome-trace-event — returns 0 when output buffer is too small", () => {
        let tiny: [12]u8 = undefined;
        const ev = TraceEvent{
            name: "overflow",
            cat: "test",
            ph: .Counter,
            ts_us: 1,
            dur_us: 0,
            pid: 1,
            tid: 1,
        };
        expect_eq(formatTraceEvent(ev, tiny[0..]), 0);
        const evs: [1]TraceEvent = [ev];
        expect_eq(formatTraceArray(evs[0..], tiny[0..]), 0);
    });
}
