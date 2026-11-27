// cap/test/testCapFlatted.cp
// Comprehensive unit tests for @cap/flatted

import { test, expect_eq } from "cap:test";
import {
    FlatNodeKind,
    FlatArena,
    stringifyFlatted,
    parseFlatted,
} from "../../packages/flatted/src/index.cp";

fn main() {
    test("1. @cap/flatted — self-referencing circular object stringify & parse", () => {
        let graph = FlatArena.init();
        const root = graph.addObject();
        const name_id = graph.addStr("cp");
        expect_eq(graph.setField(root, "name", name_id), true);
        expect_eq(graph.setField(root, "self", root), true);

        let buf: [512]u8 = undefined;
        const n = stringifyFlatted(graph, root, buf[0..]);
        expect_eq(buf[0..n], "[{\"name\":\"1\",\"self\":\"0\"},\"cp\"]");

        let parsed = FlatArena.init();
        const rid = parseFlatted(buf[0..n], *parsed);
        expect_eq(rid, 0 as i64);
        const self_id = parsed.getField(usize(rid), "self");
        expect_eq(self_id, rid);
        const pname_id = parsed.getField(usize(rid), "name");
        expect_eq(parsed.nodeStr(usize(pname_id)), "cp");
    });

    test("2. @cap/flatted — mutual cyclic references between two objects", () => {
        let graph = FlatArena.init();
        const a = graph.addObject();
        const b = graph.addObject();
        const ver = graph.addInt(42);
        expect_eq(graph.setField(a, "b", b), true);
        expect_eq(graph.setField(b, "a", a), true);
        expect_eq(graph.setField(b, "v", ver), true);

        let buf: [512]u8 = undefined;
        const n = stringifyFlatted(graph, a, buf[0..]);
        expect_eq(buf[0..n], "[{\"b\":\"1\"},{\"a\":\"0\",\"v\":42}]");

        let parsed = FlatArena.init();
        const rid = parseFlatted(buf[0..n], *parsed);
        expect_eq(rid, 0 as i64);
        const pb = parsed.getField(usize(rid), "b");
        const pa = parsed.getField(usize(pb), "a");
        const pv = parsed.getField(usize(pb), "v");
        expect_eq(pa, rid);
        expect_eq(parsed.nodeInt(usize(pv)), 42 as i64);
    });

    test("3. @cap/flatted — string deduplication & cyclic array", () => {
        let graph = FlatArena.init();
        const arr = graph.addArray();
        const s1 = graph.addStr("dup");
        const s2 = graph.addStr("dup");
        expect_eq(graph.pushElem(arr, s1), true);
        expect_eq(graph.pushElem(arr, s2), true);
        expect_eq(graph.pushElem(arr, arr), true);

        let buf: [512]u8 = undefined;
        const n = stringifyFlatted(graph, arr, buf[0..]);
        expect_eq(buf[0..n], "[[\"1\",\"1\",\"0\"],\"dup\"]");

        let parsed = FlatArena.init();
        const rid = parseFlatted(buf[0..n], *parsed);
        expect_eq(rid, 0 as i64);
        expect_eq(parsed.elemCount(usize(rid)), 3 as usize);
        const e0 = parsed.getElem(usize(rid), 0);
        const e1 = parsed.getElem(usize(rid), 1);
        const e2 = parsed.getElem(usize(rid), 2);
        expect_eq(e0, e1);
        expect_eq(e2, rid);
        expect_eq(parsed.nodeStr(usize(e0)), "dup");
    });

    test("4. @cap/flatted — primitives (null, bool, negative int) inline serialization", () => {
        let graph = FlatArena.init();
        const arr = graph.addArray();
        const n_id = graph.addNull();
        const t_id = graph.addBool(true);
        const f_id = graph.addBool(false);
        const i_id = graph.addInt(-12345);
        * = graph.pushElem(arr, n_id);
        * = graph.pushElem(arr, t_id);
        * = graph.pushElem(arr, f_id);
        * = graph.pushElem(arr, i_id);

        let buf: [512]u8 = undefined;
        const n = stringifyFlatted(graph, arr, buf[0..]);
        expect_eq(buf[0..n], "[[null,true,false,-12345]]");

        let parsed = FlatArena.init();
        const rid = parseFlatted(buf[0..n], *parsed);
        expect_eq(rid, 0 as i64);
        expect_eq(parsed.nodeKind(usize(parsed.getElem(usize(rid), 0))) == .Null, true);
        expect_eq(parsed.nodeBool(usize(parsed.getElem(usize(rid), 1))), true);
        expect_eq(parsed.nodeBool(usize(parsed.getElem(usize(rid), 2))), false);
        expect_eq(parsed.nodeInt(usize(parsed.getElem(usize(rid), 3))), -12345 as i64);
    });

    test("5. @cap/flatted — scalar root value stringify & parse", () => {
        let graph = FlatArena.init();
        const s = graph.addStr("hello");
        let buf: [128]u8 = undefined;
        const n = stringifyFlatted(graph, s, buf[0..]);
        expect_eq(buf[0..n], "[\"hello\"]");

        let parsed = FlatArena.init();
        const rid = parseFlatted(buf[0..n], *parsed);
        expect_eq(rid, 0 as i64);
        expect_eq(parsed.nodeStr(usize(rid)), "hello");
    });

    test("6. @cap/flatted — invalid JSON and out-of-range slot refs return -1", () => {
        let parsed = FlatArena.init();
        expect_eq(parseFlatted("", *parsed), -1 as i64);
        expect_eq(parseFlatted("[]", *parsed), -1 as i64);
        expect_eq(parseFlatted("{}", *parsed), -1 as i64);
        expect_eq(parseFlatted("[{\"x\":\"99\"}]", *parsed), -1 as i64);
        expect_eq(parseFlatted("[{\"unclosed\"", *parsed), -1 as i64);
    });
}
