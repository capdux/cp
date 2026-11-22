// Origin: Pure-CP Stdlib (Zero-Alloc Augmented Interval Tree, Overlap/Stabbing Queries & Range Coalescer) | Created: 2026-11-22
// Pure CP zero-heap-allocation Augmented Interval Tree & Range Coalescer:
//   - Fixed-capacity inline node pool (`AUG_ITREE_MAX_NODES = 256`) supporting closed `i64` intervals `[start, end]`
//   - Pure iterative (zero recursion) balanced tree construction & incremental BST insertion maintaining subtree `max_end`
//   - Subtree-pruned iterative queries: stabbing (`queryPoint`), overlap (`queryOverlap`), containment (`queryContained`),
//     and O(log N) CLRS single-path overlap existence check (`hasAnyOverlap`)
//   - In-place overlapping/adjacent range merging (`coalesceIntervals`) and complement gap extraction (`findGaps`)
//   - Strict boundary guards for reversed intervals (`start > end`), empty trees, full capacity, and output buffer truncation

/// Maximum intervals stored in `AugmentedIntervalTree` (zero dynamic heap allocation).
pub const AUG_ITREE_MAX_NODES: usize = 256;

/// Sentinel invalid node index.
pub const AUG_ITREE_NO_NODE: u16 = 0xFFFF;

const I64_MAX: i64 = 9223372036854775807;

// ============================================================================
// 1. Public Data Models
// ============================================================================

/// Closed integer interval `[start, end]` carrying an opaque `payload_id`.
pub struct IntervalSpan {
    start: i64 = 0;
    end: i64 = 0;
    payload_id: u64 = 0;
}

/// Result summary for interval queries and gap extraction, tracking truncation when `out_hits` is smaller than matches.
pub struct IntervalQueryResult {
    written: usize = 0;
    total_matched: usize = 0;
    truncated: bool = false;
}

/// Aggregate operational statistics for `AugmentedIntervalTree`.
pub struct IntervalTreeStats {
    count: usize = 0;
    capacity: usize = AUG_ITREE_MAX_NODES;
    balanced: bool = true;
    min_start: i64 = 0;
    max_end: i64 = 0;
}

struct AugIntervalNode {
    span: IntervalSpan = IntervalSpan{},
    max_end: i64 = 0;
    left: u16 = AUG_ITREE_NO_NODE;
    right: u16 = AUG_ITREE_NO_NODE;
}

struct BuildFrame {
    lo: usize = 0;
    hi: usize = 0;
    parent_idx: u16 = AUG_ITREE_NO_NODE;
    is_left: bool = false;
}

fn spanLess(a: IntervalSpan, b: IntervalSpan): bool {
    if (a.start != b.start) return a.start < b.start;
    if (a.end != b.end) return a.end < b.end;
    return a.payload_id < b.payload_id;
}

fn sortSpansAsc(items: *[]IntervalSpan, count: usize) {
    let i: usize = 1;
    while (i < count) : (i += 1) {
        const key = items[i];
        let j: usize = i;
        while (j > 0 && spanLess(key, items[j - 1])) : (j -= 1) {
            items[j] = items[j - 1];
        }
        items[j] = key;
    }
}

fn canMergeSpans(cur_end: i64, next_start: i64, merge_adjacent: bool): bool {
    if (next_start <= cur_end) return true;
    if (merge_adjacent && cur_end < I64_MAX && next_start <= cur_end + 1) return true;
    return false;
}

// ============================================================================
// 2. Pure Functions: Range Coalescing & Complement Gap Extraction
// ============================================================================

