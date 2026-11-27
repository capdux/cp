// @cap/fast-fifo — Zero-heap O(1) ring-buffer FIFO queue in pure CP
// Clean-room MIT implementation of npm `fast-fifo` (#824, 71.1M/wk).

pub const FIFO_CAP: usize = 256;

pub struct FifoI64 {
    pub buf: [256]i64,
    pub head: usize,
    pub tail: usize,
    pub count: usize,

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn isFull(): bool {
        return _.count == FIFO_CAP;
    }

    pub fn push(*_, val: i64): bool {
        if (_.count >= FIFO_CAP) {
            return false;
        }
        _.buf[_.tail] = val;
        _.tail = (_.tail + 1) & (FIFO_CAP - 1);
        _.count += 1;
        return true;
    }

    pub fn shift(*_): i64? {
        if (_.count == 0) {
            return null;
        }
        const v = _.buf[_.head];
        _.head = (_.head + 1) & (FIFO_CAP - 1);
        _.count -= 1;
        return v;
    }

    pub fn peek(): i64? {
        if (_.count == 0) {
            return null;
        }
        return _.buf[_.head];
    }

    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }
}

pub fn newFifoI64(): FifoI64 {
    return FifoI64 {
        buf: undefined,
        head: 0,
        tail: 0,
        count: 0,
    };
}

pub struct FifoStr {
    pub buf: [256]string,
    pub head: usize,
    pub tail: usize,
    pub count: usize,

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn push(*_, val: string): bool {
        if (_.count >= FIFO_CAP) {
            return false;
        }
        _.buf[_.tail] = val;
        _.tail = (_.tail + 1) & (FIFO_CAP - 1);
        _.count += 1;
        return true;
    }

    pub fn shift(*_): string? {
        if (_.count == 0) {
            return null;
        }
        const v = _.buf[_.head];
        _.head = (_.head + 1) & (FIFO_CAP - 1);
        _.count -= 1;
        return v;
    }

    pub fn peek(): string? {
        if (_.count == 0) {
            return null;
        }
        return _.buf[_.head];
    }

    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
    }
}

pub fn newFifoStr(): FifoStr {
    return FifoStr {
        buf: undefined,
        head: 0,
        tail: 0,
        count: 0,
    };
}

export {
    FIFO_CAP,
    FifoI64,
    newFifoI64,
    FifoStr,
    newFifoStr,
};
