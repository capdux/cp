// Source: Absorbed from /data/dev/bun/src/safety/safety.cp | Synced: 2026-11-20T06:56:21Z
// CP Standard Library: cap:safety
// High-performance thread safety, race condition detection, ASAN integration, CPU features & crash diagnostics
import std from "std";

import { invalid, getCurrent, isCurrent, isValid } from "./threadId.cp";
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
import { CpuFeatures } from "./cpuFeatures.cp";
import { OomPolicy, OomHookFn, OomGuard, CrashReport } from "./crash.cp";

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

    // Hardware CPU feature detection & crash/OOM diagnostics
    CpuFeatures,
    OomPolicy,
    OomHookFn,
    OomGuard,
    CrashReport,
};
