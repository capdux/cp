// Type definitions for CP

export fn strEq(a: string, b: string): bool;
export class MultiEntry {
    key: string,
    value: string,
}

export class MultiMapStr {
    entries: [MULTIMAP_CAP]MultiEntry,
    count: usize,
    fn init(): MultiMapStr;
    fn totalValues(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
    fn insert(_: auto, key: string, value: string): bool;
    fn getFirst(key: string): string?;
    fn getLast(key: string): string?;
    fn getNth(key: string, n: usize): string?;
    fn countFor(key: string): usize;
    fn containsKey(key: string): bool;
    fn uniqueKeyCount(): usize;
    fn removeKey(_: auto, key: string): usize;
    fn removePair(_: auto, key: string, value: string): bool;
    fn entryAt(idx: usize): MultiEntry?;
}

