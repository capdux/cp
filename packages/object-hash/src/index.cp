// @cap/object-hash — Zero-allocation deterministic order-independent object hasher in pure CP
// Clean-room MIT implementation inspired by npm `object-hash`.

pub const OHSH_KIND_NULL: u8 = 0;
pub const OHSH_KIND_BOOL: u8 = 1;
pub const OHSH_KIND_NUMBER: u8 = 2;
pub const OHSH_KIND_STRING: u8 = 3;
pub const OHSH_MAX_SORT_FIELDS: usize = 128;

/// A typed key-value field for canonical object hashing (`kind`: 0=null, 1=bool, 2=number, 3=string).
pub struct HashField {
    key: string,
    value: string,
    kind: u8,
}

fn ohshFnvOffset(): u64 {
    return ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
}

fn ohshFnvPrime(): u64 {
    return ((0x00000100 as u64) << 32) | (0x000001b3 as u64);
}

fn ohshFeedByte(h: u64, b: u8): u64 {
    let next: u64 = h ^ (b as u64);
    next *%= ohshFnvPrime();
    return next;
}

fn ohshFeedBytes(h: u64, data: []const u8): u64 {
    let cur: u64 = h;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        cur = ohshFeedByte(cur, data[i]);
    }
    return cur;
}

fn ohshCmpStr(a: string, b: string): i32 {
    const min_len: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

fn ohshFieldLess(a: HashField, b: HashField): bool {
    const kc: i32 = ohshCmpStr(a.key, b.key);
    if (kc != 0) {
        return kc < 0;
    }
    if (a.kind != b.kind) {
        return a.kind < b.kind;
    }
    return ohshCmpStr(a.value, b.value) < 0;
}

fn ohshHexDigit(nibble: u8): u8 {
    if (nibble < 10) {
        return '0' + nibble;
    }
    return 'a' + (nibble - 10);
}

/// Compute FNV-1a 64-bit hash of a raw byte slice.
pub fn hashBytesFnv64(data: []const u8): u64 {
    return ohshFeedBytes(ohshFnvOffset(), data);
}

/// Compute FNV-1a 64-bit hash of a string.
pub fn hashStringFnv64(s: string): u64 {
    return ohshFeedBytes(ohshFnvOffset(), s);
}

/// Compute an order-independent FNV-1a 64-bit hash over structured `HashField` entries.
/// Fields are sorted lexicographically by `key` (then `kind`, `value`) and fed as `kind:key=value`.
pub fn hashFieldsFnv64(fields: []const HashField): u64 {
    const n: usize = if (fields.len <= OHSH_MAX_SORT_FIELDS) fields.len else OHSH_MAX_SORT_FIELDS;
    let order: [128]usize = undefined;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        order[i] = i;
    }

    // Insertion sort on index permutation (zero heap allocation, stable)
    let j: usize = 1;
    while (j < n) : (j += 1) {
        const cur_idx: usize = order[j];
        let k: usize = j;
        while (k > 0 && ohshFieldLess(fields[cur_idx], fields[order[k - 1]])) : (k -= 1) {
            order[k] = order[k - 1];
        }
        order[k] = cur_idx;
    }

    let h: u64 = ohshFnvOffset();
    let m: usize = 0;
    while (m < n) : (m += 1) {
        const f: HashField = fields[order[m]];
        if (m > 0) {
            h = ohshFeedByte(h, 0x1F);
        }
        h = ohshFeedByte(h, '0' + (f.kind % 10));
        h = ohshFeedByte(h, ':');
        h = ohshFeedBytes(h, f.key);
        h = ohshFeedByte(h, '=');
        h = ohshFeedBytes(h, f.value);
    }
    return h;
}

/// Compute order-independent FNV-1a 64-bit hash and write 16 lowercase hex chars into `out`.
/// Returns the number of bytes written (16 when `out.len >= 16`).
pub fn hashFieldsHex(fields: []const HashField, out: []u8): usize {
    const h: u64 = hashFieldsFnv64(fields);
    const limit: usize = if (out.len < 16) out.len else 16;
    let i: usize = 0;
    while (i < limit) : (i += 1) {
        const shift: u6 = ((15 - i) * 4) as u6;
        const nibble: u8 = ((h >> shift) & 0x0F) as u8;
        out[i] = ohshHexDigit(nibble);
    }
    return limit;
}
