// @cap/priority-queue — Zero-allocation Indexed Binary Min-Heap with O(log N) changePriority & remove for CP
// Clean-room MIT implementation inspired by Rust `priority-queue` and npm `heap-js`.

const MAX_ITEMS: usize = 128;
const NOT_IN_HEAP: i16 = -1;

pub struct PqItem {
    pub id: u16,
    pub priority: i64,
    pub valid: bool,
}

pub struct IndexedMinHeap {
    pub heap_ids: [MAX_ITEMS]u16,
    pub priorities: [MAX_ITEMS]i64,
    pub pos: [MAX_ITEMS]i16,
    pub size: usize,

    pub static fn init() -> IndexedMinHeap {
        let hids: [MAX_ITEMS]u16 = undefined;
        let pris: [MAX_ITEMS]i64 = undefined;
        let ps: [MAX_ITEMS]i16 = undefined;
        let i: usize = 0;
        while (i < MAX_ITEMS) {
            hids[i] = 0 as u16;
            pris[i] = 0 as i64;
            ps[i] = NOT_IN_HEAP;
            i += 1;
        }
        return IndexedMinHeap{
            heap_ids: hids,
            priorities: pris,
            pos: ps,
            size: 0 as usize,
        };
    }

    pub fn len() -> usize {
        return _.size;
    }

    pub fn isEmpty() -> bool {
        return _.size == (0 as usize);
    }

    pub fn contains(id: u16) -> bool {
        const idx = id as usize;
        if (idx >= MAX_ITEMS) {
            return false;
        }
        return _.pos[idx] >= (0 as i16);
    }

    pub fn priorityOf(id: u16) -> i64 {
        const idx = id as usize;
        if (idx >= MAX_ITEMS || _.pos[idx] < (0 as i16)) {
            return 0 as i64;
        }
        return _.priorities[idx];
    }

    pub fn peek() -> PqItem {
        if (_.size == (0 as usize)) {
            return PqItem{
                id: 0 as u16,
                priority: 0 as i64,
                valid: false,
            };
        }
        const topId = _.heap_ids[0];
        return PqItem{
            id: topId,
            priority: _.priorities[topId as usize],
            valid: true,
        };
    }

    fn lessAt(i: usize, j: usize) -> bool {
        const idA = _.heap_ids[i];
        const idB = _.heap_ids[j];
        const pA = _.priorities[idA as usize];
        const pB = _.priorities[idB as usize];
        if (pA != pB) {
            return pA < pB;
        }
        return idA < idB;
    }

    fn swapAt(*_, i: usize, j: usize) {
        const idA = _.heap_ids[i];
        const idB = _.heap_ids[j];
        _.heap_ids[i] = idB;
        _.heap_ids[j] = idA;
        _.pos[idB as usize] = i as i16;
        _.pos[idA as usize] = j as i16;
    }

    fn siftUp(*_, idx_in: usize) {
        let idx = idx_in;
        while (idx > (0 as usize)) {
            const parent = (idx - (1 as usize)) / (2 as usize);
            if (_.lessAt(idx, parent)) {
                _.swapAt(idx, parent);
                idx = parent;
            } else {
                break;
            }
        }
    }

    fn siftDown(*_, idx_in: usize) {
        let idx = idx_in;
        while (true) {
            const left = (idx * (2 as usize)) + (1 as usize);
            const right = left + (1 as usize);
            let smallest = idx;
            if (left < _.size && _.lessAt(left, smallest)) {
                smallest = left;
            }
            if (right < _.size && _.lessAt(right, smallest)) {
                smallest = right;
            }
            if (smallest != idx) {
                _.swapAt(idx, smallest);
                idx = smallest;
            } else {
                break;
            }
        }
    }

    pub fn changePriority(*_, id: u16, new_priority: i64) -> bool {
        const key = id as usize;
        if (key >= MAX_ITEMS || _.pos[key] < (0 as i16)) {
            return false;
        }
        const oldPri = _.priorities[key];
        _.priorities[key] = new_priority;
        const heapIdx = _.pos[key] as usize;
        if (new_priority < oldPri) {
            _.siftUp(heapIdx);
        } else if (new_priority > oldPri) {
            _.siftDown(heapIdx);
        }
        return true;
    }

    pub fn push(*_, id: u16, priority: i64) -> bool {
        const key = id as usize;
        if (key >= MAX_ITEMS) {
            return false;
        }
        if (_.pos[key] >= (0 as i16)) {
            return _.changePriority(id, priority);
        }
        const idx = _.size;
        _.heap_ids[idx] = id;
        _.priorities[key] = priority;
        _.pos[key] = idx as i16;
        _.size += 1;
        _.siftUp(idx);
        return true;
    }

    pub fn remove(*_, id: u16) -> bool {
        const key = id as usize;
        if (key >= MAX_ITEMS || _.pos[key] < (0 as i16)) {
            return false;
        }
        const idx = _.pos[key] as usize;
        const lastIdx = _.size - (1 as usize);
        _.size -= 1;
        _.pos[key] = NOT_IN_HEAP;

        if (idx < lastIdx) {
            const movedId = _.heap_ids[lastIdx];
            _.heap_ids[idx] = movedId;
            _.pos[movedId as usize] = idx as i16;
            _.siftUp(idx);
            _.siftDown(_.pos[movedId as usize] as usize);
        }
        return true;
    }

    pub fn pop(*_) -> PqItem {
        if (_.size == (0 as usize)) {
            return PqItem{
                id: 0 as u16,
                priority: 0 as i64,
                valid: false,
            };
        }
        const topId = _.heap_ids[0];
        const topPri = _.priorities[topId as usize];
        * = _.remove(topId);
        return PqItem{
            id: topId,
            priority: topPri,
            valid: true,
        };
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.size) {
            const id = _.heap_ids[i] as usize;
            _.pos[id] = NOT_IN_HEAP;
            i += 1;
        }
        _.size = 0 as usize;
    }
}

export {
    PqItem,
    IndexedMinHeap,
};