/// Sorts `in_out_ranges[0..count]` in place and merges overlapping intervals (plus integer-adjacent
/// intervals `[a, b]` + `[b+1, c]` when `merge_adjacent == true`).
/// Returns the merged interval count, or `null` if `count > in_out_ranges.len` or any interval has `start > end`.
pub fn coalesceIntervals(
    in_out_ranges: *[]IntervalSpan,
    count: usize,
    merge_adjacent: bool,
): usize? {
    if (count > in_out_ranges.len) return null;
    if (count == 0) return 0;

    let chk: usize = 0;
    while (chk < count) : (chk += 1) {
        if (in_out_ranges[chk].start > in_out_ranges[chk].end) return null;
    }

    sortSpansAsc(in_out_ranges, count);

    let out_cnt: usize = 1;
    let i: usize = 1;
    while (i < count) : (i += 1) {
        const next = in_out_ranges[i];
        if (canMergeSpans(in_out_ranges[out_cnt - 1].end, next.start, merge_adjacent)) {
            if (next.end > in_out_ranges[out_cnt - 1].end) {
                in_out_ranges[out_cnt - 1].end = next.end;
            }
        } else {
            in_out_ranges[out_cnt] = next;
            out_cnt += 1;
        }
    }
    return out_cnt;
}

/// Extracts uncovered complement gaps within closed window `[window_start, window_end]` relative to `ranges`.
/// Rejects `window_start > window_end` (returns empty result). Ignores intervals outside the window;
/// tracks `total_matched` and `truncated` if `out_gaps` is smaller than the number of gaps.
pub fn findGaps(
    ranges: []const IntervalSpan,
    window_start: i64,
    window_end: i64,
    out_gaps: *[]IntervalSpan,
): IntervalQueryResult {
    let res = IntervalQueryResult{};
    if (window_start > window_end) return res;

    // Collect clipped valid intervals intersecting [window_start, window_end]
    let clipped: [AUG_ITREE_MAX_NODES]IntervalSpan = [IntervalSpan{}; AUG_ITREE_MAX_NODES];
    let clip_cnt: usize = 0;

    let i: usize = 0;
    while (i < ranges.len && clip_cnt < AUG_ITREE_MAX_NODES) : (i += 1) {
        const r = ranges[i];
        if (r.start > r.end) continue;
        if (r.end < window_start || r.start > window_end) continue;
        clipped[clip_cnt] = IntervalSpan{
            start: if (r.start < window_start) window_start else r.start,
            end: if (r.end > window_end) window_end else r.end,
            payload_id: r.payload_id,
        };
        clip_cnt += 1;
    }

    if (clip_cnt == 0) {
        res.total_matched = 1;
        if (out_gaps.len > 0) {
            out_gaps[0] = IntervalSpan{ start: window_start, end: window_end, payload_id: 0 };
            res.written = 1;
        } else {
            res.truncated = true;
        }
        return res;
    }

    let clip_s: []IntervalSpan = clipped[0..clip_cnt];
    const merged_cnt: usize = coalesceIntervals(*clip_s, clip_cnt, true) ?? 0;

    let cursor: i64 = window_start;
    let m: usize = 0;
    while (m < merged_cnt) : (m += 1) {
        const seg = clipped[m];
        if (seg.start > cursor) {
            res.total_matched += 1;
            if (res.written < out_gaps.len) {
                out_gaps[res.written] = IntervalSpan{
                    start: cursor,
                    end: seg.start - 1,
                    payload_id: 0,
                };
                res.written += 1;
            } else {
                res.truncated = true;
            }
        }
        if (seg.end == I64_MAX) {
            return res;
        }
        if (seg.end + 1 > cursor) {
            cursor = seg.end + 1;
        }
    }

    if (cursor <= window_end) {
        res.total_matched += 1;
        if (res.written < out_gaps.len) {
            out_gaps[res.written] = IntervalSpan{
                start: cursor,
                end: window_end,
                payload_id: 0,
            };
            res.written += 1;
        } else {
            res.truncated = true;
        }
    }
    return res;
}

// ============================================================================
// 3. Zero-Alloc Augmented Interval Tree Engine (Pure Iterative)
// ============================================================================

/// Zero-heap-allocation Augmented Interval Tree supporting balanced static construction,
/// incremental insertion, subtree-pruned overlap/stabbing/containment queries, and gap finding.
pub struct AugmentedIntervalTree {
    nodes: [AUG_ITREE_MAX_NODES]AugIntervalNode = [AugIntervalNode{}; AUG_ITREE_MAX_NODES];
    count: usize = 0;
    root: u16 = AUG_ITREE_NO_NODE;
    balanced: bool = true;
    min_start: i64 = 0;
    max_end: i64 = 0;

