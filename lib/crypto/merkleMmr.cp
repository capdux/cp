// Origin: Pure-CP Stdlib (Zero-Alloc Merkle Mountain Range Append-Only Cryptographic Accumulator) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation Merkle Mountain Range (MMR) append-only accumulator:
//   - Flat inline MMR node array (`MMR_MAX_LEAVES = 128`, `MMR_MAX_NODES = 256`, `MMR_MAX_PEAKS = 16`)
//   - Strict domain-separated SHA-256 hashing preventing second-preimage ambiguity:
//       * leaf   = SHA256(0x00 || pos_le64 || payload)
//       * parent = SHA256(0x01 || pos_le64 || left || right)
//       * bag    = SHA256(0x02 || mmr_size_le64 || left_peak || right_bag)
//   - O(log N) append-only `appendLeaf`, right-to-left `bagPeaks` root folding, `peaks` extraction,
//     stateless O(log N) inclusion proof generation & constant-time verification (`generateProof` / `verifyMmrProof`),
//     and O(log N) historical prefix root verification (`rootAtLeafCount` / `verifyPrefixRoot`)

const std = @import("std");

/// SHA-256 digest length in bytes.
pub const MMR_HASH_LEN: usize = 32;

/// Maximum number of leaves stored inline in `MerkleMmr` (zero dynamic heap).
pub const MMR_MAX_LEAVES: usize = 128;

/// Maximum number of flat MMR nodes (`2 * 128 - 1 = 255`, padded to `256`).
pub const MMR_MAX_NODES: usize = 256;

/// Maximum tree height (`log2(128) = 7`, padded to `8`).
pub const MMR_MAX_HEIGHT: usize = 8;

/// Maximum simultaneous mountain peaks (`<= 8` for 128 leaves, padded to `16`).
pub const MMR_MAX_PEAKS: usize = 16;

// ============================================================================
// 1. Constant-Time & Domain-Separated SHA-256 Primitives
// ============================================================================

/// Constant-time equality comparison for two 32-byte MMR digests.
pub fn mmrConstantTimeEq(a: [MMR_HASH_LEN]u8, b: [MMR_HASH_LEN]u8): bool {
    let diff: u8 = 0;
    let idx: usize = 0;
    while (idx < MMR_HASH_LEN) {
        diff |= a[idx] ^ b[idx];
        idx += 1;
    }
    return diff == 0;
}

fn encodeU64Le(v: u64): [8]u8 {
    return [
        @as(u8, @intCast(v & 0xFF)),
        @as(u8, @intCast((v >> 8) & 0xFF)),
        @as(u8, @intCast((v >> 16) & 0xFF)),
        @as(u8, @intCast((v >> 24) & 0xFF)),
        @as(u8, @intCast((v >> 32) & 0xFF)),
        @as(u8, @intCast((v >> 40) & 0xFF)),
        @as(u8, @intCast((v >> 48) & 0xFF)),
        @as(u8, @intCast((v >> 56) & 0xFF)),
    ];
}

