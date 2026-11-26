// Type definitions for CP

export class Range {
    start: i64,
    end: i64,
}

export class RangeSet {
    ranges: [MAX_RANGES]Range,
    count: usize,
    fn init(): RangeSet;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn get(idx: usize): Range;
    fn insert(_: auto, start: i64, end: i64): bool;
    fn remove(_: auto, start: i64, end: i64): bool;
    fn lowerBoundEnd(value: i64): usize;
    fn contains(point: i64): bool;
    fn containsRange(start: i64, end: i64): bool;
    fn overlaps(start: i64, end: i64): bool;
    fn totalSpan(): i64;
}

