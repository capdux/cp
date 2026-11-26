// Type definitions for CP

export class BiPair {
    left: u64,
    right: u64,
}

export class BiMap {
    pairs: [128]BiPair,
    count: usize,
    fn init(): BiMap;
    fn len(): usize;
    fn isEmpty(): bool;
    fn capacity(): usize;
    fn findLeftIndex(left: u64): i64;
    fn findRightIndex(right: u64): i64;
    fn containsLeft(left: u64): bool;
    fn containsRight(right: u64): bool;
    fn getByLeft(left: u64): i64;
    fn getByRight(right: u64): i64;
    fn insert(_: auto, left: u64, right: u64): bool;
    fn insertNoOverwrite(_: auto, left: u64, right: u64): bool;
    fn removeByLeft(_: auto, left: u64): bool;
    fn removeByRight(_: auto, right: u64): bool;
    fn clear(_: auto);
}

