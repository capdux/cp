// Source: cp/cap/test/test_tagged_ptr_production_deep.cp (Production deep tests for cap:tagged_ptr)

import { test, expect_eq } from "cap:test";
import {
    TaggedPointer,
    TaggedPtr2,
    TaggedPtr4,
    PTR_BITS,
    TAG_BITS,
    PTR_MASK,
    MAX_TAG,
} from "cap:tagged_ptr";

struct NodeA {
    id: u32,
}

struct NodeB {
    score: i64,
}

struct NodeC {
    flag: bool,
}

struct NodeD {
    weight: f64,
}

test("TaggedPointer 8-byte layout and 49-bit pointer + 15-bit tag lossless roundtrip") {
    expect_eq(PTR_BITS, @as(u6, 49));
    expect_eq(TAG_BITS, @as(u6, 15));
    expect_eq(@sizeOf(TaggedPointer), @as(usize, 8));
    const u2_sample = TaggedPtr2<NodeA, NodeB>.empty();
    const u4_sample = TaggedPtr4<NodeA, NodeB, NodeC, NodeD>.empty();
    expect_eq(@sizeOf(@TypeOf(u2_sample)), @as(usize, 8));
    expect_eq(@sizeOf(@TypeOf(u4_sample)), @as(usize, 8));

    let a = NodeA{ id: 1234 };
    const tp = TaggedPointer.fromPtr(NodeA, &a, 42);
    expect_eq(tp.isNull(), false);
    expect_eq(tp.tag(), @as(u15, 42));

    if (ptr := tp.getPtr(NodeA)) {
        unsafe {
            expect_eq(ptr.id, @as(u32, 1234));
            ptr.id = 5678;
        }
    } else {
        expect_eq(true, false);
    }
    expect_eq(a.id, @as(u32, 5678));
}

test("Null pointer combined with zero and non-zero 15-bit tags") {
    const empty = TaggedPointer.empty();
    expect_eq(empty.isNull(), true);
    expect_eq(empty.address(), @as(usize, 0));
    expect_eq(empty.tag(), @as(u15, 0));
    expect_eq(empty.toU64(), @as(u64, 0));
    expect_eq(empty.getPtr(NodeA) == null, true);

    const tagged_null = TaggedPointer.nullWithTag(1337);
    expect_eq(tagged_null.isNull(), true);
    expect_eq(tagged_null.address(), @as(usize, 0));
    expect_eq(tagged_null.tag(), @as(u15, 1337));
    expect_eq(tagged_null.getPtr(NodeA) == null, true);

    const max_null = TaggedPointer.nullWithTag(MAX_TAG);
    expect_eq(max_null.isNull(), true);
    expect_eq(max_null.tag(), MAX_TAG);
    expect_eq(max_null.toU64(), (@as(u64, MAX_TAG) << 49));
}

test("fromU64/toU64 bit fidelity and setTag/withTag/setAddress non-interference") {
    let b = NodeB{ score: -999 };
    let tp = TaggedPointer.fromPtr(NodeB, &b, 100);
    const orig_addr = tp.address();

    // toU64 <-> fromU64 严格位级保真
    const raw = tp.toU64();
    const restored = TaggedPointer.fromU64(raw);
    expect_eq(restored.eql(tp), true);
    expect_eq(restored.address(), orig_addr);
    expect_eq(restored.tag(), @as(u15, 100));

    // 修改标签不破坏指针地址
    tp.setTag(MAX_TAG);
    expect_eq(tp.tag(), MAX_TAG);
    expect_eq(tp.address(), orig_addr);
    if (p := tp.getPtr(NodeB)) {
        unsafe { expect_eq(p.score, @as(i64, -999)); }
    } else {
        expect_eq(true, false);
    }

    const tp2 = tp.withTag(7);
    expect_eq(tp2.tag(), @as(u15, 7));
    expect_eq(tp2.address(), orig_addr);
    expect_eq(tp.tag(), MAX_TAG);

    // 修改地址不破坏高 15 位标签
    let b2 = NodeB{ score: 2048 };
    let b2_addr: usize = 0;
    unsafe { b2_addr = (&b2) as usize; }
    tp.setAddress(b2_addr);
    expect_eq(tp.tag(), MAX_TAG);
    expect_eq(tp.address(), b2_addr & (PTR_MASK as usize));
    if (p2 := tp.getPtr(NodeB)) {
        unsafe { expect_eq(p2.score, @as(i64, 2048)); }
    } else {
        expect_eq(true, false);
    }
}

