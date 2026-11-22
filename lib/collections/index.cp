// Source: cp/lib/collections/index.cp (cap:collections unified export)
export { CollectionError } from "./errors.cp";
export { BitSet } from "./bitset.cp";
export { BoundedDeque, RingDeque } from "./deque.cp";
export { SmallVec } from "./smallVec.cp";
export { BinaryHeap } from "./binaryHeap.cp";
export {
    IDENTITY_TABLE_SLOTS,
    IdentityMapError,
    IdentityContextU64,
    IdentityContextU32,
    ArrayIdentityContext,
    ArrayIdentityContextU64,
    identityBucketIndexU64,
    identityBucketIndexU32,
    IdentitySetU64,
    IdentityMapU64,
} from "./identityContext.cp";
