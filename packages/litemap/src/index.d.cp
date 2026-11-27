// Type definitions for CP

export class SearchResult {
    found: bool,
    index: usize,
}

export class StrEntry {
    key: string,
    value: string,
}

export class StrInsertResult {
    ok: bool,
    replaced: bool,
    prev: string,
}

export class U64Entry {
    key: u64,
    value: i64,
}

export class U64InsertResult {
    ok: bool,
    replaced: bool,
    prev: i64,
}

export fn cmpStr(a: string, b: string): i8;
export class LiteMapStr {
    entries: [LITEMAP_CAP]StrEntry,
    count: usize,
    fn init(): LiteMapStr;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
    fn binarySearch(key: string): SearchResult;
    fn get(key: string): string?;
    fn containsKey(key: string): bool;
    fn insert(_: auto, key: string, value: string): StrInsertResult;
    fn remove(_: auto, key: string): string?;
    fn entryAt(idx: usize): StrEntry?;
}

export class LiteMapU64 {
    entries: [LITEMAP_CAP]U64Entry,
    count: usize,
    fn init(): LiteMapU64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
    fn binarySearch(key: u64): SearchResult;
    fn get(key: u64): i64?;
    fn containsKey(key: u64): bool;
    fn insert(_: auto, key: u64, value: i64): U64InsertResult;
    fn remove(_: auto, key: u64): i64?;
    fn entryAt(idx: usize): U64Entry?;
}

