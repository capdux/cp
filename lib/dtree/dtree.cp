// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc CART Decision Tree Classifier & Regressor)

/// Sentinel node index indicating the absence of a child node.
pub const DTREE_NO_NODE: u32 = 0xFFFFFFFF;

const INV_LN2: f64 = 1.44269504088896340736;
const SPLIT_EPS: f64 = 1.0e-12;
const INSERTION_SORT_THRESHOLD: usize = 16;

/// Impurity / loss criterion used for axis-aligned CART splitting.
pub enum SplitCriterion {
    Gini,
    Entropy,
    Mse,
}

/// Fixed-pool node representation for both classification and regression trees.
pub struct TreeNode {
    is_leaf: bool,
    feature_idx: usize,
    threshold: f64,
    left: u32,
    right: u32,
    predicted_class: i32,
    predicted_value: f64,
    sample_count: usize,
    impurity: f64,

    /// Returns a zero-initialized leaf node suitable for initializing caller node pools.
    static fn empty(): TreeNode {
        return TreeNode {
            is_leaf: true,
            feature_idx: 0,
            threshold: 0.0,
            left: DTREE_NO_NODE,
            right: DTREE_NO_NODE,
            predicted_class: 0,
            predicted_value: 0.0,
            sample_count: 0,
            impurity: 0.0,
        };
    }
}

/// Hyperparameters controlling CART tree growth and pruning.
pub struct TreeConfig {
    max_depth: usize,
    min_samples_split: usize,
    min_samples_leaf: usize,
    criterion: SplitCriterion,

    /// Default classifier configuration (`max_depth = 16`, `min_samples_split = 2`, `min_samples_leaf = 1`, `Gini`).
    static fn defaultClassifier(): TreeConfig {
        return TreeConfig {
            max_depth: 16,
            min_samples_split: 2,
            min_samples_leaf: 1,
            criterion: SplitCriterion.Gini,
        };
    }

    /// Default regressor configuration (`max_depth = 16`, `min_samples_split = 2`, `min_samples_leaf = 1`, `Mse`).
    static fn defaultRegressor(): TreeConfig {
        return TreeConfig {
            max_depth: 16,
            min_samples_split: 2,
            min_samples_leaf: 1,
            criterion: SplitCriterion.Mse,
        };
    }
}

/// Caller-provided zero-allocation workspace for iterative CART training.
pub struct TreeWorkspace {
    sample_indices: []usize,
    sort_tmp: []usize,
    class_counts: []usize,
    left_counts: []usize,
    stack_node: []u32,
    stack_lo: []usize,
    stack_hi: []usize,
    stack_depth: []usize,
}

