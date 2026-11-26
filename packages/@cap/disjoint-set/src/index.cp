// @cap/disjoint-set — Zero-heap Disjoint Set Union (Union-Find)
// Clean-room MIT implementation in pure CP (no heap allocations, no FFI).

export const MAX_ELEMENTS: usize = 256;

export struct DisjointSet {
    parent: [256]u16,
    sizes: [256]u16,
    count: usize,
    components: usize,

    pub static fn init(n: usize): DisjointSet {
        let cap: usize = n;
        if (cap > MAX_ELEMENTS) {
            cap = MAX_ELEMENTS;
        }
        let parent: [256]u16 = undefined;
        let sizes: [256]u16 = undefined;
        let i: usize = 0;
        while (i < cap) {
            parent[i] = i as u16;
            sizes[i] = 1;
            i += 1;
        }
        return DisjointSet {
            parent: parent,
            sizes: sizes,
            count: cap,
            components: cap,
        };
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn componentCount(): usize {
        return _.components;
    }

    pub fn find(*_, x: usize): i64 {
        if (x >= _.count) {
            return -1;
        }
        let curr: usize = x;
        while ((_.parent[curr] as usize) != curr) {
            let p: usize = _.parent[curr] as usize;
            let gp: u16 = _.parent[p];
            _.parent[curr] = gp;
            curr = gp as usize;
        }
        return curr as i64;
    }

    pub fn findReadOnly(x: usize): i64 {
        if (x >= _.count) {
            return -1;
        }
        let curr: usize = x;
        while ((_.parent[curr] as usize) != curr) {
            curr = _.parent[curr] as usize;
        }
        return curr as i64;
    }

    pub fn connected(*_, a: usize, b: usize): bool {
        let ra: i64 = _.find(a);
        if (ra < 0) {
            return false;
        }
        let rb: i64 = _.find(b);
        if (rb < 0) {
            return false;
        }
        return ra == rb;
    }

    pub fn unite(*_, a: usize, b: usize): bool {
        let ra_i: i64 = _.find(a);
        if (ra_i < 0) {
            return false;
        }
        let rb_i: i64 = _.find(b);
        if (rb_i < 0) {
            return false;
        }
        if (ra_i == rb_i) {
            return false;
        }
        let ra: usize = ra_i as usize;
        let rb: usize = rb_i as usize;
        if (_.sizes[ra] < _.sizes[rb]) {
            let tmp: usize = ra;
            ra = rb;
            rb = tmp;
        }
        _.parent[rb] = ra as u16;
        _.sizes[ra] = _.sizes[ra] + _.sizes[rb];
        _.components = _.components - 1;
        return true;
    }

    pub fn setSize(*_, x: usize): usize {
        let r: i64 = _.find(x);
        if (r < 0) {
            return 0;
        }
        return _.sizes[r as usize] as usize;
    }

    pub fn reset(*_, n: usize) {
        let cap: usize = n;
        if (cap > MAX_ELEMENTS) {
            cap = MAX_ELEMENTS;
        }
        _.count = cap;
        _.components = cap;
        let i: usize = 0;
        while (i < cap) {
            _.parent[i] = i as u16;
            _.sizes[i] = 1;
            i += 1;
        }
    }
}