    /// Initializes an empty `AugmentedIntervalTree`.
    static fn init(): AugmentedIntervalTree {
        return AugmentedIntervalTree{
            nodes: [AugIntervalNode{}; AUG_ITREE_MAX_NODES],
            count: 0,
            root: AUG_ITREE_NO_NODE,
            balanced: true,
            min_start: 0,
            max_end: 0,
        };
    }

    /// Builds a strictly balanced `AugmentedIntervalTree` from `items` in one pass (zero recursion).
    /// Returns `null` if `items.len > AUG_ITREE_MAX_NODES` or any interval has `start > end`.
    static fn buildFromSlice(items: []const IntervalSpan): AugmentedIntervalTree? {
        if (items.len > AUG_ITREE_MAX_NODES) return null;
        let tree = AugmentedIntervalTree.init();
        let i: usize = 0;
        while (i < items.len) : (i += 1) {
            if (items[i].start > items[i].end) return null;
            tree.nodes[i] = AugIntervalNode{
                span: items[i],
                max_end: items[i].end,
                left: AUG_ITREE_NO_NODE,
                right: AUG_ITREE_NO_NODE,
            };
        }
        tree.count = items.len;
        if (tree.count > 0) {
            tree.rebuildBalanced();
        }
        return tree;
    }

    /// Incrementally inserts closed interval `[start, end]` with `payload_id` into the augmented BST
    /// (updating subtree `max_end` along the descent path iteratively). Returns `false` if `start > end` or full.
    fn insert(*_, start: i64, end: i64, payload_id: u64): bool {
        if (start > end || _.count >= AUG_ITREE_MAX_NODES) return false;

        const span = IntervalSpan{ start: start, end: end, payload_id: payload_id };
        const new_slot: u16 = _.count as u16;
        _.nodes[_.count] = AugIntervalNode{
            span: span,
            max_end: end,
            left: AUG_ITREE_NO_NODE,
            right: AUG_ITREE_NO_NODE,
        };
        _.count += 1;

        if (_.root == AUG_ITREE_NO_NODE) {
            _.root = new_slot;
            _.min_start = start;
            _.max_end = end;
            _.balanced = true;
            return true;
        }

        if (start < _.min_start) _.min_start = start;
        if (end > _.max_end) _.max_end = end;
        _.balanced = false;

        let cur: u16 = _.root;
        let guard: usize = 0;
        while (guard < AUG_ITREE_MAX_NODES) : (guard += 1) {
            const c_idx: usize = cur as usize;
            if (end > _.nodes[c_idx].max_end) {
                _.nodes[c_idx].max_end = end;
            }
            if (spanLess(span, _.nodes[c_idx].span)) {
                if (_.nodes[c_idx].left == AUG_ITREE_NO_NODE) {
                    _.nodes[c_idx].left = new_slot;
                    return true;
                }
                cur = _.nodes[c_idx].left;
            } else {
                if (_.nodes[c_idx].right == AUG_ITREE_NO_NODE) {
                    _.nodes[c_idx].right = new_slot;
                    return true;
                }
                cur = _.nodes[c_idx].right;
            }
        }
        return true;
    }

    /// Rebuilds the entire tree into a strictly balanced augmented BST (height <= log2(N)+1)
    /// and recomputes exact subtree `max_end` values purely iteratively (zero recursion).
    fn rebuildBalanced(*_) {
        if (_.count == 0) {
            _.root = AUG_ITREE_NO_NODE;
            _.balanced = true;
            _.min_start = 0;
            _.max_end = 0;
            return;
        }

        // Extract and sort spans ascending
        let sorted: [AUG_ITREE_MAX_NODES]IntervalSpan = [IntervalSpan{}; AUG_ITREE_MAX_NODES];
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            sorted[i] = _.nodes[i].span;
        }
        let sorted_s: []IntervalSpan = sorted[0.._.count];
        sortSpansAsc(*sorted_s, _.count);

