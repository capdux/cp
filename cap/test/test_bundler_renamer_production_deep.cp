// Production Deep Tests for cap:bundler Symbol Renamer & Minifier (lib/bundler/renamer.cp)
import { test, expect_eq } from "cap:test";
import {
    INLINE_NAME_CAP,
    MAX_MINIFY_SLOTS,
    RenamerError,
    SlotNamespace,
    InlineName,
    ensureValidJsIdentifier,
    isJsReservedWord,
    isCjsReservedName,
    encodeMinifiedNameWithAlphabet,
    encodeMinifiedName,
    NumberScopeRenamer,
    MinifySlotAssigner,
} from "cap:bundler";

test("bundler renamer: reserved words, CJS names, and identifier sanitization", fn() {
    expect_eq(isJsReservedWord("if"), true);
    expect_eq(isJsReservedWord("return"), true);
    expect_eq(isJsReservedWord("instanceof"), true);
    expect_eq(isJsReservedWord("await"), true);
    expect_eq(isJsReservedWord("foo"), false);
    expect_eq(isJsReservedWord("a"), false);

    expect_eq(isCjsReservedName("require"), true);
    expect_eq(isCjsReservedName("exports"), true);
    expect_eq(isCjsReservedName("__dirname"), true);
    expect_eq(isCjsReservedName("customVar"), false);

    let buf: [32]u8 = [0; 32];
    expect_eq(ensureValidJsIdentifier("", buf[0..]) catch unreachable, "_");
    expect_eq(ensureValidJsIdentifier("123abc", buf[0..]) catch unreachable, "_123abc");
    expect_eq(ensureValidJsIdentifier("my-mod.js", buf[0..]) catch unreachable, "my_mod_js");
    expect_eq(ensureValidJsIdentifier("$valid_1", buf[0..]) catch unreachable, "$valid_1");

    let tiny: [2]u8 = [0, 0];
    let too_small = false;
    * = ensureValidJsIdentifier("long_name", tiny[0..]) catch |e| {
        too_small = (e == RenamerError.BufferTooSmall);
        ""
    };
    expect_eq(too_small, true);
});

test("bundler renamer: Base54/Base64 minified name encoding and custom alphabet", fn() {
    let buf: [16]u8 = [0; 16];
    expect_eq(encodeMinifiedName(0, buf[0..]) catch unreachable, "a");
    expect_eq(encodeMinifiedName(25, buf[0..]) catch unreachable, "z");
    expect_eq(encodeMinifiedName(26, buf[0..]) catch unreachable, "A");
    expect_eq(encodeMinifiedName(52, buf[0..]) catch unreachable, "_");
    expect_eq(encodeMinifiedName(53, buf[0..]) catch unreachable, "$");
    expect_eq(encodeMinifiedName(54, buf[0..]) catch unreachable, "aa");
    expect_eq(encodeMinifiedName(55, buf[0..]) catch unreachable, "ba");

    // Custom frequency-sorted alphabet: most frequent chars first
    const custom_head = "etaoinshrdlcumwfgypbvkjxqzETAOINSHRDLCUMWFGYPBVKJXQZ_$";
    const custom_tail = "etaoinshrdlcumwfgypbvkjxqzETAOINSHRDLCUMWFGYPBVKJXQZ_$0123456789";
    expect_eq(encodeMinifiedNameWithAlphabet(0, custom_head, custom_tail, buf[0..]) catch unreachable, "e");
    expect_eq(encodeMinifiedNameWithAlphabet(1, custom_head, custom_tail, buf[0..]) catch unreachable, "t");
    expect_eq(encodeMinifiedNameWithAlphabet(54, custom_head, custom_tail, buf[0..]) catch unreachable, "ee");
});