struct BestSplit {
    found: bool,
    feature_idx: usize,
    threshold: f64,
    gain: f64,
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn checkMatrixBounds(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn minStackCap(ws: *TreeWorkspace): usize {
    let cap: usize = ws.*.stack_node.len;
    if (ws.*.stack_lo.len < cap) {
        cap = ws.*.stack_lo.len;
    }
    if (ws.*.stack_hi.len < cap) {
        cap = ws.*.stack_hi.len;
    }
    if (ws.*.stack_depth.len < cap) {
        cap = ws.*.stack_depth.len;
    }
    return cap;
}

fn insertionSortRange(
    indices: []usize,
    lo: usize,
    hi: usize,
    x: []const f64,
    n_features: usize,
    feat: usize,
) {
    let i: usize = lo + 1;
    while (i < hi) {
        const key_idx: usize = indices[i];
        const key_val: f64 = x[key_idx * n_features + feat];
        let j: usize = i;
        while (j > lo) {
            const prev_idx: usize = indices[j - 1];
            const prev_val: f64 = x[prev_idx * n_features + feat];
            if (prev_val <= key_val) {
                break;
            }
            indices[j] = prev_idx;
            j -= 1;
        }
        indices[j] = key_idx;
        i += 1;
    }
}

/// Iterative bottom-up stable merge sort over `indices[lo..hi]` by feature `feat`.
fn sortIndicesByFeature(
    indices: []usize,
    tmp: []usize,
    lo: usize,
    hi: usize,
    x: []const f64,
    n_features: usize,
    feat: usize,
) {
    const m: usize = hi - lo;
    if (m <= 1) {
        return;
    }
    if (m <= INSERTION_SORT_THRESHOLD) {
        insertionSortRange(indices, lo, hi, x, n_features, feat);
        return;
    }

    // Sort small runs with insertion sort first.
    let start: usize = lo;
    while (start < hi) {
        let run_end: usize = start + INSERTION_SORT_THRESHOLD;
        if (run_end > hi) {
            run_end = hi;
        }
        insertionSortRange(indices, start, run_end, x, n_features, feat);
        start = run_end;
    }

    // Bottom-up merge passes using caller `tmp[0..m]`.
    let width: usize = INSERTION_SORT_THRESHOLD;
    while (width < m) {
        let left_off: usize = 0;
        while (left_off < m) {
            let mid_off: usize = left_off + width;
            if (mid_off > m) {
                mid_off = m;
            }
            let right_off: usize = mid_off + width;
            if (right_off > m) {
                right_off = m;
            }

            let i: usize = left_off;
            let j: usize = mid_off;
            let k: usize = left_off;
            while (i < mid_off && j < right_off) {
                const li: usize = indices[lo + i];
                const rj: usize = indices[lo + j];
                if (x[li * n_features + feat] <= x[rj * n_features + feat]) {
                    tmp[k] = li;
                    i += 1;
                } else {
                    tmp[k] = rj;
                    j += 1;
                }
                k += 1;
            }
            while (i < mid_off) {
                tmp[k] = indices[lo + i];
                i += 1;
                k += 1;
            }
            while (j < right_off) {
                tmp[k] = indices[lo + j];
                j += 1;
                k += 1;
            }

            left_off = right_off;
        }

        let c: usize = 0;
        while (c < m) {
            indices[lo + c] = tmp[c];
            c += 1;
        }
        width = width * 2;
    }
}

/// Stable partition of `indices[lo..hi]` around `x[idx * n_features + feat] <= threshold`.
/// Returns the number of elements placed in the left partition.
fn partitionIndices(
    indices: []usize,
    tmp: []usize,
    lo: usize,
    hi: usize,
    x: []const f64,
    n_features: usize,
    feat: usize,
    threshold: f64,
): usize {
    const m: usize = hi - lo;
    let left_n: usize = 0;
    let i: usize = 0;
    while (i < m) {
        const idx: usize = indices[lo + i];
        if (x[idx * n_features + feat] <= threshold) {
            tmp[left_n] = idx;
            left_n += 1;
        }
        i += 1;
    }
    let write_pos: usize = left_n;
    let j: usize = 0;
    while (j < m) {
        const idx: usize = indices[lo + j];
        if (x[idx * n_features + feat] > threshold) {
            tmp[write_pos] = idx;
            write_pos += 1;
        }
        j += 1;
    }
    let k: usize = 0;
    while (k < m) {
        indices[lo + k] = tmp[k];
        k += 1;
    }
    return left_n;
}

fn computeGini(counts: []const usize, num_classes: usize, n: usize): f64 {
    if (n == 0) {
        return 0.0;
    }
    const inv_n: f64 = 1.0 / (n as f64);
    let sum_sq: f64 = 0.0;
    let c: usize = 0;
    while (c < num_classes) {
        const p: f64 = (counts[c] as f64) * inv_n;
        sum_sq += p * p;
        c += 1;
    }
    const g: f64 = 1.0 - sum_sq;
    return if (g < 0.0) 0.0 else g;
}

fn computeEntropy(counts: []const usize, num_classes: usize, n: usize): f64 {
    if (n == 0) {
        return 0.0;
    }
    const inv_n: f64 = 1.0 / (n as f64);
    let ent: f64 = 0.0;
    let c: usize = 0;
    while (c < num_classes) {
        const cnt: usize = counts[c];
        if (cnt > 0) {
            const p: f64 = (cnt as f64) * inv_n;
            ent -= p * (@log(p) * INV_LN2);
        }
        c += 1;
    }
    return if (ent < 0.0) 0.0 else ent;
}

fn computeSplitEntropy(
    total_counts: []const usize,
    left_counts: []const usize,
    num_classes: usize,
    n_left: usize,
    n_right: usize,
    n_total: usize,
): f64 {
    const inv_l: f64 = 1.0 / (n_left as f64);
    const inv_r: f64 = 1.0 / (n_right as f64);
    let ent_l: f64 = 0.0;
    let ent_r: f64 = 0.0;
    let c: usize = 0;
    while (c < num_classes) {
        const lc: usize = left_counts[c];
        const rc: usize = total_counts[c] - lc;
        if (lc > 0) {
            const pl: f64 = (lc as f64) * inv_l;
            ent_l -= pl * (@log(pl) * INV_LN2);
        }
        if (rc > 0) {
            const pr: f64 = (rc as f64) * inv_r;
            ent_r -= pr * (@log(pr) * INV_LN2);
        }
        c += 1;
    }
    const n_tot_f: f64 = n_total as f64;
    const weighted: f64 = ((n_left as f64) * ent_l + (n_right as f64) * ent_r) / n_tot_f;
    return if (weighted < 0.0) 0.0 else weighted;
}

fn findBestClassifierSplit(
    x: []const f64,
    y: []const i32,
    n_features: usize,
    num_classes: usize,
    lo: usize,
    hi: usize,
    parent_impurity: f64,
    cfg: TreeConfig,
    ws: *TreeWorkspace,
): BestSplit {
    let best = BestSplit {
        found: false,
        feature_idx: 0,
        threshold: 0.0,
        gain: SPLIT_EPS,
    };

    const m: usize = hi - lo;
    const m_f: f64 = m as f64;
    const use_gini: bool = match (cfg.criterion) {
        .Gini => true,
        .Entropy => false,
        .Mse => false,
    };

    let init_right_sq: f64 = 0.0;
    if (use_gini) {
        let c: usize = 0;
        while (c < num_classes) {
            const cnt_f: f64 = ws.*.class_counts[c] as f64;
            init_right_sq += cnt_f * cnt_f;
            c += 1;
        }
    }

    let feat: usize = 0;
    while (feat < n_features) {
        sortIndicesByFeature(
            ws.*.sample_indices,
            ws.*.sort_tmp,
            lo,
            hi,
            x,
            n_features,
            feat,
        );

        let c: usize = 0;
        while (c < num_classes) {
            ws.*.left_counts[c] = 0;
            c += 1;
        }

        let left_sq: f64 = 0.0;
        let right_sq: f64 = init_right_sq;

        let pos: usize = 0;
        while (pos + 1 < m) {
            const idx_cur: usize = ws.*.sample_indices[lo + pos];
            const cls: usize = y[idx_cur] as usize;
            const lc: usize = ws.*.left_counts[cls];
            const rc: usize = ws.*.class_counts[cls] - lc;

            ws.*.left_counts[cls] = lc + 1;
            if (use_gini) {
                left_sq += (2.0 * (lc as f64)) + 1.0;
                right_sq -= (2.0 * (rc as f64)) - 1.0;
            }

            const n_left: usize = pos + 1;
            const n_right: usize = m - n_left;

            if (n_left >= cfg.min_samples_leaf && n_right >= cfg.min_samples_leaf) {
                const idx_next: usize = ws.*.sample_indices[lo + pos + 1];
                const v_cur: f64 = x[idx_cur * n_features + feat];
                const v_next: f64 = x[idx_next * n_features + feat];
                if ((v_next - v_cur) > SPLIT_EPS) {
                    let weighted_imp: f64 = 0.0;
                    if (use_gini) {
                        const nl_f: f64 = n_left as f64;
                        const nr_f: f64 = n_right as f64;
                        const gini_l: f64 = 1.0 - (left_sq / (nl_f * nl_f));
                        const gini_r: f64 = 1.0 - (right_sq / (nr_f * nr_f));
                        weighted_imp = (nl_f * gini_l + nr_f * gini_r) / m_f;
                    } else {
                        weighted_imp = computeSplitEntropy(
                            ws.*.class_counts,
                            ws.*.left_counts,
                            num_classes,
                            n_left,
                            n_right,
                            m,
                        );
                    }
                    const gain: f64 = parent_impurity - weighted_imp;
                    if (gain > best.gain) {
                        best.found = true;
                        best.feature_idx = feat;
                        best.threshold = 0.5 * (v_cur + v_next);
                        best.gain = gain;
                    }
                }
            }

            pos += 1;
        }

        feat += 1;
    }

    return best;
}

fn findBestRegressorSplit(
    x: []const f64,
    y: []const f64,
    n_features: usize,
    lo: usize,
    hi: usize,
    mean_y: f64,
    total_centered_sum: f64,
    cfg: TreeConfig,
    ws: *TreeWorkspace,
): BestSplit {
    let best = BestSplit {
        found: false,
        feature_idx: 0,
        threshold: 0.0,
        gain: SPLIT_EPS,
    };

    const m: usize = hi - lo;
    const m_f: f64 = m as f64;
    const base_term: f64 = (total_centered_sum * total_centered_sum) / m_f;

    let feat: usize = 0;
    while (feat < n_features) {
        sortIndicesByFeature(
            ws.*.sample_indices,
            ws.*.sort_tmp,
            lo,
            hi,
            x,
            n_features,
            feat,
        );

        let left_sum: f64 = 0.0;
        let pos: usize = 0;
        while (pos + 1 < m) {
            const idx_cur: usize = ws.*.sample_indices[lo + pos];
            left_sum += y[idx_cur] - mean_y;

            const n_left: usize = pos + 1;
            const n_right: usize = m - n_left;

            if (n_left >= cfg.min_samples_leaf && n_right >= cfg.min_samples_leaf) {
                const idx_next: usize = ws.*.sample_indices[lo + pos + 1];
                const v_cur: f64 = x[idx_cur * n_features + feat];
                const v_next: f64 = x[idx_next * n_features + feat];
                if ((v_next - v_cur) > SPLIT_EPS) {
                    const right_sum: f64 = total_centered_sum - left_sum;
                    const nl_f: f64 = n_left as f64;
                    const nr_f: f64 = n_right as f64;
                    const sse_reduction: f64 =
                        ((left_sum * left_sum) / nl_f) + ((right_sum * right_sum) / nr_f) - base_term;
                    const gain: f64 = sse_reduction / m_f;
                    if (gain > best.gain) {
                        best.found = true;
                        best.feature_idx = feat;
                        best.threshold = 0.5 * (v_cur + v_next);
                        best.gain = gain;
                    }
                }
            }

            pos += 1;
        }

        feat += 1;
    }

    return best;
}

/// Zero-allocation CART decision tree (classification and regression) backed by a caller-owned node pool.
pub struct DecisionTree {
    nodes: []TreeNode,
    node_count: usize,
    num_features: usize,
    num_classes: usize,
    is_regressor: bool,
    max_depth_reached: usize,

    /// Fits an axis-aligned CART classifier using Gini impurity or Shannon Entropy.
    /// Returns `null` if inputs, labels, criterion, or workspace buffers are invalid.
    /// If `nodes_buf` or the workspace stack runs out of space mid-training, active nodes
    /// gracefully degrade into leaf nodes without error.
    static fn fitClassifier(
        x: []const f64,
        y: []const i32,
        n_samples: usize,
        n_features: usize,
        num_classes: usize,
        cfg: TreeConfig,
        nodes_buf: *[]TreeNode,
        ws: *TreeWorkspace,
    ): DecisionTree? {
        const criterion_ok: bool = match (cfg.criterion) {
            .Gini => true,
            .Entropy => true,
            .Mse => false,
        };
        if (!criterion_ok) {
            return null;
        }
        if (n_samples == 0 || n_features == 0 || num_classes == 0) {
            return null;
        }
        if (!checkMatrixBounds(x.len, n_samples, n_features) || y.len < n_samples) {
            return null;
        }
        if (cfg.min_samples_leaf == 0 || cfg.min_samples_split < 2) {
            return null;
        }
        if (nodes_buf.*.len == 0) {
            return null;
        }
        if (ws.*.sample_indices.len < n_samples || ws.*.sort_tmp.len < n_samples) {
            return null;
        }
        if (ws.*.class_counts.len < num_classes || ws.*.left_counts.len < num_classes) {
            return null;
        }
        const stack_cap: usize = minStackCap(ws);
        if (stack_cap == 0) {
            return null;
        }
        if (!checkFinitePrefix(x, n_samples * n_features)) {
            return null;
        }

        let i: usize = 0;
        while (i < n_samples) {
            const lbl: i32 = y[i];
            if (lbl < 0 || (lbl as usize) >= num_classes) {
                return null;
            }
            ws.*.sample_indices[i] = i;
            i += 1;
        }

        let nodes: []TreeNode = nodes_buf.*;
        nodes[0] = TreeNode.empty();
        let node_count: usize = 1;
        let max_depth_reached: usize = 0;

        ws.*.stack_node[0] = 0;
        ws.*.stack_lo[0] = 0;
        ws.*.stack_hi[0] = n_samples;
        ws.*.stack_depth[0] = 0;
        let sp: usize = 1;

        while (sp > 0) {
            sp -= 1;
            const node_idx: usize = ws.*.stack_node[sp] as usize;
            const lo: usize = ws.*.stack_lo[sp];
            const hi: usize = ws.*.stack_hi[sp];
            const cur_depth: usize = ws.*.stack_depth[sp];
            const m: usize = hi - lo;

            if (cur_depth > max_depth_reached) {
                max_depth_reached = cur_depth;
            }

            let c: usize = 0;
            while (c < num_classes) {
                ws.*.class_counts[c] = 0;
                c += 1;
            }
            let s: usize = lo;
            while (s < hi) {
                const cls: usize = y[ws.*.sample_indices[s]] as usize;
                ws.*.class_counts[cls] += 1;
                s += 1;
            }

            let best_cls: usize = 0;
            let best_cnt: usize = 0;
            let k: usize = 0;
            while (k < num_classes) {
                if (ws.*.class_counts[k] > best_cnt) {
                    best_cnt = ws.*.class_counts[k];
                    best_cls = k;
                }
                k += 1;
            }

            const impurity: f64 = match (cfg.criterion) {
                .Gini => computeGini(ws.*.class_counts, num_classes, m),
                .Entropy => computeEntropy(ws.*.class_counts, num_classes, m),
                .Mse => 0.0,
            };

            nodes[node_idx] = TreeNode {
                is_leaf: true,
                feature_idx: 0,
                threshold: 0.0,
                left: DTREE_NO_NODE,
                right: DTREE_NO_NODE,
                predicted_class: best_cls as i32,
                predicted_value: best_cls as f64,
                sample_count: m,
                impurity: impurity,
            };

            const can_split: bool =
                (cur_depth < cfg.max_depth) &&
                (m >= cfg.min_samples_split) &&
                (m >= (2 * cfg.min_samples_leaf)) &&
                (impurity > SPLIT_EPS) &&
                ((node_count + 2) <= nodes.len) &&
                ((sp + 2) <= stack_cap);

            if (!can_split) {
                continue;
            }

            const split: BestSplit = findBestClassifierSplit(
                x,
                y,
                n_features,
                num_classes,
                lo,
                hi,
                impurity,
                cfg,
                ws,
            );
            if (!split.found) {
                continue;
            }

            const left_n: usize = partitionIndices(
                ws.*.sample_indices,
                ws.*.sort_tmp,
                lo,
                hi,
                x,
                n_features,
                split.feature_idx,
                split.threshold,
            );
            const right_n: usize = m - left_n;
            if (left_n < cfg.min_samples_leaf || right_n < cfg.min_samples_leaf) {
                continue;
            }

            const left_id: u32 = node_count as u32;
            const right_id: u32 = (node_count + 1) as u32;
            nodes[node_count] = TreeNode.empty();
            nodes[node_count + 1] = TreeNode.empty();
            node_count += 2;

            nodes[node_idx].is_leaf = false;
            nodes[node_idx].feature_idx = split.feature_idx;
            nodes[node_idx].threshold = split.threshold;
            nodes[node_idx].left = left_id;
            nodes[node_idx].right = right_id;

            const mid: usize = lo + left_n;
            // Push right first so left is processed first (DFS pre-order).
            ws.*.stack_node[sp] = right_id;
            ws.*.stack_lo[sp] = mid;
            ws.*.stack_hi[sp] = hi;
            ws.*.stack_depth[sp] = cur_depth + 1;
            sp += 1;

            ws.*.stack_node[sp] = left_id;
            ws.*.stack_lo[sp] = lo;
            ws.*.stack_hi[sp] = mid;
            ws.*.stack_depth[sp] = cur_depth + 1;
            sp += 1;
        }

        return DecisionTree {
            nodes: nodes,
            node_count: node_count,
            num_features: n_features,
            num_classes: num_classes,
            is_regressor: false,
            max_depth_reached: max_depth_reached,
        };
    }

    /// Fits an axis-aligned CART regressor minimizing Mean Squared Error (MSE).
    /// Returns `null` if inputs, targets, criterion, or workspace buffers are invalid.
    /// If `nodes_buf` or the workspace stack runs out of space mid-training, active nodes
    /// gracefully degrade into leaf nodes without error.
    static fn fitRegressor(
        x: []const f64,
        y: []const f64,
        n_samples: usize,
        n_features: usize,
        cfg: TreeConfig,
        nodes_buf: *[]TreeNode,
        ws: *TreeWorkspace,
    ): DecisionTree? {
        const criterion_ok: bool = match (cfg.criterion) {
            .Mse => true,
            .Gini => false,
            .Entropy => false,
        };
        if (!criterion_ok) {
            return null;
        }
        if (n_samples == 0 || n_features == 0) {
            return null;
        }
        if (!checkMatrixBounds(x.len, n_samples, n_features) || y.len < n_samples) {
            return null;
        }
        if (cfg.min_samples_leaf == 0 || cfg.min_samples_split < 2) {
            return null;
        }
        if (nodes_buf.*.len == 0) {
            return null;
        }
        if (ws.*.sample_indices.len < n_samples || ws.*.sort_tmp.len < n_samples) {
            return null;
        }
        const stack_cap: usize = minStackCap(ws);
        if (stack_cap == 0) {
            return null;
        }
        if (!checkFinitePrefix(x, n_samples * n_features) || !checkFinitePrefix(y, n_samples)) {
            return null;
        }

        let i: usize = 0;
        while (i < n_samples) {
            ws.*.sample_indices[i] = i;
            i += 1;
        }

        let nodes: []TreeNode = nodes_buf.*;
        nodes[0] = TreeNode.empty();
        let node_count: usize = 1;
        let max_depth_reached: usize = 0;

        ws.*.stack_node[0] = 0;
        ws.*.stack_lo[0] = 0;
        ws.*.stack_hi[0] = n_samples;
        ws.*.stack_depth[0] = 0;
        let sp: usize = 1;

        while (sp > 0) {
            sp -= 1;
            const node_idx: usize = ws.*.stack_node[sp] as usize;
            const lo: usize = ws.*.stack_lo[sp];
            const hi: usize = ws.*.stack_hi[sp];
            const cur_depth: usize = ws.*.stack_depth[sp];
            const m: usize = hi - lo;
            const m_f: f64 = m as f64;

            if (cur_depth > max_depth_reached) {
                max_depth_reached = cur_depth;
            }

            let sum_y: f64 = 0.0;
            let s: usize = lo;
            while (s < hi) {
                sum_y += y[ws.*.sample_indices[s]];
                s += 1;
            }
            const mean_y: f64 = sum_y / m_f;

            let centered_sum: f64 = 0.0;
            let sse: f64 = 0.0;
            let t: usize = lo;
            while (t < hi) {
                const diff: f64 = y[ws.*.sample_indices[t]] - mean_y;
                centered_sum += diff;
                sse += diff * diff;
                t += 1;
            }
            const mse: f64 = if (sse < 0.0) 0.0 else (sse / m_f);

            nodes[node_idx] = TreeNode {
                is_leaf: true,
                feature_idx: 0,
                threshold: 0.0,
                left: DTREE_NO_NODE,
                right: DTREE_NO_NODE,
                predicted_class: 0,
                predicted_value: mean_y,
                sample_count: m,
                impurity: mse,
            };

            const can_split: bool =
                (cur_depth < cfg.max_depth) &&
                (m >= cfg.min_samples_split) &&
                (m >= (2 * cfg.min_samples_leaf)) &&
                (mse > SPLIT_EPS) &&
                ((node_count + 2) <= nodes.len) &&
                ((sp + 2) <= stack_cap);

            if (!can_split) {
                continue;
            }

            const split: BestSplit = findBestRegressorSplit(
                x,
                y,
                n_features,
                lo,
                hi,
                mean_y,
                centered_sum,
                cfg,
                ws,
            );
            if (!split.found) {
                continue;
            }

            const left_n: usize = partitionIndices(
                ws.*.sample_indices,
                ws.*.sort_tmp,
                lo,
                hi,
                x,
                n_features,
                split.feature_idx,
                split.threshold,
            );
            const right_n: usize = m - left_n;
            if (left_n < cfg.min_samples_leaf || right_n < cfg.min_samples_leaf) {
                continue;
            }

            const left_id: u32 = node_count as u32;
            const right_id: u32 = (node_count + 1) as u32;
            nodes[node_count] = TreeNode.empty();
            nodes[node_count + 1] = TreeNode.empty();
            node_count += 2;

            nodes[node_idx].is_leaf = false;
            nodes[node_idx].feature_idx = split.feature_idx;
            nodes[node_idx].threshold = split.threshold;
            nodes[node_idx].left = left_id;
            nodes[node_idx].right = right_id;

            const mid: usize = lo + left_n;
            ws.*.stack_node[sp] = right_id;
            ws.*.stack_lo[sp] = mid;
            ws.*.stack_hi[sp] = hi;
            ws.*.stack_depth[sp] = cur_depth + 1;
            sp += 1;

            ws.*.stack_node[sp] = left_id;
            ws.*.stack_lo[sp] = lo;
            ws.*.stack_hi[sp] = mid;
            ws.*.stack_depth[sp] = cur_depth + 1;
            sp += 1;
        }

        return DecisionTree {
            nodes: nodes,
            node_count: node_count,
            num_features: n_features,
            num_classes: 0,
            is_regressor: true,
            max_depth_reached: max_depth_reached,
        };
    }

    /// Predicts the class label for a single feature vector.
    /// Returns `null` if this tree is a regressor, uninitialized, or `sample` is short or non-finite.
    fn predictClass(sample: []const f64): i32? {
        if (_.is_regressor || _.node_count == 0 || _.num_features == 0) {
            return null;
        }
        if (sample.len < _.num_features || !checkFinitePrefix(sample, _.num_features)) {
            return null;
        }

        let cur: usize = 0;
        let steps: usize = 0;
        while (steps < _.node_count) {
            const node: TreeNode = _.nodes[cur];
            if (node.is_leaf) {
                return node.predicted_class;
            }
            if (node.feature_idx >= _.num_features) {
                return null;
            }
            const next_id: u32 = if (sample[node.feature_idx] <= node.threshold) node.left else node.right;
            if (next_id == DTREE_NO_NODE || (next_id as usize) >= _.node_count) {
                return null;
            }
            cur = next_id as usize;
            steps += 1;
        }
        return null;
    }

    /// Predicts the continuous target value for a single feature vector.
    /// Returns `null` if this tree is a classifier, uninitialized, or `sample` is short or non-finite.
    fn predictValue(sample: []const f64): f64? {
        if (!_.is_regressor || _.node_count == 0 || _.num_features == 0) {
            return null;
        }
        if (sample.len < _.num_features || !checkFinitePrefix(sample, _.num_features)) {
            return null;
        }

        let cur: usize = 0;
        let steps: usize = 0;
        while (steps < _.node_count) {
            const node: TreeNode = _.nodes[cur];
            if (node.is_leaf) {
                return node.predicted_value;
            }
            if (node.feature_idx >= _.num_features) {
                return null;
            }
            const next_id: u32 = if (sample[node.feature_idx] <= node.threshold) node.left else node.right;
            if (next_id == DTREE_NO_NODE || (next_id as usize) >= _.node_count) {
                return null;
            }
            cur = next_id as usize;
            steps += 1;
        }
        return null;
    }

    /// Returns the total number of allocated nodes in the fitted tree.
    fn nodeCount(): usize {
        return _.node_count;
    }

    /// Returns the number of leaf nodes in the fitted tree.
    fn leafCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].is_leaf) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    /// Returns the maximum depth of the fitted tree (root has depth 0).
    fn depth(): usize {
        return _.max_depth_reached;
    }

