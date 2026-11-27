// @cap/dlv-list — Zero-heap Vec-backed doubly linked list with stable handles.

pub const DLV_NONE: u32 = 0xFFFFFFFF;
pub const DLV_CAP: usize = 128;

fn dlvlZeroI64(): [DLV_CAP]i64 {
    let arr: [DLV_CAP]i64 = undefined;
    let i: usize = 0;
    while (i < DLV_CAP) : (i += 1) {
        arr[i] = 0;
    }
    return arr;
}

fn dlvlNoneU32(): [DLV_CAP]u32 {
    let arr: [DLV_CAP]u32 = undefined;
    let i: usize = 0;
    while (i < DLV_CAP) : (i += 1) {
        arr[i] = DLV_NONE;
    }
    return arr;
}

fn dlvlFalseBool(): [DLV_CAP]bool {
    let arr: [DLV_CAP]bool = undefined;
    let i: usize = 0;
    while (i < DLV_CAP) : (i += 1) {
        arr[i] = false;
    }
    return arr;
}

fn dlvlAllocSlot(list: *DlvListI64): u32 {
    if (list.freeHead != DLV_NONE) {
        const slot = list.freeHead;
        list.freeHead = list.next[slot as usize];
        return slot;
    }
    if ((list.usedSlots as usize) < DLV_CAP) {
        const slot = list.usedSlots;
        list.usedSlots += 1;
        return slot;
    }
    return DLV_NONE;
}

pub struct DlvListI64 {
    values: [DLV_CAP]i64,
    prev: [DLV_CAP]u32,
    next: [DLV_CAP]u32,
    occupied: [DLV_CAP]bool,
    head: u32,
    tail: u32,
    freeHead: u32,
    usedSlots: u32,
    count: usize,

    static fn new(): DlvListI64 {
        return DlvListI64{
            values: dlvlZeroI64(),
            prev: dlvlNoneU32(),
            next: dlvlNoneU32(),
            occupied: dlvlFalseBool(),
            head: DLV_NONE,
            tail: DLV_NONE,
            freeHead: DLV_NONE,
            usedSlots: 0,
            count: 0,
        };
    }

    fn pushFront(*_, val: i64): u32 {
        const slot = dlvlAllocSlot(_);
        if (slot == DLV_NONE) {
            return DLV_NONE;
        }
        const si: usize = slot as usize;
        _.values[si] = val;
        _.occupied[si] = true;
        _.prev[si] = DLV_NONE;
        _.next[si] = _.head;
        if (_.head != DLV_NONE) {
            _.prev[_.head as usize] = slot;
        } else {
            _.tail = slot;
        }
        _.head = slot;
        _.count += 1;
        return slot;
    }

    fn pushBack(*_, val: i64): u32 {
        const slot = dlvlAllocSlot(_);
        if (slot == DLV_NONE) {
            return DLV_NONE;
        }
        const si: usize = slot as usize;
        _.values[si] = val;
        _.occupied[si] = true;
        _.next[si] = DLV_NONE;
        _.prev[si] = _.tail;
        if (_.tail != DLV_NONE) {
            _.next[_.tail as usize] = slot;
        } else {
            _.head = slot;
        }
        _.tail = slot;
        _.count += 1;
        return slot;
    }

    fn get(idx: u32): i64? {
        const si: usize = idx as usize;
        if (si >= DLV_CAP || !_.occupied[si]) {
            return null;
        }
        return _.values[si];
    }

    fn remove(*_, idx: u32): i64? {
        const si: usize = idx as usize;
        if (si >= DLV_CAP || !_.occupied[si]) {
            return null;
        }
        const val = _.values[si];
        const p = _.prev[si];
        const n = _.next[si];
        if (p != DLV_NONE) {
            _.next[p as usize] = n;
        } else {
            _.head = n;
        }
        if (n != DLV_NONE) {
            _.prev[n as usize] = p;
        } else {
            _.tail = p;
        }
        _.occupied[si] = false;
        _.prev[si] = DLV_NONE;
        _.next[si] = _.freeHead;
        _.freeHead = idx;
        _.count -= 1;
        return val;
    }

    fn popFront(*_): i64? {
        if (_.head == DLV_NONE) {
            return null;
        }
        return _.remove(_.head);
    }

    fn popBack(*_): i64? {
        if (_.tail == DLV_NONE) {
            return null;
        }
        return _.remove(_.tail);
    }

    fn front(): i64? {
        if (_.head == DLV_NONE) {
            return null;
        }
        return _.values[_.head as usize];
    }

    fn back(): i64? {
        if (_.tail == DLV_NONE) {
            return null;
        }
        return _.values[_.tail as usize];
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }
}
