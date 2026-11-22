// Source: Absorbed from /data/dev/bun/src/collections/collections.cp | Synced: 2026-11-19T20:27:32Z
// ============================================================================
// Absorbed from Bun: bun/src/collections/
// Upstream Sync: Refer to bun/src/collections/ for future upstream updates.
// ============================================================================
// lib/col/index.cp
// Unified Public Exports for cap:col (and legacy cap:collections)
// Architecture:
//   - Built-in high-level containers (provided by cpc runtime):
//       VecDeque, BinaryHeap, BTreeMap, BTreeSet, HashMap, HashSet, Map, Set, Dict
//   - bounded_array.cp : Zero-heap stack-allocated BoundedArray<T, CAP>
//   - baby_list.cp     : Compact u32-indexed BabyList<T> and OffsetByteList
//   - linear_fifo.cp   : Power-of-two ring-buffer LinearFifo<T>
//   - hive_array.cp    : Hardware @ctz/@popCount BitSet64 and O(1) HiveArray<T, CAP> slot pool
//   - bit_set.cp       : Range, BitSet64, BitSet128, BitSet256, BitSet512, BitSet1024, DynamicBitSet, Iterators
//   - string_map.cp    : Length-filtered FastStringMap<V, CAP> (exact & ASCII case-insensitive)
//   - object_pool.cp   : Fixed-capacity zero-heap free-list ObjectPool<T, CAP>
//   - hash_map.cp      : Fixed-capacity zero-heap StaticHashMap<K, V, CAP>
//   - soa.cp           : High-performance SoA (Struct of Arrays) Soa<T> with SIMD column slices

export * from "./boundedArray.cp";
export * from "./babyList.cp";
export * from "./linearFifo.cp";
export * from "./hiveArray.cp";
export * from "./bitSet.cp";
export * from "./stringMap.cp";
export * from "./objectPool.cp";
export * from "./hashMap.cp";
export * from "./soa.cp";

export * from "./priorityQueue.cp";
export * from "./leb128.cp";
export * from "./treap.cp";
