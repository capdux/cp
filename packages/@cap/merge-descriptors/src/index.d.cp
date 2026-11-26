// Type definitions for CP

export fn mgdStrEq(a: string, b: string): bool;
export class PropDescriptor {
    key: string,
    value: string,
    writable: bool,
    enumerable: bool,
    configurable: bool,
    fn data(key: string, value: string): PropDescriptor;
    fn readonly(key: string, value: string): PropDescriptor;
}

export fn mgdEmptyDesc(): PropDescriptor;
export class DescriptorTable {
    entries: [32]PropDescriptor,
    count: usize,
    fn new(): DescriptorTable;
    fn len(): usize;
    fn has(key: string): bool;
    fn get(key: string): PropDescriptor?;
    fn define(_: auto, desc: PropDescriptor): bool;
    fn deleteKey(_: auto, key: string): bool;
}

export fn mergeDescriptors(dst: &mut DescriptorTable, src: DescriptorTable, redefine: bool): usize;
export fn countEnumerable(tbl: DescriptorTable): usize;
