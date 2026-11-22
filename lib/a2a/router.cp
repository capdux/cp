// Origin: Pure-CP Stdlib (A2A Multi-Agent Capability Discovery, Skill Router & Handoff Guard) | Created: 2026-11-22
// Pure CP zero-allocation multi-agent skill bitmask routing, heartbeat health tracking,
// deterministic load-balanced selection, and cycle/hop-bounded handoff tracing.

/// Operational health state of a registered A2A peer agent.
pub enum PeerHealth {
    Healthy,
    Degraded,
    Unhealthy,
    Draining,
}

/// Fixed-layout descriptor for a registered A2A agent peer (zero heap ownership).
pub struct AgentPeer {
    id: u32 = 0,
    name: string = "",
    skill_mask: u64 = 0,
    priority: i32 = 0,
    max_concurrency: u32 = 1,
    active_tasks: u32 = 0,
    last_heartbeat_ms: i64 = 0,
    health: PeerHealth = PeerHealth.Healthy,
}

fn healthRank(h: PeerHealth): u8 {
    return switch (h) {
        PeerHealth.Healthy => 0,
        PeerHealth.Degraded => 1,
        PeerHealth.Draining => 2,
        PeerHealth.Unhealthy => 3,
    };
}

fn isRoutableHealth(h: PeerHealth): bool {
    return h == PeerHealth.Healthy || h == PeerHealth.Degraded;
}

/// Returns `true` if candidate `a` is strictly preferred over candidate `b`:
///   1. Healthier status (`Healthy` before `Degraded`)
///   2. Lower load ratio (`active_tasks / max_concurrency`, exact u64 cross-multiplication)
///   3. Higher `priority`
///   4. Smaller `id` (deterministic tie-breaker)
fn isBetterPeer(a: AgentPeer, b: AgentPeer): bool {
    const ha: u8 = healthRank(a.health);
    const hb: u8 = healthRank(b.health);
    if (ha != hb) {
        return ha < hb;
    }

    const load_a: u64 = (a.active_tasks as u64) * (b.max_concurrency as u64);
    const load_b: u64 = (b.active_tasks as u64) * (a.max_concurrency as u64);
    if (load_a != load_b) {
        return load_a < load_b;
    }

    if (a.priority != b.priority) {
        return a.priority > b.priority;
    }

    return a.id < b.id;
}

/// Zero-allocation fixed-pool multi-agent capability discovery and load-balanced router.
pub struct AgentRouter {
    peers: []AgentPeer,
    count: usize = 0,
    heartbeat_timeout_ms: i64 = 30000,

    pub static fn init(peers: *[]AgentPeer, heartbeat_timeout_ms: i64): AgentRouter {
        const timeout: i64 = if (heartbeat_timeout_ms > 0) heartbeat_timeout_ms else 0;
        return AgentRouter{
            peers: peers,
            count: 0,
            heartbeat_timeout_ms: timeout,
        };
    }