test("bundler renamer: NumberScopeRenamer collision avoidance and keyword escaping", fn() {
    let scope = NumberScopeRenamer.init();
    let out: [32]u8 = [0; 32];

    expect_eq(scope.findUnusedName("foo", out[0..]) catch unreachable, "foo");
    expect_eq(scope.findUnusedName("foo", out[0..]) catch unreachable, "foo2");
    expect_eq(scope.findUnusedName("foo", out[0..]) catch unreachable, "foo3");

    // Pre-reserve "bar2" -> first "bar" gets "bar", second "bar" skips "bar2" -> "bar3"
    scope.reserve("bar2") catch unreachable;
    expect_eq(scope.findUnusedName("bar", out[0..]) catch unreachable, "bar");
    expect_eq(scope.findUnusedName("bar", out[0..]) catch unreachable, "bar3");

    // Reserved keyword "class" must never be emitted raw -> "class2"
    expect_eq(scope.findUnusedName("class", out[0..]) catch unreachable, "class2");
});

test("bundler renamer: NumberScopeRenamer parent-child scope chain", fn() {
    let parent = NumberScopeRenamer.init();
    let out: [32]u8 = [0; 32];

    expect_eq(parent.findUnusedName("item", out[0..]) catch unreachable, "item");

    let child = NumberScopeRenamer.initWithParent(&parent);
    expect_eq(child.isUsedInChain("item"), true);
    expect_eq(child.findUnusedName("item", out[0..]) catch unreachable, "item2");
    expect_eq(child.findUnusedName("local", out[0..]) catch unreachable, "local");
    expect_eq(parent.isUsedInChain("local"), false);
});

test("bundler renamer: MinifySlotAssigner frequency ordering, reserved skip, and JSX capital rule", fn() {
    let assigner = MinifySlotAssigner.init();
    // Reserve "a" so top-frequency symbol must skip "a" and get "b"
    assigner.addReserved("a") catch unreachable;

    const s_low = assigner.addSlot(1, 5, false) catch unreachable;
    const s_high = assigner.addSlot(2, 100, false) catch unreachable;
    const s_jsx = assigner.addSlot(3, 50, true) catch unreachable;
    const s_mid = assigner.addSlot(4, 20, false) catch unreachable;

    assigner.assignByFrequency(.default) catch unreachable;

    // Order by count desc: s_high(100) -> "b" (skipped reserved "a")
    //                      s_jsx(50, cap) -> skips "c".."z", gets "A"
    //                      s_mid(20) -> "B"
    //                      s_low(5) -> "C"
    expect_eq(assigner.assignedName(s_high) ?? "", "b");
    expect_eq(assigner.assignedName(s_jsx) ?? "", "A");
    expect_eq(assigner.assignedName(s_mid) ?? "", "B");
    expect_eq(assigner.assignedName(s_low) ?? "", "C");
});

test("bundler renamer: namespace rules (label/private_name) and capacity guards", fn() {
    // Label namespace ignores user reserved names (only skips JS keywords)
    let lbl = MinifySlotAssigner.init();
    lbl.addReserved("a") catch unreachable;
    const l0 = lbl.addSlot(1, 10, false) catch unreachable;
    lbl.assignByFrequency(.label) catch unreachable;
    expect_eq(lbl.assignedName(l0) ?? "", "a");

    // Private name namespace allows everything starting from "a"
    let priv_assigner = MinifySlotAssigner.init();
    priv_assigner.addReserved("a") catch unreachable;
    const p0 = priv_assigner.addSlot(1, 10, false) catch unreachable;
    priv_assigner.assignByFrequency(.private_name) catch unreachable;
    expect_eq(priv_assigner.assignedName(p0) ?? "", "a");

    // InlineName length guard (> 15 bytes)
    let too_long = false;
    * = InlineName.fromSlice("0123456789abcdef") catch |e| {
        too_long = (e == RenamerError.NameTooLong);
        InlineName.empty()
    };
    expect_eq(too_long, true);
    expect_eq(INLINE_NAME_CAP, @as(usize, 15));

    // Slot capacity guard (> MAX_MINIFY_SLOTS)
    let full = MinifySlotAssigner.init();
    let i: u32 = 0;
    while (i < MAX_MINIFY_SLOTS) {
        * = full.addSlot(i, 1, false) catch unreachable;
        i += 1;
    }
    let cap_err = false;
    * = full.addSlot(999, 1, false) catch |e| {
        cap_err = (e == RenamerError.SlotCapacityExceeded);
        @as(usize, 0)
    };
    expect_eq(cap_err, true);
});
