// @cap/depd — Zero-heap deprecation notice formatter & deduplicated warning tracker
// Matching npm depd (150M+/wk). Clean-room MIT in pure CP.

import { fnv64OffsetBasis, mulFnv64Prime } from "../../@sindresorhus/fnv1a/src/index.cp";

fn depdHashKey(ns: string, msg: string): u64 {
    let h: u64 = fnv64OffsetBasis();
    let i: usize = 0;
    while (i < ns.len) {
        h = mulFnv64Prime(h ^ (ns[i] as u64));
        i = i + 1;
    }
    h = mulFnv64Prime(h ^ 58);
    let j: usize = 0;
    while (j < msg.len) {
        h = mulFnv64Prime(h ^ (msg[j] as u64));
        j = j + 1;
    }
    return h;
}

fn copySlice(dst: []u8, pos: usize, src: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < src.len && p < dst.len) {
        dst[p] = src[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

pub struct Deprecator {
    pub namespace: string,
    pub silenced: bool,
    pub seen_hashes: [64]u64,
    pub seen_count: usize,
    pub emitted_count: u64,

    pub static fn new(namespace: string): Deprecator {
        return Deprecator {
            namespace: namespace,
            silenced: false,
            seen_hashes: [0; 64],
            seen_count: 0,
            emitted_count: 0,
        };
    }

    pub fn setSilenced(*_, flag: bool) {
        _.silenced = flag;
    }

    pub fn hasWarned(msg: string): bool {
        let h = depdHashKey(_.namespace, msg);
        let i: usize = 0;
        while (i < _.seen_count) {
            if (_.seen_hashes[i] == h) { return true; }
            i = i + 1;
        }
        return false;
    }

    // Formats "<namespace> deprecated <msg> at <site>" once per unique message.
    // Returns 0 if silenced or already warned for this message.
    pub fn warnOnce(*_, msg: string, site: string, out: []u8): usize {
        if (_.silenced) { return 0; }
        let h = depdHashKey(_.namespace, msg);
        let i: usize = 0;
        while (i < _.seen_count) {
            if (_.seen_hashes[i] == h) { return 0; }
            i = i + 1;
        }
        if (_.seen_count < 64) {
            _.seen_hashes[_.seen_count] = h;
            _.seen_count = _.seen_count + 1;
        }
        _.emitted_count = _.emitted_count + 1;
        return formatDeprecation(_.namespace, msg, site, out);
    }

    pub fn reset(*_) {
        _.seen_count = 0;
        _.emitted_count = 0;
    }
}

pub fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize {
    let p: usize = 0;
    p = copySlice(out, p, namespace);
    p = copySlice(out, p, " deprecated ");
    p = copySlice(out, p, msg);
    if (site.len > 0) {
        p = copySlice(out, p, " at ");
        p = copySlice(out, p, site);
    }
    return p;
}