        _.min_start = sorted[0].start;
        _.max_end = sorted[0].end;
        let k: usize = 0;
        while (k < _.count) : (k += 1) {
            if (sorted[k].end > _.max_end) _.max_end = sorted[k].end;
            _.nodes[k] = AugIntervalNode{
                span: sorted[k],
                max_end: sorted[k].end,
                left: AUG_ITREE_NO_NODE,
                right: AUG_ITREE_NO_NODE,
            };
        }

        // Pure iterative median-split BST construction + record creation order for bottom-up max_end propagation
        let stack: [AUG_ITREE_MAX_NODES]BuildFrame = [BuildFrame{}; AUG_ITREE_MAX_NODES];
        let order: [AUG_ITREE_MAX_NODES]u16 = [AUG_ITREE_NO_NODE; AUG_ITREE_MAX_NODES];
        let top: usize = 0;
        let ord_cnt: usize = 0;

        stack[top] = BuildFrame{ lo: 0, hi: _.count, parent_idx: AUG_ITREE_NO_NODE, is_left: false };
        top += 1;
        _.root = AUG_ITREE_NO_NODE;

        while (top > 0) {
            top -= 1;
            const f = stack[top];
            if (f.lo >= f.hi) continue;
            const mid: usize = f.lo + ((f.hi - f.lo) >> 1);
            const mid_u16: u16 = mid as u16;

            if (f.parent_idx == AUG_ITREE_NO_NODE) {
                _.root = mid_u16;
            } else if (f.is_left) {
                _.nodes[f.parent_idx as usize].left = mid_u16;
            } else {
                _.nodes[f.parent_idx as usize].right = mid_u16;
            }

            order[ord_cnt] = mid_u16;
            ord_cnt += 1;

            if (mid + 1 < f.hi) {
                stack[top] = BuildFrame{ lo: mid + 1, hi: f.hi, parent_idx: mid_u16, is_left: false };
                top += 1;
            }
            if (f.lo < mid) {
                stack[top] = BuildFrame{ lo: f.lo, hi: mid, parent_idx: mid_u16, is_left: true };
                top += 1;
            }
        }