/// Computes the canonical empty MMR root hash: `SHA256("")`.
pub fn mmrHashEmpty(): [MMR_HASH_LEN]u8 {
    let out: [MMR_HASH_LEN]u8 = [0; MMR_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    h.final(&out);
    return out;
}

/// Computes domain-separated MMR leaf hash: `SHA256(0x00 || pos_le64 || payload)`.
pub fn mmrHashLeaf(pos: u64, payload: []const u8): [MMR_HASH_LEN]u8 {
    let out: [MMR_HASH_LEN]u8 = [0; MMR_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const tag: [1]u8 = [0x00];
    const pos_le: [8]u8 = encodeU64Le(pos);
    h.update(&tag);
    h.update(&pos_le);
    h.update(payload);
    h.final(&out);
    return out;
}

/// Computes domain-separated MMR internal parent hash: `SHA256(0x01 || pos_le64 || left || right)`.
pub fn mmrHashParent(pos: u64, left: [MMR_HASH_LEN]u8, right: [MMR_HASH_LEN]u8): [MMR_HASH_LEN]u8 {
    let out: [MMR_HASH_LEN]u8 = [0; MMR_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const tag: [1]u8 = [0x01];
    const pos_le: [8]u8 = encodeU64Le(pos);
    h.update(&tag);
    h.update(&pos_le);
    h.update(&left);
    h.update(&right);
    h.final(&out);
    return out;
}

/// Computes domain-separated peak bagging hash: `SHA256(0x02 || mmr_size_le64 || left_peak || right_bag)`.
pub fn mmrHashBag(mmr_size: u64, left_peak: [MMR_HASH_LEN]u8, right_bag: [MMR_HASH_LEN]u8): [MMR_HASH_LEN]u8 {
    let out: [MMR_HASH_LEN]u8 = [0; MMR_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const tag: [1]u8 = [0x02];
    const size_le: [8]u8 = encodeU64Le(mmr_size);
    h.update(&tag);
    h.update(&size_le);
    h.update(&left_peak);
    h.update(&right_bag);
    h.final(&out);
    return out;
}

// ============================================================================
// 2. Public Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Descriptor of a single mountain peak inside an MMR.
pub struct MmrPeak {
    pos: u64 = 0;
    height: u8 = 0;
    hash: [MMR_HASH_LEN]u8 = [0; MMR_HASH_LEN];
}

/// Self-contained O(log N) MMR inclusion proof for a single leaf.
pub struct MmrInclusionProof {
    leaf_index: u64 = 0;
    leaf_pos: u64 = 0;
    mmr_size: u64 = 0;
    sibling_count: usize = 0;
    sibling_hashes: [MMR_MAX_HEIGHT][MMR_HASH_LEN]u8 = [[0; MMR_HASH_LEN]; MMR_MAX_HEIGHT];
    /// `true` if `sibling_hashes[d]` is the right child (current node is left); `false` if sibling is left.
    sibling_is_right: [MMR_MAX_HEIGHT]bool = [false; MMR_MAX_HEIGHT];
    /// Exact 0-based MMR position of the parent node at each climb step `d`.
    parent_positions: [MMR_MAX_HEIGHT]u64 = [0; MMR_MAX_HEIGHT];
    /// Hashes of all peaks strictly to the left of the leaf's mountain (ordered left-to-right).
    left_peak_count: usize = 0;
    left_peaks: [MMR_MAX_PEAKS][MMR_HASH_LEN]u8 = [[0; MMR_HASH_LEN]; MMR_MAX_PEAKS];
    /// Pre-bagged hash of all peaks strictly to the right of the leaf's mountain, if any.
    has_right_bag: bool = false;
    right_bag_hash: [MMR_HASH_LEN]u8 = [0; MMR_HASH_LEN];
}

/// Zero-heap telemetry snapshot of `MerkleMmr`.
pub struct MmrStats {
    max_leaves: usize = MMR_MAX_LEAVES;
    max_nodes: usize = MMR_MAX_NODES;
    leaf_count: usize = 0;
    node_count: usize = 0;
    peak_count: usize = 0;
    max_height: u8 = 0;
}

// ============================================================================
// 3. Stateless Proof Verifier
// ============================================================================

/// Independently verifies an `MmrInclusionProof` against `root` and `leaf_data` in constant time.
pub fn verifyMmrProof(root: [MMR_HASH_LEN]u8, proof: MmrInclusionProof, leaf_data: []const u8): bool {
    if (proof.mmr_size == 0
        || proof.leaf_pos >= proof.mmr_size
        || proof.sibling_count > MMR_MAX_HEIGHT
        || proof.left_peak_count > MMR_MAX_PEAKS)
    {
        return false;
    }

    let cur: [MMR_HASH_LEN]u8 = mmrHashLeaf(proof.leaf_pos, leaf_data);
    let d: usize = 0;
    while (d < proof.sibling_count) {
        const p_pos: u64 = proof.parent_positions[d];
        if (p_pos >= proof.mmr_size) {
            return false;
        }
        if (proof.sibling_is_right[d]) {
            cur = mmrHashParent(p_pos, cur, proof.sibling_hashes[d]);
        } else {
            cur = mmrHashParent(p_pos, proof.sibling_hashes[d], cur);
        }
        d += 1;
    }

    // Bag with pre-folded right peaks (if any), then fold left peaks from right to left.
    if (proof.has_right_bag) {
        cur = mmrHashBag(proof.mmr_size, cur, proof.right_bag_hash);
    }

    let lp: usize = proof.left_peak_count;
    while (lp > 0) {
        lp -= 1;
        cur = mmrHashBag(proof.mmr_size, proof.left_peaks[lp], cur);
    }

    return mmrConstantTimeEq(cur, root);
}

// ============================================================================
// 4. MerkleMmr Core Accumulator
// ============================================================================

/// Zero-heap append-only Merkle Mountain Range cryptographic accumulator.
pub struct MerkleMmr {
    nodes: [MMR_MAX_NODES][MMR_HASH_LEN]u8 = [[0; MMR_HASH_LEN]; MMR_MAX_NODES];
    heights: [MMR_MAX_NODES]u8 = [0; MMR_MAX_NODES];
    leaf_positions: [MMR_MAX_LEAVES]u64 = [0; MMR_MAX_LEAVES];
    peak_positions: [MMR_MAX_PEAKS]u64 = [0; MMR_MAX_PEAKS];
    leaf_count: usize = 0;
    node_count: usize = 0;
    peak_count: usize = 0;

    /// Initializes an empty `MerkleMmr`.
    static fn init(): MerkleMmr {
        return MerkleMmr{};
    }

    /// Resets the MMR back to empty state.
    fn clear(*_) {
        _.leaf_count = 0;
        _.node_count = 0;
        _.peak_count = 0;
    }

    /// Returns the number of appended leaves (`0..=128`).
    fn leafCount(): usize {
        return _.leaf_count;
    }

    /// Returns the total number of MMR nodes (`0..=255`).
    fn nodeCount(): usize {
        return _.node_count;
    }

    /// Returns the current number of mountain peaks.
    fn peakCount(): usize {
        return _.peak_count;
    }

    /// Appends a new leaf payload in O(log N) time, merging equal-height adjacent peaks automatically.
    /// Returns the new leaf's 0-based MMR node position, or `null` if capacity is reached.
    fn appendLeaf(*_, data: []const u8): u64? {
        if (_.leaf_count >= MMR_MAX_LEAVES || _.node_count >= MMR_MAX_NODES) {
            return null;
        }

        const leaf_pos: u64 = @as(u64, _.node_count);
        _.nodes[_.node_count] = mmrHashLeaf(leaf_pos, data);
        _.heights[_.node_count] = 0;
        _.leaf_positions[_.leaf_count] = leaf_pos;
        _.leaf_count += 1;
        _.node_count += 1;

        let cur_pos: u64 = leaf_pos;
        let cur_h: u8 = 0;

        // Merge with left neighbor peak whenever heights match.
        while (_.peak_count > 0) {
            const left_pos: u64 = _.peak_positions[_.peak_count - 1];
            const left_h: u8 = _.heights[@as(usize, left_pos)];
            if (left_h != cur_h) {
                break;
            }
            if (_.node_count >= MMR_MAX_NODES) {
                return null;
            }
            _.peak_count -= 1;

            const parent_pos: u64 = @as(u64, _.node_count);
            _.nodes[_.node_count] = mmrHashParent(
                parent_pos,
                _.nodes[@as(usize, left_pos)],
                _.nodes[@as(usize, cur_pos)],
            );
            _.heights[_.node_count] = cur_h + 1;
            _.node_count += 1;

            cur_pos = parent_pos;
            cur_h += 1;
        }

        if (_.peak_count < MMR_MAX_PEAKS) {
            _.peak_positions[_.peak_count] = cur_pos;
            _.peak_count += 1;
        }
        return leaf_pos;
    }

    /// Writes all current mountain peaks (left-to-right, descending height) into `out_peaks`.
    /// Returns the number of peaks written.
    fn peaks(out_peaks: *[]MmrPeak): usize {
        const n: usize = if (_.peak_count < out_peaks.len) _.peak_count else out_peaks.len;
        let idx: usize = 0;
        while (idx < n) {
            const p_pos: u64 = _.peak_positions[idx];
            const pu: usize = @as(usize, p_pos);
            out_peaks[idx] = MmrPeak{
                pos: p_pos,
                height: _.heights[pu],
                hash: _.nodes[pu],
            };
            idx += 1;
        }
        return n;
    }

    /// Folds all current mountain peaks right-to-left into a single 32-byte MMR root digest.
    fn bagPeaks(): [MMR_HASH_LEN]u8 {
        if (_.peak_count == 0 || _.node_count == 0) {
            return mmrHashEmpty();
        }
        const mmr_size: u64 = @as(u64, _.node_count);
        let acc: [MMR_HASH_LEN]u8 = _.nodes[@as(usize, _.peak_positions[_.peak_count - 1])];
        let idx: usize = _.peak_count - 1;
        while (idx > 0) {
            idx -= 1;
            acc = mmrHashBag(mmr_size, _.nodes[@as(usize, _.peak_positions[idx])], acc);
        }
        return acc;
    }

    /// Generates a self-contained O(log N) inclusion proof for `leaf_index` (`0..leaf_count-1`).
    fn generateProof(leaf_index: usize): MmrInclusionProof? {
        if (leaf_index >= _.leaf_count || _.peak_count == 0) {
            return null;
        }

        const leaf_pos: u64 = _.leaf_positions[leaf_index];

        // Locate the unique mountain peak covering `leaf_pos`.
        let peak_idx: usize = 0;
        let found_peak: bool = false;
        let pk: usize = 0;
        while (pk < _.peak_count) {
            if (leaf_pos <= _.peak_positions[pk]) {
                peak_idx = pk;
                found_peak = true;
                break;
            }
            pk += 1;
        }
        if (!found_peak) {
            return null;
        }

        const target_peak_pos: u64 = _.peak_positions[peak_idx];
        let proof = MmrInclusionProof{
            leaf_index: @as(u64, leaf_index),
            leaf_pos: leaf_pos,
            mmr_size: @as(u64, _.node_count),
        };

        // Climb from `leaf_pos` up to `target_peak_pos`, recording siblings and exact parent positions.
        let cur_pos: u64 = leaf_pos;
        let cur_h: u8 = 0;
        while (cur_pos != target_peak_pos) {
            if (proof.sibling_count >= MMR_MAX_HEIGHT) {
                return null;
            }
            const shift_amt: u6 = @as(u6, @intCast(cur_h + 1));
            const sib_offset: u64 = (@as(u64, 1) << shift_amt) - 1;

            const is_right_child: bool = (cur_pos + 1 < @as(u64, _.node_count))
                && (_.heights[@as(usize, cur_pos + 1)] == cur_h + 1);

            const d: usize = proof.sibling_count;
            if (is_right_child) {
                const left_sib: u64 = cur_pos - sib_offset;
                const parent_pos: u64 = cur_pos + 1;
                proof.sibling_hashes[d] = _.nodes[@as(usize, left_sib)];
                proof.sibling_is_right[d] = false;
                proof.parent_positions[d] = parent_pos;
                cur_pos = parent_pos;
            } else {
                const right_sib: u64 = cur_pos + sib_offset;
                const parent_pos: u64 = right_sib + 1;
                proof.sibling_hashes[d] = _.nodes[@as(usize, right_sib)];
                proof.sibling_is_right[d] = true;
                proof.parent_positions[d] = parent_pos;
                cur_pos = parent_pos;
            }
            proof.sibling_count += 1;
            cur_h += 1;
        }

        // Copy left peaks (0..peak_idx).
        let lp: usize = 0;
        while (lp < peak_idx && lp < MMR_MAX_PEAKS) {
            proof.left_peaks[lp] = _.nodes[@as(usize, _.peak_positions[lp])];
            lp += 1;
        }
        proof.left_peak_count = lp;

        // Pre-bag all peaks to the right of `peak_idx` right-to-left.
        if (peak_idx + 1 < _.peak_count) {
            const mmr_size: u64 = @as(u64, _.node_count);
            let r_acc: [MMR_HASH_LEN]u8 = _.nodes[@as(usize, _.peak_positions[_.peak_count - 1])];
            let rp: usize = _.peak_count - 1;
            while (rp > peak_idx + 1) {
                rp -= 1;
                r_acc = mmrHashBag(mmr_size, _.nodes[@as(usize, _.peak_positions[rp])], r_acc);
            }
            proof.has_right_bag = true;
            proof.right_bag_hash = r_acc;
        }

        return proof;
    }

    /// Recomputes the exact historical MMR root at an earlier `prefix_leaf_count` (`0..=leaf_count`) in O(log N).
    fn rootAtLeafCount(prefix_leaf_count: usize): ([MMR_HASH_LEN]u8)? {
        if (prefix_leaf_count > _.leaf_count) {
            return null;
        }
        if (prefix_leaf_count == 0) {
            return mmrHashEmpty();
        }

        // Binary decomposition of `prefix_leaf_count` yields exact historical peak heights & positions.
        let hist_peaks: [MMR_MAX_PEAKS]u64 = [0; MMR_MAX_PEAKS];
        let hist_count: usize = 0;
        let offset: u64 = 0;

        let bit: usize = MMR_MAX_HEIGHT;
        while (bit > 0) {
            bit -= 1;
            const mask: usize = @as(usize, 1) << @as(u6, @intCast(bit));
            if ((prefix_leaf_count & mask) != 0) {
                const sub_nodes: u64 = (@as(u64, 1) << @as(u6, @intCast(bit + 1))) - 1;
                const peak_pos: u64 = offset + sub_nodes - 1;
                if (peak_pos >= @as(u64, _.node_count) || hist_count >= MMR_MAX_PEAKS) {
                    return null;
                }
                hist_peaks[hist_count] = peak_pos;
                hist_count += 1;
                offset += sub_nodes;
            }
        }

        if (hist_count == 0) {
            return mmrHashEmpty();
        }

        let acc: [MMR_HASH_LEN]u8 = _.nodes[@as(usize, hist_peaks[hist_count - 1])];
        let idx: usize = hist_count - 1;
        while (idx > 0) {
            idx -= 1;
            acc = mmrHashBag(offset, _.nodes[@as(usize, hist_peaks[idx])], acc);
        }
        return acc;
    }

    /// Verifies that the historical prefix of `prefix_leaf_count` leaves matches `expected_root`.
    fn verifyPrefixRoot(prefix_leaf_count: usize, expected_root: [MMR_HASH_LEN]u8): bool {
        const computed = _.rootAtLeafCount(prefix_leaf_count) ?? return false;
        return mmrConstantTimeEq(computed, expected_root);
    }

    /// Returns a zero-heap telemetry snapshot of the MMR.
    fn stats(): MmrStats {
        let max_h: u8 = 0;
        if (_.peak_count > 0) {
            max_h = _.heights[@as(usize, _.peak_positions[0])];
        }
        return MmrStats{
            max_leaves: MMR_MAX_LEAVES,
            max_nodes: MMR_MAX_NODES,
            leaf_count: _.leaf_count,
            node_count: _.node_count,
            peak_count: _.peak_count,
            max_height: max_h,
        };
    }
}
