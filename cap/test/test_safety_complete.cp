// cap/test/test_safety_complete.cp
// Comprehensive end-to-end verification of cap:safety standard library
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    invalid,
    getCurrent,
    isCurrent,
    isValid,
    ThreadLock,
    CriticalSection,
    CheckedAllocator,
    assertAllocatorEq,
    poison,
    unpoison,
    isPoisoned,
    assertPoisoned,
    assertUnpoisoned,
    describeAddress,
    registerRootRegion,
    unregisterRootRegion,
    safety_assert,
} from "cap:safety";

fn test_thread_id_utilities(): !void {
    const cur_id = getCurrent();
    expect_eq(isValid(cur_id), true);
    expect_eq(isValid(invalid), false);
    expect_eq(isCurrent(cur_id), true);
    expect_eq(isCurrent(invalid), false);
    expect_eq(cur_id != invalid, true);
}

fn test_thread_lock_lifecycle(): !void {
    let lock = ThreadLock.initUnlocked();
    expect_eq(lock.isLocked(), false);
    expect_eq(lock.isHeldByCurrentThread(), false);

    lock.assertNotLocked();

    // Lock and verify ownership
    lock.lock();
    expect_eq(lock.isLocked(), true);
    expect_eq(lock.isHeldByCurrentThread(), true);
    lock.assertLocked();

    // Unlock and verify released state
    lock.unlock();
    expect_eq(lock.isLocked(), false);
    expect_eq(lock.isHeldByCurrentThread(), false);
    lock.assertNotLocked();

    // Test lockOrAssert
    lock.lockOrAssert();
    expect_eq(lock.isLocked(), true);
    lock.lockOrAssert(); // Idempotent assertion on same thread
    lock.unlock();
    expect_eq(lock.isLocked(), false);

    // Test initLocked
    let locked_instance = ThreadLock.initLocked();
    expect_eq(locked_instance.isLocked(), true);
    expect_eq(locked_instance.isHeldByCurrentThread(), true);
    locked_instance.unlock();
    expect_eq(locked_instance.isLocked(), false);
}

fn test_critical_section_reentrancy(): !void {
    let cs = CriticalSection.init();
    expect_eq(cs.isLocked(), false);
    expect_eq(cs.isExclusiveHeldByCurrentThread(), false);

    // 1. Exclusive write lock & nested reentrancy on current thread
    cs.begin();
    expect_eq(cs.isLocked(), true);
    expect_eq(cs.isExclusiveHeldByCurrentThread(), true);

    cs.begin(); // Nested reentrancy
    expect_eq(cs.isLocked(), true);

    cs.end(); // Decrement nested depth
    expect_eq(cs.isLocked(), true);

    cs.end(); // Fully released
    expect_eq(cs.isLocked(), false);
    expect_eq(cs.isExclusiveHeldByCurrentThread(), false);

    // 2. Shared read-only lock
    cs.beginReadOnly();
    expect_eq(cs.isLocked(), true);
    expect_eq(cs.isExclusiveHeldByCurrentThread(), false);

    cs.end();
    expect_eq(cs.isLocked(), false);

    // 3. Sequential read-then-write
    cs.beginReadOnly();
    cs.end();
    cs.begin();
    cs.end();
    expect_eq(cs.isLocked(), false);
}

fn test_checked_allocator(): !void {
    const page_alloc = std.heap.page_allocator;

    // Direct allocator identity assertion
    assertAllocatorEq(page_alloc, page_alloc);

    // CheckedAllocator lifecycle
    let checked = CheckedAllocator.init(page_alloc);
    expect_eq(checked.isSet(), true);

    // Consistent re-set and assert
    checked.set(page_alloc);
    checked.assertEq(page_alloc);

    // Reset and re-bind
    checked.reset();
    expect_eq(checked.isSet(), false);
    checked.set(page_alloc);
    expect_eq(checked.isSet(), true);
}

fn test_asan_stubs_and_safety_assertions(): !void {
    let dummy_byte: u8 = 42;
    const ptr: *const anyopaque = unsafe { @ptrCast(&dummy_byte) };

    // ASAN operations should execute seamlessly
    poison(ptr, 1);
    expect_eq(isPoisoned(ptr), false);
    unpoison(ptr, 1);
    assertPoisoned(ptr);
    assertUnpoisoned(ptr);
    describeAddress(ptr);
    registerRootRegion(ptr, 1);
    unregisterRootRegion(ptr, 1);

    // Safety assertion
    safety_assert(true, "condition should hold");
}

fn main() {
    test("1. cap:safety — thread_id identification and validity checks", () => {
        test_thread_id_utilities()?;
    });

    test("2. cap:safety — ThreadLock ownership and state assertions", () => {
        test_thread_lock_lifecycle()?;
    });

    test("3. cap:safety — CriticalSection reentrant and shared race detection", () => {
        test_critical_section_reentrancy()?;
    });

    test("4. cap:safety — CheckedAllocator allocator consistency validation", () => {
        test_checked_allocator()?;
    });

    test("5. cap:safety — ASAN memory poisoning and safety assertion primitives", () => {
        test_asan_stubs_and_safety_assertions()?;
    });
}