        // Bottom-up exact subtree max_end propagation (reverse topological order)
        let r: usize = ord_cnt;
        while (r > 0) : (r -= 1) {
            const idx: usize = order[r - 1] as usize;
            let best: i64 = _.nodes[idx].span.end;
            const l_child: u16 = _.nodes[idx].left;
            const r_child: u16 = _.nodes[idx].right;
            if (l_child != AUG_ITREE_NO_NODE && _.nodes[l_child as usize].max_end > best) {
                best = _.nodes[l_child as usize].max_end;
            }
            if (r_child != AUG_ITREE_NO_NODE && _.nodes[r_child as usize].max_end > best) {
                best = _.nodes[r_child as usize].max_end;
            }
            _.nodes[idx].max_end = best;
        }
        _.balanced = true;
    }

    /// O(log N) CLRS augmented-tree single-path overlap existence check for closed interval `[low, high]`.
    fn hasAnyOverlap(low: i64, high: i64): bool {
        if (low > high || _.root == AUG_ITREE_NO_NODE) return false;
        let cur: u16 = _.root;
        let guard: usize = 0;
        while (cur != AUG_ITREE_NO_NODE && guard < AUG_ITREE_MAX_NODES) : (guard += 1) {
            const node = _.nodes[cur as usize];
            if (node.span.start <= high && low <= node.span.end) {
                return true;
            }
            if (node.left != AUG_ITREE_NO_NODE && _.nodes[node.left as usize].max_end >= low) {
                cur = node.left;
            } else {
                cur = node.right;
            }
        }
        return false;
    }

    /// Pure iterative subtree-pruned query collecting all intervals overlapping `[low, high]`.
    fn queryOverlap(low: i64, high: i64, out_hits: *[]IntervalSpan): IntervalQueryResult {
        let res = IntervalQueryResult{};
        if (low > high || _.root == AUG_ITREE_NO_NODE) return res;

        let stack: [AUG_ITREE_MAX_NODES]u16 = [AUG_ITREE_NO_NODE; AUG_ITREE_MAX_NODES];
        let top: usize = 0;
        stack[top] = _.root;
        top += 1;

        while (top > 0) {
            top -= 1;
            const cur: u16 = stack[top];
            const node = _.nodes[cur as usize];

            // Prune right subtree if node.span.start > high (all right children have start >= node.span.start > high)
            if (node.right != AUG_ITREE_NO_NODE && node.span.start <= high && _.nodes[node.right as usize].max_end >= low) {
                if (top < AUG_ITREE_MAX_NODES) {
                    stack[top] = node.right;
                    top += 1;
                }
            }

            if (node.span.start <= high && low <= node.span.end) {
                res.total_matched += 1;
                if (res.written < out_hits.len) {
                    out_hits[res.written] = node.span;
                    res.written += 1;
                } else {
                    res.truncated = true;
                }
            }

            // Prune left subtree if left.max_end < low
            if (node.left != AUG_ITREE_NO_NODE && _.nodes[node.left as usize].max_end >= low) {
                if (top < AUG_ITREE_MAX_NODES) {
                    stack[top] = node.left;
                    top += 1;
                }
            }
        }
        return res;
    }

    /// Stabbing query collecting all intervals containing `point` (`start <= point <= end`).
    fn queryPoint(point: i64, out_hits: *[]IntervalSpan): IntervalQueryResult {
        return _.queryOverlap(point, point, out_hits);
    }

    /// Containment query collecting all intervals strictly contained inside `[low, high]` (`low <= start && end <= high`).
    fn queryContained(low: i64, high: i64, out_hits: *[]IntervalSpan): IntervalQueryResult {
        let res = IntervalQueryResult{};
        if (low > high || _.root == AUG_ITREE_NO_NODE) return res;

        let stack: [AUG_ITREE_MAX_NODES]u16 = [AUG_ITREE_NO_NODE; AUG_ITREE_MAX_NODES];
        let top: usize = 0;
        stack[top] = _.root;
        top += 1;

        while (top > 0) {
            top -= 1;
            const cur: u16 = stack[top];
            const node = _.nodes[cur as usize];

            if (node.right != AUG_ITREE_NO_NODE && node.span.start <= high && _.nodes[node.right as usize].max_end >= low) {
                if (top < AUG_ITREE_MAX_NODES) {
                    stack[top] = node.right;
                    top += 1;
                }
            }

            if (low <= node.span.start && node.span.end <= high) {
                res.total_matched += 1;
                if (res.written < out_hits.len) {
                    out_hits[res.written] = node.span;
                    res.written += 1;
                } else {
                    res.truncated = true;
                }
            }

            if (node.left != AUG_ITREE_NO_NODE && _.nodes[node.left as usize].max_end >= low) {
                if (top < AUG_ITREE_MAX_NODES) {
                    stack[top] = node.left;
                    top += 1;
                }
            }
        }
        return res;
    }

    /// Extracts uncovered complement gaps within `[window_start, window_end]` against all intervals in this tree.
    fn findGapsInWindow(window_start: i64, window_end: i64, out_gaps: *[]IntervalSpan): IntervalQueryResult {
        let all: [AUG_ITREE_MAX_NODES]IntervalSpan = [IntervalSpan{}; AUG_ITREE_MAX_NODES];
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            all[i] = _.nodes[i].span;
        }
        return findGaps(all[0.._.count], window_start, window_end, out_gaps);
    }

    /// Returns aggregate tree capacity, balance, and span bounds statistics.
    fn stats(): IntervalTreeStats {
        return IntervalTreeStats{
            count: _.count,
            capacity: AUG_ITREE_MAX_NODES,
            balanced: _.balanced,
            min_start: _.min_start,
            max_end: _.max_end,
        };
    }
}
