import { test, assert, assertEq } from "cap:test";
import {
    JsBindingError,
    JsBindingTag,
    JsArrayBindingItem,
    JsObjectBindingProp,
    JsBindingNode,
    JsBindingArena,
} from "cap:js";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("JsBindingArena nested array & object pattern formatting (pretty & minified)", () => {
    let nodes: [16]JsBindingNode = [JsBindingNode.empty(); 16];
    let arr_items: [8]JsArrayBindingItem = [JsArrayBindingItem{}; 8];
    let obj_props: [8]JsObjectBindingProp = [JsObjectBindingProp{}; 8];
    let arena = JsBindingArena.init(nodes[0..], arr_items[0..], obj_props[0..]);

    // Build: [first, , { x = 1, [dynKey]: y, ...objRest }, ...tail]
    const n_first = arena.addIdentifier("first", 1) catch unreachable;
    const n_hole = arena.addMissing(8) catch unreachable;
    const n_x = arena.addIdentifier("x", 12) catch unreachable;
    const n_y = arena.addIdentifier("y", 28) catch unreachable;
    const n_obj_rest = arena.addIdentifier("objRest", 34) catch unreachable;
    const props: [3]JsObjectBindingProp = [
        JsObjectBindingProp.shorthand("x", n_x, "1"),
        JsObjectBindingProp.computed("dynKey", n_y, ""),
        JsObjectBindingProp.spread(n_obj_rest),
    ];
    const n_obj = arena.addObject(props[0..], 10) catch unreachable;
    const n_tail = arena.addIdentifier("tail", 46) catch unreachable;
    const items: [4]JsArrayBindingItem = [
        JsArrayBindingItem.simple(n_first),
        JsArrayBindingItem.simple(n_hole),
        JsArrayBindingItem.simple(n_obj),
        JsArrayBindingItem.rest(n_tail),
    ];
    const root = arena.addArray(items[0..], 0) catch unreachable;

    assert(arena.hasDefaultValues(root) catch unreachable);
    assert(arena.hasRestElement(root) catch unreachable);

    let buf: [128]u8 = [0; 128];
    const n_pretty = arena.formatPattern(root, buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n_pretty], "[first, , { x = 1, [dynKey]: y, ...objRest }, ...tail]"));

    const n_min = arena.formatPattern(root, buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n_min], "[first,,{x=1,[dynKey]:y,...objRest},...tail]"));
});

test("JsBindingArena BoundNames extraction & duplicate binding detection", () => {
    let nodes: [16]JsBindingNode = [JsBindingNode.empty(); 16];
    let arr_items: [8]JsArrayBindingItem = [JsArrayBindingItem{}; 8];
    let obj_props: [8]JsObjectBindingProp = [JsObjectBindingProp{}; 8];
    let arena = JsBindingArena.init(nodes[0..], arr_items[0..], obj_props[0..]);

    const a1 = arena.addIdentifier("alpha", 0) catch unreachable;
    const b1 = arena.addIdentifier("beta", 6) catch unreachable;
    const c1 = arena.addIdentifier("gamma", 12) catch unreachable;
    const props: [2]JsObjectBindingProp = [
        JsObjectBindingProp.keyed("b", b1, ""),
        JsObjectBindingProp.keyed("c", c1, "0"),
    ];
    const obj = arena.addObject(props[0..], 5) catch unreachable;
    const items: [2]JsArrayBindingItem = [
        JsArrayBindingItem.simple(a1),
        JsArrayBindingItem.simple(obj),
    ];
    const root = arena.addArray(items[0..], 0) catch unreachable;

    let names: [8][]const u8 = [""; 8];
    const count = arena.collectBoundNames(root, names[0..]) catch unreachable;
    assertEq(count, 3);
    assert(strEql(names[0], "alpha"));
    assert(strEql(names[1], "beta"));
    assert(strEql(names[2], "gamma"));
    assert(!(arena.hasDuplicateBoundNames(root, names[0..]) catch unreachable));

    // Duplicate binding: [alpha, alpha]
    const dup_items: [2]JsArrayBindingItem = [
        JsArrayBindingItem.simple(a1),
        JsArrayBindingItem.simple(a1),
    ];
    const dup_root = arena.addArray(dup_items[0..], 20) catch unreachable;
    assert(arena.hasDuplicateBoundNames(dup_root, names[0..]) catch unreachable);
});