test("TaggedPtr2 safe typed dispatch, mutation and mismatch null guard") {
    let a = NodeA{ id: 10 };
    let b = NodeB{ score: 200 };

    let u = TaggedPtr2<NodeA, NodeB>.empty();
    expect_eq(u.isEmpty(), true);
    expect_eq(u.isA(), false);
    expect_eq(u.isB(), false);
    expect_eq(u.asA() == null, true);
    expect_eq(u.asB() == null, true);

    u.setA(&a);
    expect_eq(u.isEmpty(), false);
    expect_eq(u.isA(), true);
    expect_eq(u.isB(), false);
    expect_eq(u.asB() == null, true);
    if (pa := u.asA()) {
        unsafe { pa.id = 77; }
    } else {
        expect_eq(true, false);
    }
    expect_eq(a.id, @as(u32, 77));

    // 通过 u64 序列化后再恢复
    const raw_a = u.toU64();
    const u_restored = TaggedPtr2<NodeA, NodeB>.fromU64(raw_a);
    expect_eq(u_restored.isA(), true);
    expect_eq(u_restored.asB() == null, true);

    u.setB(&b);
    expect_eq(u.isA(), false);
    expect_eq(u.isB(), true);
    expect_eq(u.asA() == null, true);
    if (pb := u.asB()) {
        unsafe { pb.score = 888; }
    } else {
        expect_eq(true, false);
    }
    expect_eq(b.score, @as(i64, 888));

    u.clear();
    expect_eq(u.isEmpty(), true);
    expect_eq(u.asA() == null, true);
    expect_eq(u.asB() == null, true);
}

test("TaggedPtr4 four-way safe dispatch and mutual exclusion") {
    let a = NodeA{ id: 1 };
    let b = NodeB{ score: 2 };
    let c = NodeC{ flag: true };
    let d = NodeD{ weight: 3.5 };

    let u = TaggedPtr4<NodeA, NodeB, NodeC, NodeD>.fromA(&a);
    expect_eq(u.isA(), true);
    expect_eq(u.asB() == null && u.asC() == null && u.asD() == null, true);
    expect_eq(u.tag(), @as(u15, 1));

    u.setB(&b);
    expect_eq(u.isB(), true);
    expect_eq(u.asA() == null && u.asC() == null && u.asD() == null, true);
    expect_eq(u.tag(), @as(u15, 2));

    u.setC(&c);
    expect_eq(u.isC(), true);
    expect_eq(u.asA() == null && u.asB() == null && u.asD() == null, true);
    if (pc := u.asC()) {
        unsafe { pc.flag = false; }
    } else {
        expect_eq(true, false);
    }
    expect_eq(c.flag, false);

    u.setD(&d);
    expect_eq(u.isD(), true);
    expect_eq(u.asA() == null && u.asB() == null && u.asC() == null, true);
    if (pd := u.asD()) {
        unsafe { expect_eq(pd.weight, @as(f64, 3.5)); }
    } else {
        expect_eq(true, false);
    }

    u.clear();
    expect_eq(u.isEmpty(), true);
}

test("Const pointer tagging and full 15-bit tag boundary sweep") {
    const item = NodeA{ id: 0xCAFE };
    const tags: [7]u15 = [0, 1, 15, 255, 1024, 16383, MAX_TAG];

    let i: usize = 0;
    while (i < tags.len) {
        const tp = TaggedPointer.fromConstPtr(NodeA, &item, tags[i]);
        expect_eq(tp.tag(), tags[i]);
        expect_eq(tp.isNull(), false);
        if (cp := tp.getConstPtr(NodeA)) {
            unsafe { expect_eq(cp.id, @as(u32, 0xCAFE)); }
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }
}
