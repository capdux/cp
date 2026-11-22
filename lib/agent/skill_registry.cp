// Origin: Pure-CP Stdlib (AI Agent Skill Registry & Capability Matcher) | Created: 2026-11-22
// Pure CP zero-allocation AI Agent Skill / Plugin registry and capability matcher providing:
// - Fixed-slot skill registration, deduplication (ID & name), lifecycle management, and lookup
// - Bitmask capability matching (`required_caps` / `preferred_caps`), tag filtering, and
//   priority/cost-weighted deterministic Top-K ranking
// - Dependency graph validation and deterministic topological activation order (`resolveActivationOrder`)
//   with missing-dependency, self-cycle, and cycle rejection
// - Per-skill concurrency quota tracking (`tryAcquire` / `release`) and aggregate `SkillRegistryStats`

/// Sentinel value indicating an unused dependency slot on `SkillDescriptor`.
pub const SKILL_NO_DEP: u32 = 0xFFFFFFFF;

/// Lifecycle status of a registered agent skill or plugin.
pub enum SkillStatus {
    Active,
    Disabled,
    Deprecated,
}

/// Fixed-layout descriptor for a registered agent skill/plugin.
/// `max_concurrency == 0` means unlimited concurrency; unused tag hashes are `0`;
/// unused dependency slots are `SKILL_NO_DEP`.
pub struct SkillDescriptor {
    id: u32 = 0,
    name: string = "",
    version: u32 = 1,
    capability_mask: u64 = 0,
    priority: i32 = 0,
    cost_units: u32 = 0,
    max_concurrency: u32 = 0,
    in_flight: u32 = 0,
    tag_hash_0: u64 = 0,
    tag_hash_1: u64 = 0,
    tag_hash_2: u64 = 0,
    dep_0: u32 = SKILL_NO_DEP,
    dep_1: u32 = SKILL_NO_DEP,
    dep_2: u32 = SKILL_NO_DEP,
    dep_3: u32 = SKILL_NO_DEP,
    status: SkillStatus = SkillStatus.Active,
    occupied: bool = false,
}

/// Query criteria for capability, tag, cost, and concurrency-aware skill matching.
pub struct SkillQuery {
    required_caps: u64 = 0,
    preferred_caps: u64 = 0,
    required_tag_hash: u64 = 0,
    max_cost_units: u32 = 0,
    require_available_slot: bool = false,
    include_deprecated: bool = false,
}

/// Ranked candidate result emitted by `SkillRegistry.matchTopK`.
pub struct SkillMatch {
    skill_id: u32 = 0,
    slot_index: usize = 0,
    score: i64 = 0,
}

/// Aggregate health and utilization snapshot of a `SkillRegistry`.
pub struct SkillRegistryStats {
    total_registered: usize = 0,
    active_count: usize = 0,
    disabled_count: usize = 0,
    deprecated_count: usize = 0,
    total_in_flight: u64 = 0,
}

// ============================================================================
// Internal Helpers & Public Tag Hash Utility
// ============================================================================