test("JsBindingArena formatAsAssignmentTarget parenthesizes top-level object pattern", () => {
    let nodes: [8]JsBindingNode = [JsBindingNode.empty(); 8];
    let arr_items: [4]JsArrayBindingItem = [JsArrayBindingItem{}; 4];
    let obj_props: [4]JsObjectBindingProp = [JsObjectBindingProp{}; 4];
    let arena = JsBindingArena.init(nodes[0..], arr_items[0..], obj_props[0..]);

    const a = arena.addIdentifier("a", 0) catch unreachable;
    const b = arena.addIdentifier("b", 4) catch unreachable;

    const props: [2]JsObjectBindingProp = [
        JsObjectBindingProp.shorthand("a", a, ""),
        JsObjectBindingProp.keyed("k", b, "42"),
    ];
    const obj_root = arena.addObject(props[0..], 0) catch unreachable;

    let buf: [128]u8 = [0; 128];
    const n1 = arena.formatAsAssignmentTarget(obj_root, "payload", buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n1], "({ a, k: b = 42 } = payload)"));

    const n2 = arena.formatAsAssignmentTarget(obj_root, "payload", buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n2], "({a,k:b=42}=payload)"));

    // Array destructuring does NOT need outer parentheses
    const items: [2]JsArrayBindingItem = [
        JsArrayBindingItem.simple(a),
        JsArrayBindingItem.withDefault(b, "0"),
    ];
    const arr_root = arena.addArray(items[0..], 10) catch unreachable;
    const n3 = arena.formatAsAssignmentTarget(arr_root, "list", buf[0..], false) catch unreachable;
    assert(strEql(buf[0..n3], "[a, b = 0] = list"));
});

test("JsBindingArena trailing hole preserves trailing comma in JS array pattern", () => {
    let nodes: [4]JsBindingNode = [JsBindingNode.empty(); 4];
    let arr_items: [4]JsArrayBindingItem = [JsArrayBindingItem{}; 4];
    let obj_props: [2]JsObjectBindingProp = [JsObjectBindingProp{}; 2];
    let arena = JsBindingArena.init(nodes[0..], arr_items[0..], obj_props[0..]);

    const a = arena.addIdentifier("a", 0) catch unreachable;
    const hole = arena.addMissing(2) catch unreachable;
    const items: [2]JsArrayBindingItem = [
        JsArrayBindingItem.simple(a),
        JsArrayBindingItem.simple(hole),
    ];
    const root = arena.addArray(items[0..], 0) catch unreachable;
    assert(!(arena.hasDefaultValues(root) catch unreachable));
    assert(!(arena.hasRestElement(root) catch unreachable));

    let buf: [32]u8 = [0; 32];
    const n = arena.formatPattern(root, buf[0..], true) catch unreachable;
    assert(strEql(buf[0..n], "[a,,]"));
});

test("JsBindingArena ECMAScript rest validation & capacity/buffer guards", () => {
    let nodes: [8]JsBindingNode = [JsBindingNode.empty(); 8];
    let arr_items: [8]JsArrayBindingItem = [JsArrayBindingItem{}; 8];
    let obj_props: [8]JsObjectBindingProp = [JsObjectBindingProp{}; 8];
    let arena = JsBindingArena.init(nodes[0..], arr_items[0..], obj_props[0..]);

    // Empty identifier rejected
    let empty_err = false;
    * = arena.addIdentifier("", 0) catch |e| { empty_err = (e == JsBindingError.EmptyIdentifier); };
    assert(empty_err);

    const a = arena.addIdentifier("a", 0) catch unreachable;
    const b = arena.addIdentifier("b", 2) catch unreachable;
    const hole = arena.addMissing(4) catch unreachable;

    // Rest not in last position rejected
    const bad_pos: [2]JsArrayBindingItem = [
        JsArrayBindingItem.rest(a),
        JsArrayBindingItem.simple(b),
    ];
    let pos_err = false;
    * = arena.addArray(bad_pos[0..], 0) catch |e| { pos_err = (e == JsBindingError.InvalidRestPosition); };
    assert(pos_err);

    // Rest with default initializer rejected
    const bad_def: [1]JsArrayBindingItem = [
        JsArrayBindingItem{ binding: a, default_expr: "[]", is_rest: true },
    ];
    let def_err = false;
    * = arena.addArray(bad_def[0..], 0) catch |e| { def_err = (e == JsBindingError.RestWithDefault); };
    assert(def_err);

    // Rest targeting missing hole rejected
    const bad_hole: [1]JsArrayBindingItem = [
        JsArrayBindingItem.rest(hole),
    ];
    let target_err = false;
    * = arena.addArray(bad_hole[0..], 0) catch |e| { target_err = (e == JsBindingError.InvalidRestTarget); };
    assert(target_err);

    // Object rest targeting nested array (non-identifier) rejected
    const ok_arr: [1]JsArrayBindingItem = [JsArrayBindingItem.simple(a)];
    const arr_id = arena.addArray(ok_arr[0..], 0) catch unreachable;
    const bad_obj_rest: [1]JsObjectBindingProp = [JsObjectBindingProp.spread(arr_id)];
    let obj_rest_err = false;
    * = arena.addObject(bad_obj_rest[0..], 0) catch |e| { obj_rest_err = (e == JsBindingError.InvalidRestTarget); };
    assert(obj_rest_err);

    // OutputTooSmall & BufferTooSmall guards
    let tiny_names: [0][]const u8 = [];
    let out_small = false;
    * = arena.collectBoundNames(arr_id, tiny_names[0..]) catch |e| { out_small = (e == JsBindingError.OutputTooSmall); };
    assert(out_small);

    let tiny_buf: [2]u8 = [0; 2];
    let buf_small = false;
    * = arena.formatPattern(arr_id, tiny_buf[0..], true) catch |e| { buf_small = (e == JsBindingError.BufferTooSmall); };
    assert(buf_small);

    // Tag string sanity
    assert(strEql(JsBindingTag.b_object.asStr(), "b_object"));
});
