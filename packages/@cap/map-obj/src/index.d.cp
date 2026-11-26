// Type definitions for CP

export class ObjEntry {
    key: string,
    val: string,
    active: bool,
}

export class MapObjResult {
    entries: [32]ObjEntry,
    count: usize,
    fn empty(): MapObjResult;
    fn push(_: auto, k: string, v: string): bool;
    fn get(k: string): string?;
}

export fn mobjStrEq(a: string, b: string): bool;
export fn swapKeysAndValues(src: MapObjResult): MapObjResult;
export fn pickKeys(src: MapObjResult, allow_keys: []string): MapObjResult;
export fn omitKeys(src: MapObjResult, skip_keys: []string): MapObjResult;
export fn filterEmptyValues(src: MapObjResult): MapObjResult;
export fn mergeObjects(base: MapObjResult, overrides: MapObjResult): MapObjResult;