fn strEquals(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn popcount64(x: u64): u32 {
    let v = x;
    let c: u32 = 0;
    while (v != 0) {
        v &= (v - 1);
        c += 1;
    }
    return c;
}

fn getDepAt(s: SkillDescriptor, idx: usize): u32 {
    if (idx == 0) return s.dep_0;
    if (idx == 1) return s.dep_1;
    if (idx == 2) return s.dep_2;
    return s.dep_3;
}

fn hasDepId(s: SkillDescriptor, dep_id: u32): bool {
    if (dep_id == SKILL_NO_DEP) {
        return false;
    }
    return s.dep_0 == dep_id || s.dep_1 == dep_id || s.dep_2 == dep_id || s.dep_3 == dep_id;
}

/// Computes a deterministic 64-bit FNV-1a hash for a skill tag (ASCII case-insensitive).
/// Returns `0` for empty tag; guarantees a non-zero hash for non-empty tags.
pub fn skillTagHash(tag: string): u64 {
    if (tag.len == 0) {
        return 0;
    }
    let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    let i: usize = 0;
    while (i < tag.len) : (i += 1) {
        let b = tag[i];
        if (b >= 'A' && b <= 'Z') {
            b += 32;
        }
        h = (h ^ (b as u64)) *% 1099511628211;
    }
    if (h == 0) {
        return 1;
    }
    return h;
}

// ============================================================================
// Public SkillRegistry
// ============================================================================

/// Zero-allocation fixed-pool Skill / Plugin registry, capability matcher, and dependency resolver.
pub struct SkillRegistry {
    slots: *[]SkillDescriptor,

    /// Initializes a registry over caller-provided `slots`, marking all slots unoccupied.
    pub static fn init(slots: *[]SkillDescriptor): SkillRegistry {
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i].occupied = false;
            slots[i].in_flight = 0;
        }
        return SkillRegistry{
            slots: slots,
        };
    }

    fn findSlotById(skill_id: u32): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].id == skill_id) {
                return i;
            }
        }
        return null;
    }

    fn findSlotByName(name: string): usize? {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].occupied && strEquals(_.slots[i].name, name)) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new skill descriptor into the first free slot.
    /// Rejects empty name, sentinel ID (`SKILL_NO_DEP`), duplicate ID or name, and full pool.
    /// Returns the assigned slot index, or `null` on rejection.
    pub fn register(*_, desc: SkillDescriptor): usize? {
        if (desc.name.len == 0 || desc.id == SKILL_NO_DEP) {
            return null;
        }
        if (_.findSlotById(desc.id) != null || _.findSlotByName(desc.name) != null) {
            return null;
        }

        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (!_.slots[i].occupied) {
                _.slots[i] = SkillDescriptor{
                    id: desc.id,
                    name: desc.name,
                    version: desc.version,
                    capability_mask: desc.capability_mask,
                    priority: desc.priority,
                    cost_units: desc.cost_units,
                    max_concurrency: desc.max_concurrency,
                    in_flight: 0,
                    tag_hash_0: desc.tag_hash_0,
                    tag_hash_1: desc.tag_hash_1,
                    tag_hash_2: desc.tag_hash_2,
                    dep_0: desc.dep_0,
                    dep_1: desc.dep_1,
                    dep_2: desc.dep_2,
                    dep_3: desc.dep_3,
                    status: desc.status,
                    occupied: true,
                };
                return i;
            }
        }
        return null;
    }

    /// Unregisters a skill by ID. Refuses to unregister if `in_flight > 0` or ID not found.
    pub fn unregister(*_, skill_id: u32): bool {
        if (idx := _.findSlotById(skill_id)) {
            if (_.slots[idx].in_flight > 0) {
                return false;
            }
            _.slots[idx].occupied = false;
            return true;
        }
        return false;
    }

    /// Updates the lifecycle status (`Active`, `Disabled`, `Deprecated`) of a registered skill.
    pub fn setStatus(*_, skill_id: u32, status: SkillStatus): bool {
        if (idx := _.findSlotById(skill_id)) {
            _.slots[idx].status = status;
            return true;
        }
        return false;
    }

    /// Convenience helper to enable a skill (`SkillStatus.Active`).
    pub fn enable(*_, skill_id: u32): bool {
        return _.setStatus(skill_id, SkillStatus.Active);
    }

    /// Convenience helper to disable a skill (`SkillStatus.Disabled`).
    pub fn disable(*_, skill_id: u32): bool {
        return _.setStatus(skill_id, SkillStatus.Disabled);
    }

    /// Looks up a copy of a registered skill by ID.
    pub fn findById(skill_id: u32): SkillDescriptor? {
        if (idx := _.findSlotById(skill_id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Looks up a copy of a registered skill by name.
    pub fn findByName(name: string): SkillDescriptor? {
        if (idx := _.findSlotByName(name)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Acquires one concurrency slot for `skill_id`.
    /// Fails if skill is not registered, is `Disabled`, or is at `max_concurrency`.
    pub fn tryAcquire(*_, skill_id: u32): bool {
        if (idx := _.findSlotById(skill_id)) {
            if (_.slots[idx].status == SkillStatus.Disabled) {
                return false;
            }
            const cap_limit = _.slots[idx].max_concurrency;
            if (cap_limit > 0 && _.slots[idx].in_flight >= cap_limit) {
                return false;
            }
            _.slots[idx].in_flight += 1;
            return true;
        }
        return false;
    }

    /// Releases one concurrency slot for `skill_id`. Returns `false` if unknown or `in_flight == 0`.
    pub fn release(*_, skill_id: u32): bool {
        if (idx := _.findSlotById(skill_id)) {
            if (_.slots[idx].in_flight == 0) {
                return false;
            }
            _.slots[idx].in_flight -= 1;
            return true;
        }
        return false;
    }

    /// Matches and ranks up to `top_k` skills satisfying `query` by composite score:
    /// `popcount(caps & preferred_caps) * 1000 + priority * 10 - cost_units - deprecated_penalty`.
    /// Ties break deterministically by smaller `skill_id`.
    /// Returns the number of matches written into `out_matches`.
    pub fn matchTopK(query: SkillQuery, out_matches: *[]SkillMatch, top_k: usize): usize {
        const limit: usize = if (top_k < out_matches.len) top_k else out_matches.len;
        if (limit == 0) {
            return 0;
        }

        let count: usize = 0;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied) {
                continue;
            }
            if (s.status == SkillStatus.Disabled) {
                continue;
            }
            if (s.status == SkillStatus.Deprecated && !query.include_deprecated) {
                continue;
            }
            if ((s.capability_mask & query.required_caps) != query.required_caps) {
                continue;
            }
            if (query.required_tag_hash != 0) {
                const tag_hit = s.tag_hash_0 == query.required_tag_hash ||
                    s.tag_hash_1 == query.required_tag_hash ||
                    s.tag_hash_2 == query.required_tag_hash;
                if (!tag_hit) {
                    continue;
                }
            }
            if (query.max_cost_units > 0 && s.cost_units > query.max_cost_units) {
                continue;
            }
            if (query.require_available_slot && s.max_concurrency > 0 && s.in_flight >= s.max_concurrency) {
                continue;
            }

            const pref_hits = popcount64(s.capability_mask & query.preferred_caps) as i64;
            const dep_pen: i64 = if (s.status == SkillStatus.Deprecated) 500 else 0;
            const score: i64 = pref_hits * 1000 + (s.priority as i64) * 10 - (s.cost_units as i64) - dep_pen;
            const cand = SkillMatch{
                skill_id: s.id,
                slot_index: i,
                score: score,
            };

            // Insert into sorted descending top-K buffer
            let pos: usize = count;
            if (count < limit) {
                count += 1;
            } else {
                const last = out_matches[limit - 1];
                const better = cand.score > last.score || (cand.score == last.score && cand.skill_id < last.skill_id);
                if (!better) {
                    continue;
                }
                pos = limit - 1;
            }

            while (pos > 0) {
                const prev = out_matches[pos - 1];
                const ahead = cand.score > prev.score || (cand.score == prev.score && cand.skill_id < prev.skill_id);
                if (ahead) {
                    out_matches[pos] = prev;
                    pos -= 1;
                } else {
                    break;
                }
            }
            out_matches[pos] = cand;
        }

        return count;
    }

    /// Validates all skill dependencies and computes a deterministic topological activation order
    /// (dependencies strictly precede dependents; ties break by smallest `skill_id`).
    /// Scratch buffers `indeg_s` and `done_flags_s` must have length `>= slots.len`.
    /// Returns the number of skill IDs written to `out_order`, or `null` on missing dependency,
    /// self-dependency, dependency cycle, or insufficient buffer capacity.
    pub fn resolveActivationOrder(
        include_disabled: bool,
        out_order: *[]u32,
        indeg_s: *[]u32,
        done_flags_s: *[]usize,
    ): usize? {
        const cap_slots = _.slots.len;
        if (indeg_s.len < cap_slots || done_flags_s.len < cap_slots) {
            return null;
        }

        // 1. Identify participating skills and initialize in-degrees
        let participant_count: usize = 0;
        let i: usize = 0;
        while (i < cap_slots) : (i += 1) {
            const s = _.slots[i];
            const active = s.occupied && (include_disabled || s.status != SkillStatus.Disabled);
            indeg_s[i] = 0;
            done_flags_s[i] = if (active) 0 else 2; // 0=pending, 1=emitted, 2=excluded
            if (active) {
                participant_count += 1;
            }
        }

        if (participant_count > out_order.len) {
            return null;
        }
        if (participant_count == 0) {
            return 0;
        }

        // 2. Validate every declared dependency and populate in-degrees
        i = 0;
        while (i < cap_slots) : (i += 1) {
            if (done_flags_s[i] != 0) {
                continue;
            }
            const s = _.slots[i];
            let d: usize = 0;
            while (d < 4) : (d += 1) {
                const dep_id = getDepAt(s, d);
                if (dep_id == SKILL_NO_DEP) {
                    continue;
                }
                if (dep_id == s.id) {
                    return null; // Self-cycle
                }
                // Deduplicate repeated dependency ID on the same descriptor
                let dup: bool = false;
                let prev_d: usize = 0;
                while (prev_d < d) : (prev_d += 1) {
                    if (getDepAt(s, prev_d) == dep_id) {
                        dup = true;
                        break;
                    }
                }
                if (dup) {
                    continue;
                }

                // Dependency must exist and be participating
                if (dep_slot := _.findSlotById(dep_id)) {
                    if (done_flags_s[dep_slot] != 0) {
                        return null; // Dependency is disabled/excluded
                    }
                    indeg_s[i] += 1;
                } else {
                    return null; // Missing dependency
                }
            }
        }

        // 3. Deterministic Kahn topological emission (pick smallest skill_id among ready nodes)
        let emitted: usize = 0;
        while (emitted < participant_count) {
            let best_slot: usize = 0;
            let best_id: u32 = SKILL_NO_DEP;
            let found: bool = false;

            let s_idx: usize = 0;
            while (s_idx < cap_slots) : (s_idx += 1) {
                if (done_flags_s[s_idx] == 0 && indeg_s[s_idx] == 0) {
                    const sid = _.slots[s_idx].id;
                    if (!found || sid < best_id) {
                        found = true;
                        best_slot = s_idx;
                        best_id = sid;
                    }
                }
            }

            if (!found) {
                return null; // Dependency cycle detected
            }

            done_flags_s[best_slot] = 1;
            out_order[emitted] = best_id;
            emitted += 1;

            // Decrement in-degree of all pending skills depending on `best_id`
            let u_idx: usize = 0;
            while (u_idx < cap_slots) : (u_idx += 1) {
                if (done_flags_s[u_idx] == 0 && hasDepId(_.slots[u_idx], best_id)) {
                    if (indeg_s[u_idx] > 0) {
                        indeg_s[u_idx] -= 1;
                    }
                }
            }
        }

        return emitted;
    }

    /// Computes aggregate registry counts and total in-flight executions.
    pub fn stats(): SkillRegistryStats {
        let st = SkillRegistryStats{};
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied) {
                continue;
            }
            st.total_registered += 1;
            st.total_in_flight += s.in_flight as u64;
            if (s.status == SkillStatus.Active) {
                st.active_count += 1;
            } else if (s.status == SkillStatus.Disabled) {
                st.disabled_count += 1;
            } else {
                st.deprecated_count += 1;
            }
        }
        return st;
    }
}
