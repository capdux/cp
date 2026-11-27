// @cap/roaring — Zero-allocation Roaring Bitmap (16-bit high-key containers) for CP
// Clean-room MIT implementation inspired by Rust `roaring`.

const MAX_CONTAINERS: usize = 16;
const MAX_CONTAINER_ITEMS: usize = 64;

fn highKey(v: u32) -> u16 {
    return ((v / (65536 as u32)) & (0xFFFF as u32)) as u16;
}

fn lowVal(v: u32) -> u16 {
    return (v & (0xFFFF as u32)) as u16;
}

fn combineKeyVal(hi: u16, lo: u16) -> u32 {
    return ((hi as u32) * (65536 as u32)) | (lo as u32);
}

struct BinSearch {
    found: bool,
    pos: usize,
}

pub struct RoaringContainer {
    pub key: u16,
    pub values: [MAX_CONTAINER_ITEMS]u16,
    pub count: usize,
}

fn emptyContainer() -> RoaringContainer {
    let vals: [MAX_CONTAINER_ITEMS]u16 = undefined;
    let i: usize = 0;
    while (i < MAX_CONTAINER_ITEMS) {
        vals[i] = 0 as u16;
        i += 1;
    }
    return RoaringContainer{
        key: 0 as u16,
        values: vals,
        count: 0 as usize,
    };
}

fn searchU16(arr: [MAX_CONTAINER_ITEMS]u16, count: usize, target: u16) -> BinSearch {
    let lo: usize = 0;
    let hi: usize = count;
    while (lo < hi) {
        const mid = lo + ((hi - lo) / (2 as usize));
        const v = arr[mid];
        if (v == target) {
            return BinSearch{ found: true, pos: mid };
        }
        if (v < target) {
            lo = mid + (1 as usize);
        } else {
            hi = mid;
        }
    }
    return BinSearch{ found: false, pos: lo };
}

fn searchContainer(containers: [MAX_CONTAINERS]RoaringContainer, count: usize, key: u16) -> BinSearch {
    let lo: usize = 0;
    let hi: usize = count;
    while (lo < hi) {
        const mid = lo + ((hi - lo) / (2 as usize));
        const k = containers[mid].key;
        if (k == key) {
            return BinSearch{ found: true, pos: mid };
        }
        if (k < key) {
            lo = mid + (1 as usize);
        } else {
            hi = mid;
        }
    }
    return BinSearch{ found: false, pos: lo };
}

fn unionContainers(a: RoaringContainer, b: RoaringContainer) -> RoaringContainer {
    let out = emptyContainer();
    out.key = a.key;
    let i: usize = 0;
    let j: usize = 0;
    while (i < a.count && j < b.count && out.count < MAX_CONTAINER_ITEMS) {
        if (a.values[i] < b.values[j]) {
            out.values[out.count] = a.values[i];
            out.count += 1;
            i += 1;
        } else if (b.values[j] < a.values[i]) {
            out.values[out.count] = b.values[j];
            out.count += 1;
            j += 1;
        } else {
            out.values[out.count] = a.values[i];
            out.count += 1;
            i += 1;
            j += 1;
        }
    }
    while (i < a.count && out.count < MAX_CONTAINER_ITEMS) {
        out.values[out.count] = a.values[i];
        out.count += 1;
        i += 1;
    }
    while (j < b.count && out.count < MAX_CONTAINER_ITEMS) {
        out.values[out.count] = b.values[j];
        out.count += 1;
        j += 1;
    }
    return out;
}

fn intersectContainers(a: RoaringContainer, b: RoaringContainer) -> RoaringContainer {
    let out = emptyContainer();
    out.key = a.key;
    let i: usize = 0;
    let j: usize = 0;
    while (i < a.count && j < b.count && out.count < MAX_CONTAINER_ITEMS) {
        if (a.values[i] < b.values[j]) {
            i += 1;
        } else if (b.values[j] < a.values[i]) {
            j += 1;
        } else {
            out.values[out.count] = a.values[i];
            out.count += 1;
            i += 1;
            j += 1;
        }
    }
    return out;
}