    fn findIndexById(id: u32): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.peers[i].id == id) {
                return i;
            }
        }
        return null;
    }

    fn isPeerAlive(p: AgentPeer, now_ms: i64): bool {
        if (_.heartbeat_timeout_ms <= 0) {
            return true;
        }
        if (now_ms <= p.last_heartbeat_ms) {
            return true;
        }
        return (now_ms - p.last_heartbeat_ms) <= _.heartbeat_timeout_ms;
    }

    /// Registers a new peer into the fixed pool.
    /// Rejects empty `name`, duplicate `id`, or full pool by returning `null`.
    pub fn registerPeer(
        *_,
        id: u32,
        name: string,
        skill_mask: u64,
        priority: i32,
        max_concurrency: u32,
        now_ms: i64,
    ): usize? {
        if (name.len == 0 || _.count >= _.peers.len) {
            return null;
        }
        if (_.findIndexById(id) != null) {
            return null;
        }

        const idx: usize = _.count;
        _.peers[idx] = AgentPeer{
            id: id,
            name: name,
            skill_mask: skill_mask,
            priority: priority,
            max_concurrency: max_concurrency,
            active_tasks: 0,
            last_heartbeat_ms: now_ms,
            health: PeerHealth.Healthy,
        };
        _.count += 1;
        return idx;
    }

    /// Updates a peer's heartbeat timestamp and health state. Returns `false` if `id` is unknown.
    pub fn heartbeat(*_, id: u32, now_ms: i64, health: PeerHealth): bool {
        if (idx := _.findIndexById(id)) {
            _.peers[idx].last_heartbeat_ms = now_ms;
            _.peers[idx].health = health;
            return true;
        }
        return false;
    }

    /// Decrements a peer's `active_tasks` counter upon task completion.
    /// Returns `false` if `id` is unknown or `active_tasks` is already 0.
    pub fn releaseTask(*_, id: u32): bool {
        if (idx := _.findIndexById(id)) {
            if (_.peers[idx].active_tasks == 0) {
                return false;
            }
            _.peers[idx].active_tasks -= 1;
            return true;
        }
        return false;
    }

    /// Selects the optimal peer matching `required_skills` (64-bit bitmask), alive heartbeat,
    /// `Healthy`/`Degraded` state, `active_tasks < max_concurrency`, and `id != exclude_peer_id`.
    /// Selection order: `Healthy` > `Degraded` -> lowest load ratio -> highest `priority` -> smallest `id`.
    /// Increments the selected peer's `active_tasks` and returns its `id`, or `null` if none available.
    pub fn routeTask(*_, required_skills: u64, now_ms: i64, exclude_peer_id: u32): u32? {
        let found: bool = false;
        let best_idx: usize = 0;

        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const p = _.peers[i];
            if (p.id == exclude_peer_id) {
                continue;
            }
            if ((p.skill_mask & required_skills) != required_skills) {
                continue;
            }
            if (!_.isPeerAlive(p, now_ms)) {
                continue;
            }
            if (!isRoutableHealth(p.health)) {
                continue;
            }
            if (p.active_tasks >= p.max_concurrency) {
                continue;
            }

            if (!found || isBetterPeer(p, _.peers[best_idx])) {
                found = true;
                best_idx = i;
            }
        }

        if (!found) {
            return null;
        }

        _.peers[best_idx].active_tasks += 1;
        return _.peers[best_idx].id;
    }

    /// Lists peer IDs that satisfy `required_skills`, are heartbeat-alive, and are `Healthy` or `Degraded`.
    /// Writes up to `out_ids.len` IDs into `out_ids` and returns the count written.
    pub fn listCapablePeers(required_skills: u64, now_ms: i64, out_ids: *[]u32): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.count && written < out_ids.len) : (i += 1) {
            const p = _.peers[i];
            if ((p.skill_mask & required_skills) != required_skills) {
                continue;
            }
            if (!_.isPeerAlive(p, now_ms)) {
                continue;
            }
            if (!isRoutableHealth(p.health)) {
                continue;
            }
            out_ids[written] = p.id;
            written += 1;
        }
        return written;
    }
}

/// Outcome of recording a peer hop in `HandoffTrace`.
pub enum HandoffStatus {
    Ok,
    MaxHopsExceeded,
    CycleDetected,
    BufferFull,
}

/// Zero-allocation fixed-buffer handoff chain tracker for cycle detection and hop-limit enforcement.
pub struct HandoffTrace {
    peer_ids: []u32,
    hop_count: usize = 0,
    max_hops: usize = 0,

    pub static fn init(buf: *[]u32, max_hops: usize): HandoffTrace {
        return HandoffTrace{
            peer_ids: buf,
            hop_count: 0,
            max_hops: max_hops,
        };
    }

    pub fn hasVisited(peer_id: u32): bool {
        let i: usize = 0;
        while (i < _.hop_count) : (i += 1) {
            if (_.peer_ids[i] == peer_id) {
                return true;
            }
        }
        return false;
    }

    pub fn hops(): usize {
        return _.hop_count;
    }

    /// Records a handoff hop to `peer_id`.
    /// Checks in order: cycle (`CycleDetected`), hop limit (`MaxHopsExceeded`), buffer capacity (`BufferFull`).
    pub fn recordHop(*_, peer_id: u32): HandoffStatus {
        if (_.hasVisited(peer_id)) {
            return HandoffStatus.CycleDetected;
        }
        if (_.hop_count >= _.max_hops) {
            return HandoffStatus.MaxHopsExceeded;
        }
        if (_.hop_count >= _.peer_ids.len) {
            return HandoffStatus.BufferFull;
        }
        _.peer_ids[_.hop_count] = peer_id;
        _.hop_count += 1;
        return HandoffStatus.Ok;
    }
}