    /// Computes normalized feature importances based on weighted impurity decrease.
    /// Writes `num_features` values summing to `1.0` (or all `0.0` for a single-leaf tree) into `out`.
    fn featureImportances(out: *[]f64): bool {
        if (_.node_count == 0 || _.num_features == 0 || out.*.len < _.num_features) {
            return false;
        }

        let dst: []f64 = out.*;
        let f: usize = 0;
        while (f < _.num_features) {
            dst[f] = 0.0;
            f += 1;
        }

        let i: usize = 0;
        while (i < _.node_count) {
            const node: TreeNode = _.nodes[i];
            if (!node.is_leaf && node.left != DTREE_NO_NODE && node.right != DTREE_NO_NODE) {
                const li: usize = node.left as usize;
                const ri: usize = node.right as usize;
                if (li < _.node_count && ri < _.node_count && node.feature_idx < _.num_features) {
                    const left_node: TreeNode = _.nodes[li];
                    const right_node: TreeNode = _.nodes[ri];
                    const parent_w: f64 = (node.sample_count as f64) * node.impurity;
                    const left_w: f64 = (left_node.sample_count as f64) * left_node.impurity;
                    const right_w: f64 = (right_node.sample_count as f64) * right_node.impurity;
                    const delta: f64 = parent_w - left_w - right_w;
                    if (delta > 0.0) {
                        dst[node.feature_idx] += delta;
                    }
                }
            }
            i += 1;
        }

        let total: f64 = 0.0;
        let j: usize = 0;
        while (j < _.num_features) {
            total += dst[j];
            j += 1;
        }
        if (total > 0.0) {
            let k: usize = 0;
            while (k < _.num_features) {
                dst[k] = dst[k] / total;
                k += 1;
            }
        }
        return true;
    }
}
