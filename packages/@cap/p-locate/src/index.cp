// packages/@cap/p-locate/src/index.cp
// Zero-allocation bounded-concurrency locator planner for CP.
// Clean-room MIT pure CP implementation of npm `p-locate` v7, reusing `@cap/p-limit-core`.

import { PLimit } from "../../p-limit-core/src/index.cp";

pub const PLOC_MAX_CANDIDATES: usize = 32;

/// Configuration options for `PLocatePlanner`.
pub struct PLocateOptions {
    pub concurrency: usize,
    pub preserve_order: bool,

    pub static fn defaults(): PLocateOptions {
        return PLocateOptions {
            concurrency: 4,
            preserve_order: true,
        };
    }
}

/// Candidate record tracking completion and match status.
pub struct LocateCandidate {
    pub id: u32,
    pub matched: bool,
    pub finished: bool,
}

fn plocEmptyCandidate(): LocateCandidate {
    return LocateCandidate {
        id: 0,
        matched: false,
        finished: false,
    };
}

/// Zero-heap 32-slot bounded-concurrency locator scheduler reusing `PLimit`.
pub struct PLocatePlanner {
    pub opts: PLocateOptions,
    pub limiter: PLimit,
    pub items: [32]LocateCandidate,
    pub count: usize,
    pub settled: bool,
    pub has_winner: bool,
    pub winner: u32,

    /// Creates a new `PLocatePlanner`. Returns `null` if `opts.concurrency == 0`.
    pub static fn new(opts: PLocateOptions): PLocatePlanner? {
        let lim_opt: PLimit? = PLimit.new(opts.concurrency);
        if (lim_opt == null) {
            return null;
        }
        let arr: [32]LocateCandidate = [plocEmptyCandidate(); 32];
        return PLocatePlanner {
            opts: opts,
            limiter: lim_opt ?? PLimit.single(),
            items: arr,
            count: 0,
            settled: false,
            has_winner: false,
            winner: 0,
        };
    }

    /// Creates a default planner (`concurrency=4, preserve_order=true`), useful as an unwrap fallback.
    pub static fn defaultPlanner(): PLocatePlanner {
        let arr: [32]LocateCandidate = [plocEmptyCandidate(); 32];
        return PLocatePlanner {
            opts: PLocateOptions.defaults(),
            limiter: PLimit.new(4) ?? PLimit.single(),
            items: arr,
            count: 0,
            settled: false,
            has_winner: false,
            winner: 0,
        };
    }

    /// Appends a candidate ID and enqueues it into `PLimit`.
    pub fn addCandidate(*_, id: u32): bool {
        if (_.settled || _.count >= PLOC_MAX_CANDIDATES) {
            return false;
        }
        if (!_.limiter.submit(id)) {
            return false;
        }
        _.items[_.count] = LocateCandidate {
            id: id,
            matched: false,
            finished: false,
        };
        _.count += 1;
        return true;
    }

    /// Dispatches the next queued candidate subject to `PLimit` concurrency cap.
    pub fn dispatchNext(*_): u32? {
        if (_.settled) {
            return null;
        }
        return _.limiter.startNext();
    }

    /// Reports the predicate result for `id`.
    /// - When `preserve_order == false` and `matched == true`: settles immediately with `id` and cancels pending queue.
    /// - When `preserve_order == true`: resolves with the earliest `matched == true` candidate once all earlier candidates finish.
    pub fn reportResult(*_, id: u32, matched: bool): u32? {
        let found_idx: usize = _.count;
        let i: usize = 0;
        while (i < _.count) {
            if (_.items[i].id == id && !_.items[i].finished) {
                found_idx = i;
                break;
            }
            i += 1;
        }
        if (found_idx >= _.count) {
            return _.winnerId();
        }

        _.items[found_idx].finished = true;
        _.items[found_idx].matched = matched;
        * = _.limiter.finishOne();

        if (_.settled) {
            return _.winnerId();
        }

        if (!_.opts.preserve_order) {
            if (matched) {
                _.settled = true;
                _.has_winner = true;
                _.winner = id;
                * = _.limiter.clearQueue();
                return id;
            }
            let all_done: bool = true;
            let k: usize = 0;
            while (k < _.count) {
                if (!_.items[k].finished) {
                    all_done = false;
                    break;
                }
                k += 1;
            }
            if (all_done) {
                _.settled = true;
            }
            return null;
        }

        // preserve_order == true: scan from index 0
        let j: usize = 0;
        while (j < _.count) {
            if (!_.items[j].finished) {
                return null;
            }
            if (_.items[j].matched) {
                _.settled = true;
                _.has_winner = true;
                _.winner = _.items[j].id;
                * = _.limiter.clearQueue();
                return _.winner;
            }
            j += 1;
        }

        _.settled = true;
        return null;
    }

    /// Returns `true` once a winner has been selected or all candidates finished without a match.
    pub fn isSettled(): bool {
        return _.settled;
    }

    /// Returns the winning candidate ID if settled with a match, otherwise `null`.
    pub fn winnerId(): u32? {
        if (!_.has_winner) {
            return null;
        }
        return _.winner;
    }
}
