// Type definitions for CP

export fn ommEmptyStrings(): [OMM_CAP]string;
export fn ommStrEq(a: string, b: string): bool;
export class OrderedMultiMap {
    keys: [OMM_CAP]string,
    vals: [OMM_CAP]string,
    count: usize,
    fn new(): OrderedMultiMap;
    fn append(_: auto, key: string, val: string): bool;
    fn insert(_: auto, key: string, val: string): bool;
    fn getFirst(key: string): string?;
    fn getAll(key: string, out: []string): usize;
    fn removeAll(_: auto, key: string): usize;
    fn containsKey(key: string): bool;
    fn len(): usize;
    fn keysLen(): usize;
}

