// cap/test/test_ptr.cp
// Comprehensive test suite for cap:ptr smart pointer and memory abstraction standard library

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Shared,
    AtomicShared,
    Weak,
    AtomicWeak,
    Cow,
    CowSlice,
    CowString,
    Owned,
    TaggedPointer,
    LowTagPointer,
    RawRefCount,
    ThreadSafety,
    DecrementResult,
    RefCount,
    RefCountOptions,
    RefPtr,
} from "cap:ptr";

type PtrI32 = *i32;
type PtrU64 = *u64;
type PtrI64 = *i64;
type SliceU8 = []u8;

struct TestNode {
    id: i32 = 0,
    ref_count: RefCount(TestNode, "ref_count", TestNode.destroy, RefCountOptions{}) = RefCount(TestNode, "ref_count", TestNode.destroy, RefCountOptions{}).init(),

    pub static fn destroy(node: *TestNode) {
        std.heap.page_allocator.destroy(node);
    }
}

fn test_shared_lifecycle(): !void {
    let s1 = Shared(PtrI32).new(42);
    expect_eq(s1.get().*, 42);
    expect_eq(s1.strongCount() as i64, 1);
    expect_eq(s1.isUnique(), true);

    // Cloning increments strong count
    let s2 = s1.clone();
    expect_eq(s1.strongCount() as i64, 2);
    expect_eq(s2.strongCount() as i64, 2);
    expect_eq(s1.isUnique(), false);

    // Mutating through pointer affects shared data
    s2.get().* = 100;
    expect_eq(s1.get().*, 100);

    // Weak pointer clone & upgrade
    let w = s1.cloneWeak();
    expect_eq(s1.weakCount() as i64, 1);

    let upgraded_opt = w.upgrade();
    expect_eq(upgraded_opt != null, true);
    if (upgraded := upgraded_opt) {
        let mut_up = upgraded;
        expect_eq(mut_up.get().*, 100);
        expect_eq(mut_up.strongCount() as i64, 3);
        mut_up.deinit();
    }

    s1.deinit();
    expect_eq(s2.strongCount() as i64, 1);
    s2.deinit();

    // After all strong references are dropped, upgrade must fail (returns null)
    let dead_upgrade = w.upgrade();
    expect_eq(dead_upgrade == null, true);
    w.deinit();
}

fn test_atomic_shared(): !void {
    let a1 = AtomicShared(PtrU64).new(777);
    expect_eq(a1.get().*, 777);
    expect_eq(a1.strongCount() as i64, 1);
    expect_eq(a1.isUnique(), true);

    let a2 = a1.clone();
    expect_eq(a1.strongCount() as i64, 2);
    expect_eq(a2.strongCount() as i64, 2);
    expect_eq(a1.isUnique(), false);

    a2.get().* = 888;
    expect_eq(a1.get().*, 888);

    let w = a1.cloneWeak();
    expect_eq(a1.weakCount() as i64, 1);

    let up = w.upgrade();
    expect_eq(up != null, true);
    if (strong := up) {
        let mut_strong = strong;
        expect_eq(mut_strong.get().*, 888);
        mut_strong.deinit();
    }

    a1.deinit();
    a2.deinit();

    let dead = w.upgrade();
    expect_eq(dead == null, true);
    w.deinit();
}

fn test_cow_container(): !void {
    let val: i32 = 123;
    let cow = Cow(i32).borrow(&val);
    expect_eq(cow.isOwned(), false);
    expect_eq(cow.inner().*, 123);

    // Mutable borrow is null while borrowed
    expect_eq(cow.innerMut() == null, true);

    // Convert to owned triggers copy
    let ptr = cow.toOwned(std.heap.page_allocator);
    expect_eq(cow.isOwned(), true);
    expect_eq(cow.innerMut() != null, true);

    // Mutating owned copy does not affect original stack variable
    ptr.* = 456;
    expect_eq(cow.inner().*, 456);
    expect_eq(val, 123);

    cow.deinit(std.heap.page_allocator);
}

fn test_cow_slice(): !void {
    let static_str = CowSlice(u8).initStatic("antigravity");
    expect_eq(static_str.isOwned(), false);
    expect_eq(static_str.length() as i64, 11);
    expect_eq(static_str.slice(), "antigravity");

    // Subslice borrowing
    let sub = static_str.borrowSubslice(0, 4);
    expect_eq(sub.isOwned(), false);
    expect_eq(sub.length() as i64, 4);
    expect_eq(sub.slice(), "anti");

    // Mutate subslice triggers intoOwned allocation
    let mut_slice = sub.sliceMut(std.heap.page_allocator) catch unreachable;
    expect_eq(sub.isOwned(), true);
    mut_slice[0] = 'A';
    expect_eq(sub.slice(), "Anti");

    // Original static string remains untouched
    expect_eq(static_str.slice(), "antigravity");

    sub.deinit(std.heap.page_allocator);
}

