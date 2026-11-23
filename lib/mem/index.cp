// Source: Absorbed from /data/dev/bun/src/bun_alloc/bun_alloc.cp | Synced: 2026-11-22T13:46:56Z
// lib/mem/index.cp
// Unified Cap Memory & Smart Pointer Subsystem (cap:mem)

export {
    MiHeap,
    mi_malloc,
    mi_calloc,
    mi_realloc,
    mi_expand,
    mi_free,
    mi_malloc_small,
    mi_zalloc_small,
    mi_zalloc,
    mi_mallocn,
    mi_reallocn,
    mi_reallocf,
    mi_usable_size,
    mi_good_size,
    mi_collect,
    mi_version,
    mi_stats_reset,
    mi_stats_merge,
    mi_process_init,
    mi_thread_init,
    mi_thread_done,
    mi_malloc_aligned,
    mi_zalloc_aligned,
    mi_calloc_aligned,
    mi_realloc_aligned,
    mi_heap_new,
    mi_heap_delete,
    mi_heap_destroy,
    mi_heap_main,
    mi_heap_contains,
    mi_heap_collect,
    mi_heap_malloc,
    mi_heap_zalloc,
    mi_heap_calloc,
    mi_heap_realloc,
    mi_heap_malloc_aligned,
} from "./_mimalloc.cp";

export {
    Arena,
    get_arena,
    SharedTensor,
    StackFallbackAllocator,
    BumpArena,
    MaybeOwnedBuf,
    MaxHeapAllocator,
} from "./allocator.cp";

export {
    TAG_SHIFT,
    ADDR_MASK,
    LOW_TAG_MASK,
    TaggedPointer,
    LowTagPointer,
    TaggedPtr,
} from "./taggedPtr.cp";

export {
    CowBytes,
    CowSlice,
    Cow,
} from "./cow.cp";

export {
    DecrementResult,
    RefCounter,
    AtomicRefCounter,
    WeakControlBlock,
    Rc,
} from "./rc.cp";

export {
    isPowerOfTwo,
    alignForward,
    alignBackward,
    alignDelta,
    secureZero,
} from "./primitives.cp";

export {
    FixedBufferAllocator,
} from "./fixedBuffer.cp";

export {
    SlabPool,
} from "./pool.cp";

export {
    ScopeError,
    ScopeStats,
    TrackedAllocation,
    AllocationScope,
} from "./allocationScope.cp";

export {
    FallbackAllocError,
    BufferFallbackAllocator,
    MaybeOwned,
    MemFdError,
    MemFdBuffer,
} from "./fallbackAllocator.cp";

export {
    EXTERNAL_SLICE_INVALID_SENTINEL,
    ExternalSlice,
    ExternalByteSlice,
    ExternalSliceMap,
    ExternalBytePool,
} from "./externalSlice.cp";

export {
    NullableAllocError,
    AllocOrigin,
    NullableBump,
    OwnedOrBorrowed,
} from "./nullable.cp";

// === W29-Sync R4: Consolidated Allocators, Tagged Pointers & Layout Padding Checker ===
export {
    BufferFallbackAllocator as StackBufferFallbackAllocator,
} from "./fallbackAlloc.cp";

export {
    BumpAllocator,
} from "./bumpAlloc.cp";

export {
    PoolAllocator,
} from "./poolAlloc.cp";

export {
    TrackingAllocator,
} from "./trackingAlloc.cp";

export {
    ScratchArena,
    Checkpoint,
} from "./scratchArena.cp";

export {
    HEAP_ZONE_PREFIX,
    HeapBreakdownError,
    heapZoneBaseName,
    formatHeapZoneLabel,
    HeapZoneHandle,
    ZoneStats,
    HeapBreakdown,
} from "./heapBreakdown.cp";

export {
    Address,
    Tag,
    PTR_BITS,
    TAG_BITS,
    PTR_MASK,
    MAX_TAG,
    TaggedPointer as HighBitTaggedPointer,
    TaggedPtr2,
    TaggedPtr4,
} from "./highBitTaggedPtr.cp";

export {
    paddingBytes,
    hasPadding,
    hasPointers,
    isSafeBinarySerializable,
    assertNoUninitializedPadding,
} from "./paddingChecker.cp";
