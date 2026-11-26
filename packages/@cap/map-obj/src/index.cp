pub const MAP_OBJ_MAX_ENTRIES: usize = 32;

pub const MAP_KEEP: u8 = 0;
pub const MAP_SKIP: u8 = 1;

pub struct ObjEntry {
    pub key: string,
    pub val: string,
    pub active: bool,
}

pub struct MapObjResult {
    pub entries: [32]ObjEntry,
    pub count: usize,

    pub static fn empty(): MapObjResult {
        let blank = ObjEntry { key: "", val: "", active: false };
        let arr: [32]ObjEntry = [blank; 32];
        return MapObjResult { entries: arr, count: 0 };
    }

    pub fn push(*_, k: string, v: string): bool {
        if (_.count >= MAP_OBJ_MAX_ENTRIES) { return false; }
        _.entries[_.count] = ObjEntry { key: k, val: v, active: true };
        _.count = _.count + 1;
        return true;
    }

    pub fn get(k: string): string? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].active && mobjStrEq(_.entries[i].key, k)) {
                return _.entries[i].val;
            }
            i = i + 1;
        }
        return null;
    }
}

fn mobjStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

pub fn swapKeysAndValues(src: MapObjResult): MapObjResult {
    let out = MapObjResult.empty();
    let i: usize = 0;
    while (i < src.count) {
        if (src.entries[i].active) {
            * = out.push(src.entries[i].val, src.entries[i].key);
        }
        i = i + 1;
    }
    return out;
}

pub fn pickKeys(src: MapObjResult, allow_keys: []string): MapObjResult {
    let out = MapObjResult.empty();
    let i: usize = 0;
    while (i < src.count) {
        if (src.entries[i].active) {
            let k = src.entries[i].key;
            let j: usize = 0;
            let matched = false;
            while (j < allow_keys.len) {
                if (mobjStrEq(k, allow_keys[j])) {
                    matched = true;
                    break;
                }
                j = j + 1;
            }
            if (matched) {
                * = out.push(k, src.entries[i].val);
            }
        }
        i = i + 1;
    }
    return out;
}

pub fn omitKeys(src: MapObjResult, skip_keys: []string): MapObjResult {
    let out = MapObjResult.empty();
    let i: usize = 0;
    while (i < src.count) {
        if (src.entries[i].active) {
            let k = src.entries[i].key;
            let j: usize = 0;
            let excluded = false;
            while (j < skip_keys.len) {
                if (mobjStrEq(k, skip_keys[j])) {
                    excluded = true;
                    break;
                }
                j = j + 1;
            }
            if (!excluded) {
                * = out.push(k, src.entries[i].val);
            }
        }
        i = i + 1;
    }
    return out;
}

pub fn filterEmptyValues(src: MapObjResult): MapObjResult {
    let out = MapObjResult.empty();
    let i: usize = 0;
    while (i < src.count) {
        if (src.entries[i].active && src.entries[i].val.len > 0) {
            * = out.push(src.entries[i].key, src.entries[i].val);
        }
        i = i + 1;
    }
    return out;
}

pub fn mergeObjects(base: MapObjResult, overrides: MapObjResult): MapObjResult {
    let out = MapObjResult.empty();
    let i: usize = 0;
    while (i < base.count) {
        if (base.entries[i].active) {
            * = out.push(base.entries[i].key, base.entries[i].val);
        }
        i = i + 1;
    }
    let j: usize = 0;
    while (j < overrides.count) {
        if (overrides.entries[j].active) {
            let k = overrides.entries[j].key;
            let v = overrides.entries[j].val;
            let updated = false;
            let m: usize = 0;
            while (m < out.count) {
                if (mobjStrEq(out.entries[m].key, k)) {
                    out.entries[m].val = v;
                    updated = true;
                    break;
                }
                m = m + 1;
            }
            if (!updated) {
                * = out.push(k, v);
            }
        }
        j = j + 1;
    }
    return out;
}