fn test_owned_pointer(): !void {
    let o_single = Owned(PtrI64).new(987654321);
    expect_eq(o_single.get().*, 987654321);
    o_single.get().* = 111222333;
    expect_eq(o_single.get().*, 111222333);
    o_single.deinit();

    // Owned slice
    let o_slice = Owned(SliceU8).allocIn(8, 0xFF, std.heap.page_allocator) catch unreachable;
    const s_val = o_slice.get();
    expect_eq(s_val.len as i64, 8);
    expect_eq(s_val[0] as i64, 0xFF);
    o_slice.deinit();
}

fn test_tagged_pointer(): !void {
    let target_var: u64 = 0xCAFEBABE;
    let tp = TaggedPointer.fromPtr(&target_var, 0x1234);
    expect_eq(tp.tag() as i64, 0x1234);
    expect_eq(tp.isNull(), false);
    expect_eq(tp.get(u64).*, 0xCAFEBABE);

    // Changing tag preserves address
    let tp2 = tp.withTag(0x5678);
    expect_eq(tp2.tag() as i64, 0x5678);
    expect_eq(tp2.get(u64).*, 0xCAFEBABE);

    // Low 3-bit tagged pointer
    let lp = LowTagPointer.fromPtr(&target_var, 0x5);
    expect_eq(lp.tag() as i64, 5);
    expect_eq(lp.isNull(), false);
    expect_eq(lp.get(u64).*, 0xCAFEBABE);

    let lp2 = lp.withTag(0x2);
    expect_eq(lp2.tag() as i64, 2);
    expect_eq(lp2.get(u64).*, 0xCAFEBABE);
}

fn test_ref_count(): !void {
    let init_node = TestNode{
        id: 999,
        ref_count: RefCount(TestNode, "ref_count", TestNode.destroy, RefCountOptions{}).init(),
    };
    let node_ptr = RefPtr(TestNode).new(init_node);
    let node_ref = node_ptr.get();
    expect_eq(node_ref.id, 999);
    expect_eq(node_ref.ref_count.get() as i64, 1);
    expect_eq(node_ref.ref_count.hasOneRef(), true);

    // Duplicating ref
    let node_dup = node_ptr.dupeRef();
    expect_eq(node_ptr.get().ref_count.get() as i64, 2);
    expect_eq(node_dup.get().id, 999);

    // Dereferencing
    node_ptr.deref();
    expect_eq(node_dup.get().ref_count.get() as i64, 1);
    node_dup.deref();
}

fn test_raw_ref_count(): !void {
    let raw = RawRefCount(u32, ThreadSafety.single_threaded).init(1);
    expect_eq(raw.count() as i64, 1);
    expect_eq(raw.isUnique(), true);

    raw.increment();
    expect_eq(raw.count() as i64, 2);
    expect_eq(raw.isUnique(), false);

    let dec1 = raw.decrement();
    expect_eq(dec1 == DecrementResult.keep_alive, true);
    expect_eq(raw.count() as i64, 1);

    let dec2 = raw.decrement();
    expect_eq(dec2 == DecrementResult.should_destroy, true);
}

fn main() {
    test("1. cap:ptr — Shared<T> & Weak<T> single-threaded lifecycle", () => {
        test_shared_lifecycle()?;
    });

    test("2. cap:ptr — AtomicShared<T> thread-safe reference counting", () => {
        test_atomic_shared()?;
    });

    test("3. cap:ptr — Cow<T> copy-on-write value container", () => {
        test_cow_container()?;
    });

    test("4. cap:ptr — CowSlice<T> & CowString slice sharing & mutation", () => {
        test_cow_slice()?;
    });

    test("5. cap:ptr — Owned<T> single ownership pointer & slice", () => {
        test_owned_pointer()?;
    });

    test("6. cap:ptr — TaggedPointer & LowTagPointer bit packing", () => {
        test_tagged_pointer()?;
    });

    test("7. cap:ptr — RefCount & RefPtr intrusive reference counting", () => {
        test_ref_count()?;
    });

    test("8. cap:ptr — RawRefCount low-level counter", () => {
        test_raw_ref_count()?;
    });
}
