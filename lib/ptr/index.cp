// Source: Absorbed from /data/dev/bun/src/ptr/ptr.cp | Synced: 2026-11-19T20:42:44Z
// CP Standard Library: cap:ptr
// High-performance smart pointer and memory management abstractions

export {
    PointerKind,
    PointerInfo,
    AddConst,
} from "./meta.cp";

export {
    ThreadSafety,
    DecrementResult,
    PtrThreadLock,
    RawRefCount,
} from "./raw_ref_count.cp";

export {
    RefCount,
    ThreadSafeRefCount,
    RefPtr,
    RefCountOptions,
} from "./ref_count.cp";

export {
    WeakPtrData,
    WeakPtr,
} from "./weak_ptr.cp";

export {
    Shared,
    AtomicShared,
    Weak,
    AtomicWeak,
    WithOptions,
    WeakWithOptions,
    AtomicCount,
    NonAtomicCount,
    SharedOptions,
} from "./shared.cp";

export {
    Cow,
    CowWithVTable,
    DefaultVTable,
} from "./cow.cp";

export {
    CowSlice,
    CowSliceZ,
    CowString,
    CowSliceFlags,
} from "./cow_slice.cp";

export {
    Owned,
    Dynamic,
    OwnedIn,
} from "./owned.cp";

export {
    TaggedPointer,
    LowTagPointer,
    TAG_SHIFT,
    ADDR_MASK,
    TAG_MASK,
    LOW_TAG_MASK,
} from "./tagged_pointer.cp";

export {
    ExternalShared,
} from "./external_shared.cp";