fn diffContainers(a: RoaringContainer, b: RoaringContainer) -> RoaringContainer {
    let out = emptyContainer();
    out.key = a.key;
    let i: usize = 0;
    let j: usize = 0;
    while (i < a.count && j < b.count && out.count < MAX_CONTAINER_ITEMS) {
        if (a.values[i] < b.values[j]) {
            out.values[out.count] = a.values[i];
            out.count += 1;
            i += 1;
        } else if (b.values[j] < a.values[i]) {
            j += 1;
        } else {
            i += 1;
            j += 1;
        }
    }
    while (i < a.count && out.count < MAX_CONTAINER_ITEMS) {
        out.values[out.count] = a.values[i];
        out.count += 1;
        i += 1;
    }
    return out;
}

pub struct RoaringBitmap {
    pub containers: [MAX_CONTAINERS]RoaringContainer,
    pub container_count: usize,
    pub total_len: u64,

    pub static fn init() -> RoaringBitmap {
        let arr: [MAX_CONTAINERS]RoaringContainer = undefined;
        let i: usize = 0;
        while (i < MAX_CONTAINERS) {
            arr[i] = emptyContainer();
            i += 1;
        }
        return RoaringBitmap{
            containers: arr,
            container_count: 0 as usize,
            total_len: 0 as u64,
        };
    }

    pub fn insert(*_, value: u32) -> bool {
        const hi = highKey(value);
        const lo = lowVal(value);
        const cres = searchContainer(_.containers, _.container_count, hi);

        if (cres.found) {
            const cidx = cres.pos;
            const vres = searchU16(_.containers[cidx].values, _.containers[cidx].count, lo);
            if (vres.found) {
                return false;
            }
            if (_.containers[cidx].count >= MAX_CONTAINER_ITEMS) {
                return false;
            }
            let k = _.containers[cidx].count;
            while (k > vres.pos) {
                _.containers[cidx].values[k] = _.containers[cidx].values[k - (1 as usize)];
                k -= 1;
            }
            _.containers[cidx].values[vres.pos] = lo;
            _.containers[cidx].count += 1;
            _.total_len += 1 as u64;
            return true;
        }

        if (_.container_count >= MAX_CONTAINERS) {
            return false;
        }
        let m = _.container_count;
        while (m > cres.pos) {
            _.containers[m] = _.containers[m - (1 as usize)];
            m -= 1;
        }
        let nc = emptyContainer();
        nc.key = hi;
        nc.values[0] = lo;
        nc.count = 1 as usize;
        _.containers[cres.pos] = nc;
        _.container_count += 1;
        _.total_len += 1 as u64;
        return true;
    }

    pub fn remove(*_, value: u32) -> bool {
        const hi = highKey(value);
        const lo = lowVal(value);
        const cres = searchContainer(_.containers, _.container_count, hi);
        if (!cres.found) {
            return false;
        }
        const cidx = cres.pos;
        const vres = searchU16(_.containers[cidx].values, _.containers[cidx].count, lo);
        if (!vres.found) {
            return false;
        }

        let k = vres.pos;
        const lastIdx = _.containers[cidx].count - (1 as usize);
        while (k < lastIdx) {
            _.containers[cidx].values[k] = _.containers[cidx].values[k + (1 as usize)];
            k += 1;
        }
        _.containers[cidx].count -= 1;
        _.total_len -= 1 as u64;

        // Reclaim empty container slot automatically
        if (_.containers[cidx].count == 0) {
            let m = cidx;
            const lastC = _.container_count - (1 as usize);
            while (m < lastC) {
                _.containers[m] = _.containers[m + (1 as usize)];
                m += 1;
            }
            _.containers[lastC] = emptyContainer();
            _.container_count -= 1;
        }
        return true;
    }

    pub fn contains(value: u32) -> bool {
        const hi = highKey(value);
        const lo = lowVal(value);
        const cres = searchContainer(_.containers, _.container_count, hi);
        if (!cres.found) {
            return false;
        }
        const vres = searchU16(_.containers[cres.pos].values, _.containers[cres.pos].count, lo);
        return vres.found;
    }

    pub fn len() -> u64 {
        return _.total_len;
    }

    pub fn isEmpty() -> bool {
        return _.total_len == (0 as u64);
    }

    pub fn min() -> i64 {
        if (_.container_count == 0 || _.containers[0].count == 0) {
            return -1 as i64;
        }
        const c = _.containers[0];
        return combineKeyVal(c.key, c.values[0]) as i64;
    }

