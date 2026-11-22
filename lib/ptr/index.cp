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
} from "./rawRefCount.cp";

export {
    RefCount,
    ThreadSafeRefCount,
    RefPtr,
    RefCountOptions,
} from "./refCount.cp";

export {
    WeakPtrData,
    WeakPtr,
} from "./weakPtr.cp";

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
} from "./cowSlice.cp";

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
} from "./taggedPointer.cp";

export {
    ExternalShared,
} from "./externalShared.cp";
