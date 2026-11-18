// Source: bun/src/safety/safety.cp
// CP Standard Library: cap:safety
// High-performance thread safety, race condition detection, and ASAN integration
import std from "std";

import { invalid, getCurrent, isCurrent, isValid } from "./thread_id.cp";
import { ThreadLock } from "./ThreadLock.cp";
import { CriticalSection } from "./CriticalSection.cp";
import { CheckedAllocator, assertEq, assertAllocatorEq, assertEqFmt } from "./alloc.cp";
import {
    poison,
    unpoison,
    isPoisoned,
    assertPoisoned,
    assertUnpoisoned,
    describe,
    describeAddress,
    registerRootRegion,
    unregisterRootRegion,
} from "./asan.cp";
import { assert, safety_assert, safety_panic, unreachable_code } from "./safety.cp";

export {
    // Thread ID utilities
    invalid,
    getCurrent,
    isCurrent,
    isValid,

    // Thread locking and race detection
    ThreadLock,
    CriticalSection,

    // Allocator validation
    CheckedAllocator,
    assertEq,
    assertAllocatorEq,
    assertEqFmt,

    // AddressSanitizer integration
    poison,
    unpoison,
    isPoisoned,
    assertPoisoned,
    assertUnpoisoned,
    describe,
    describeAddress,
    registerRootRegion,
    unregisterRootRegion,

    // Safety assertions
    assert,
    safety_assert,
    safety_panic,
    unreachable_code,
};