    pub fn max() -> i64 {
        if (_.container_count == 0) {
            return -1 as i64;
        }
        const c = _.containers[_.container_count - (1 as usize)];
        if (c.count == 0) {
            return -1 as i64;
        }
        return combineKeyVal(c.key, c.values[c.count - (1 as usize)]) as i64;
    }

    pub fn rank(value: u32) -> u64 {
        const hi = highKey(value);
        const lo = lowVal(value);
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < _.container_count) {
            const c = _.containers[i];
            if (c.key < hi) {
                acc += c.count as u64;
            } else if (c.key == hi) {
                const vres = searchU16(c.values, c.count, lo);
                if (vres.found) {
                    acc += (vres.pos + (1 as usize)) as u64;
                } else {
                    acc += vres.pos as u64;
                }
                break;
            } else {
                break;
            }
            i += 1;
        }
        return acc;
    }

    pub fn unionWith(other: RoaringBitmap) -> RoaringBitmap {
        let out = RoaringBitmap.init();
        let i: usize = 0;
        let j: usize = 0;
        while (i < _.container_count && j < other.container_count && out.container_count < MAX_CONTAINERS) {
            const ca = _.containers[i];
            const cb = other.containers[j];
            if (ca.key < cb.key) {
                out.containers[out.container_count] = ca;
                out.total_len += ca.count as u64;
                out.container_count += 1;
                i += 1;
            } else if (cb.key < ca.key) {
                out.containers[out.container_count] = cb;
                out.total_len += cb.count as u64;
                out.container_count += 1;
                j += 1;
            } else {
                const merged = unionContainers(ca, cb);
                out.containers[out.container_count] = merged;
                out.total_len += merged.count as u64;
                out.container_count += 1;
                i += 1;
                j += 1;
            }
        }
        while (i < _.container_count && out.container_count < MAX_CONTAINERS) {
            const ca = _.containers[i];
            out.containers[out.container_count] = ca;
            out.total_len += ca.count as u64;
            out.container_count += 1;
            i += 1;
        }
        while (j < other.container_count && out.container_count < MAX_CONTAINERS) {
            const cb = other.containers[j];
            out.containers[out.container_count] = cb;
            out.total_len += cb.count as u64;
            out.container_count += 1;
            j += 1;
        }
        return out;
    }

    pub fn intersectWith(other: RoaringBitmap) -> RoaringBitmap {
        let out = RoaringBitmap.init();
        let i: usize = 0;
        let j: usize = 0;
        while (i < _.container_count && j < other.container_count && out.container_count < MAX_CONTAINERS) {
            const ca = _.containers[i];
            const cb = other.containers[j];
            if (ca.key < cb.key) {
                i += 1;
            } else if (cb.key < ca.key) {
                j += 1;
            } else {
                const inter = intersectContainers(ca, cb);
                if (inter.count > 0) {
                    out.containers[out.container_count] = inter;
                    out.total_len += inter.count as u64;
                    out.container_count += 1;
                }
                i += 1;
                j += 1;
            }
        }
        return out;
    }

    pub fn differenceWith(other: RoaringBitmap) -> RoaringBitmap {
        let out = RoaringBitmap.init();
        let i: usize = 0;
        let j: usize = 0;
        while (i < _.container_count && j < other.container_count && out.container_count < MAX_CONTAINERS) {
            const ca = _.containers[i];
            const cb = other.containers[j];
            if (ca.key < cb.key) {
                out.containers[out.container_count] = ca;
                out.total_len += ca.count as u64;
                out.container_count += 1;
                i += 1;
            } else if (cb.key < ca.key) {
                j += 1;
            } else {
                const diff = diffContainers(ca, cb);
                if (diff.count > 0) {
                    out.containers[out.container_count] = diff;
                    out.total_len += diff.count as u64;
                    out.container_count += 1;
                }
                i += 1;
                j += 1;
            }
        }
        while (i < _.container_count && out.container_count < MAX_CONTAINERS) {
            const ca = _.containers[i];
            out.containers[out.container_count] = ca;
            out.total_len += ca.count as u64;
            out.container_count += 1;
            i += 1;
        }
        return out;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.container_count) {
            _.containers[i] = emptyContainer();
            i += 1;
        }
        _.container_count = 0 as usize;
        _.total_len = 0 as u64;
    }
}

export {
    RoaringContainer,
    RoaringBitmap,
};
