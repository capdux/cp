// Origin: Pure-CP Stdlib (Zero-Alloc Prometheus-Style Alert Rules Evaluator & `for` Hysteresis FSM) | Created: 2026-11-23
// Pure CP zero-heap-allocation Prometheus-style alert rule evaluation engine:
//   - Fixed 32-rule inline table with comparison operators (`gt | ge | lt | le | eq`), threshold,
//     `for_duration_ms` debounce window, `keep_firing_for_ms` cooldown hold, severity & `group_hash`
//   - Strict 4-state hysteresis FSM (`inactive | pending | firing | resolved`) emitting one-shot
//     fire / resolve notifications with monotonic clock rollback protection
//   - Time-bounded silence windows (by rule_id or group_hash) and parent->child alert inhibition
//     (e.g. cluster-down firing automatically inhibits downstream node alerts)
//   - Zero-heap firing/suppressed alert collection & cumulative telemetry snapshot

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of alert rules stored inline (zero dynamic heap).
pub const ALERT_MAX_RULES: usize = 32;

/// Maximum number of silence windows stored inline.
pub const ALERT_MAX_SILENCES: usize = 32;

/// Maximum number of parent->child inhibition rules stored inline.
pub const ALERT_MAX_INHIBITIONS: usize = 32;

/// Maximum byte length of an alert rule name.
pub const ALERT_NAME_MAX_LEN: usize = 48;

/// Comparison operator evaluated against `threshold`.
pub enum AlertCompareOp {
    gt,
    ge,
    lt,
    le,
    eq,
}

/// Alert severity tier.
pub enum AlertSeverity {
    info,
    warning,
    critical,
}

/// Prometheus-style 4-state hysteresis lifecycle for an alert rule.
pub enum AlertState {
    inactive,
    pending,
    firing,
    resolved,
}

/// One-shot transition notification emitted by `evaluate`.
pub enum AlertNotifyKind {
    none,
    fire,
    resolve,
}

/// Target scope of a silence window.
pub enum AlertSilenceTarget {
    by_rule,
    by_group,
}

/// Declarative specification used to register a new alert rule.
pub struct AlertRuleSpec {
    rule_id: u32 = 0;
    name: []const u8 = "";
    op: AlertCompareOp = AlertCompareOp.gt;
    threshold: f64 = 0.0;
    for_duration_ms: u64 = 0;
    keep_firing_for_ms: u64 = 0;
    severity: AlertSeverity = AlertSeverity.warning;
    group_hash: u64 = 0;
}

/// Inline alert rule runtime entry stored inside `AlertRuleEngine`.
pub struct AlertRuleEntry {
    active: bool = false;
    rule_id: u32 = 0;
    name_buf: [ALERT_NAME_MAX_LEN]u8 = [0; ALERT_NAME_MAX_LEN];
    name_len: usize = 0;
    op: AlertCompareOp = AlertCompareOp.gt;
    threshold: f64 = 0.0;
    for_duration_ms: u64 = 0;
    keep_firing_for_ms: u64 = 0;
    severity: AlertSeverity = AlertSeverity.warning;
    group_hash: u64 = 0;

    state: AlertState = AlertState.inactive;
    active_since_ms: u64 = 0;
    firing_since_ms: u64 = 0;
    keep_firing_since_ms: u64 = 0;
    in_keep_firing: bool = false;
    last_value: f64 = 0.0;
    last_eval_ms: u64 = 0;

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }
}

/// Inline time-bounded silence window matching either a `rule_id` or a `group_hash`.
pub struct AlertSilence {
    active: bool = false;
    target_kind: AlertSilenceTarget = AlertSilenceTarget.by_rule;
    target_value: u64 = 0;
    start_ms: u64 = 0;
    end_ms: u64 = 0;
}

/// Inline parent->child inhibition edge: when `source_rule_id` is `firing`, `target_rule_id` is inhibited.
pub struct AlertInhibition {
    active: bool = false;
    source_rule_id: u32 = 0;
    target_rule_id: u32 = 0;
}

/// Result returned by `AlertRuleEngine.evaluate`.
pub struct AlertEvalOutcome {
    found: bool = false;
    valid_sample: bool = false;
    condition_met: bool = false;
    prev_state: AlertState = AlertState.inactive;
    new_state: AlertState = AlertState.inactive;
    notify_kind: AlertNotifyKind = AlertNotifyKind.none;
    should_notify: bool = false;
    silenced: bool = false;
    inhibited: bool = false;
    value: f64 = 0.0;
}

/// Snapshot of a currently firing alert emitted by `collectFiringAlerts`.
pub struct FiringAlertSnapshot {
    rule_id: u32 = 0;
    group_hash: u64 = 0;
    severity: AlertSeverity = AlertSeverity.warning;
    last_value: f64 = 0.0;
    firing_since_ms: u64 = 0;
    silenced: bool = false;
    inhibited: bool = false;
    in_keep_firing: bool = false;
    name_buf: [ALERT_NAME_MAX_LEN]u8 = [0; ALERT_NAME_MAX_LEN];
    name_len: usize = 0;

    pub fn nameSlice(): []const u8 {
        return name_buf[0..name_len];
    }
}

/// Cumulative operational telemetry snapshot for `AlertRuleEngine`.
pub struct AlertEngineStats {
    rule_count: usize = 0;
    capacity: usize = ALERT_MAX_RULES;
    inactive_count: usize = 0;
    pending_count: usize = 0;
    firing_count: usize = 0;
    resolved_count: usize = 0;
    silence_count: usize = 0;
    inhibition_count: usize = 0;
    total_evaluations: u64 = 0;
    total_fires: u64 = 0;
    total_resolves: u64 = 0;
    total_suppressed: u64 = 0;
}

// ============================================================================
// 2. Internal Helpers
// ============================================================================

fn isFiniteF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn isValidRuleName(name: []const u8): bool {
    if (name.len == 0 || name.len > ALERT_NAME_MAX_LEN) return false;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        if (name[i] < 0x20 || name[i] == 0x7F) return false;
    }
    return true;
}

fn evaluateCompareOp(value: f64, op: AlertCompareOp, threshold: f64): bool {
    if (op == AlertCompareOp.gt) return value > threshold;
    if (op == AlertCompareOp.ge) return value >= threshold;
    if (op == AlertCompareOp.lt) return value < threshold;
    if (op == AlertCompareOp.le) return value <= threshold;
    return value == threshold;
}

fn severityRank(sev: AlertSeverity): u8 {
    if (sev == AlertSeverity.critical) return 2;
    if (sev == AlertSeverity.warning) return 1;
    return 0;
}

// ============================================================================
// 3. Core Engine (`AlertRuleEngine`)
// ============================================================================

/// Zero-heap Prometheus-style alert rule evaluator with `for` / `keep_firing_for`
/// 4-state hysteresis, silence windows, and parent->child inhibition.
pub struct AlertRuleEngine {
    rules: [ALERT_MAX_RULES]AlertRuleEntry = [AlertRuleEntry{}; ALERT_MAX_RULES];
    silences: [ALERT_MAX_SILENCES]AlertSilence = [AlertSilence{}; ALERT_MAX_SILENCES];
    inhibitions: [ALERT_MAX_INHIBITIONS]AlertInhibition = [AlertInhibition{}; ALERT_MAX_INHIBITIONS];
    total_evaluations: u64 = 0;
    total_fires: u64 = 0;
    total_resolves: u64 = 0;
    total_suppressed: u64 = 0;

    /// Initializes an empty `AlertRuleEngine`.
    pub fn init(): AlertRuleEngine {
        return AlertRuleEngine{};
    }

    fn findRuleSlot(rule_id: u32): usize? {
        if (rule_id == 0) return null;
        let i: usize = 0;
        while (i < ALERT_MAX_RULES) : (i += 1) {
            if (rules[i].active && rules[i].rule_id == rule_id) return i;
        }
        return null;
    }

    /// Registers a new alert rule. Rejects `rule_id == 0`, duplicate ID, invalid name,
    /// non-finite `threshold`, or full 32-rule table. Returns slot index on success.
    pub fn addRule(*_, spec: AlertRuleSpec): usize? {
        if (spec.rule_id == 0 || !isValidRuleName(spec.name) || !isFiniteF64(spec.threshold)) {
            return null;
        }
        if (findRuleSlot(spec.rule_id) != null) return null;

        let free_slot: usize = ALERT_MAX_RULES;
        let i: usize = 0;
        while (i < ALERT_MAX_RULES) : (i += 1) {
            if (!rules[i].active) {
                free_slot = i;
                break;
            }
        }
        if (free_slot >= ALERT_MAX_RULES) return null;

        let entry = AlertRuleEntry{};
        entry.active = true;
        entry.rule_id = spec.rule_id;
        entry.name_len = spec.name.len;
        let k: usize = 0;
        while (k < spec.name.len) : (k += 1) {
            entry.name_buf[k] = spec.name[k];
        }
        entry.op = spec.op;
        entry.threshold = spec.threshold;
        entry.for_duration_ms = spec.for_duration_ms;
        entry.keep_firing_for_ms = spec.keep_firing_for_ms;
        entry.severity = spec.severity;
        entry.group_hash = spec.group_hash;
        entry.state = AlertState.inactive;

        rules[free_slot] = entry;
        return free_slot;
    }

    /// Removes a registered alert rule by `rule_id`.
    pub fn removeRule(*_, rule_id: u32): bool {
        const slot = findRuleSlot(rule_id) ?? return false;
        rules[slot] = AlertRuleEntry{};
        return true;
    }

    /// Registers a silence window `[start_ms, end_ms)` targeting either a specific `rule_id`
    /// (`by_group == false`) or a `group_hash` (`by_group == true`). Returns silence slot index.
    pub fn addSilence(
        *_,
        rule_id_or_group: u64,
        by_group: bool,
        start_ms: u64,
        end_ms: u64,
    ): usize? {
        if (rule_id_or_group == 0 || end_ms <= start_ms) return null;
        let i: usize = 0;
        while (i < ALERT_MAX_SILENCES) : (i += 1) {
            if (!silences[i].active) {
                silences[i] = AlertSilence{
                    active: true,
                    target_kind: if (by_group) AlertSilenceTarget.by_group else AlertSilenceTarget.by_rule,
                    target_value: rule_id_or_group,
                    start_ms: start_ms,
                    end_ms: end_ms,
                };
                return i;
            }
        }
        return null;
    }

    /// Removes a silence window by slot index.
    pub fn removeSilence(*_, slot_idx: usize): bool {
        if (slot_idx >= ALERT_MAX_SILENCES || !silences[slot_idx].active) return false;
        silences[slot_idx] = AlertSilence{};
        return true;
    }

    /// Registers a parent->child inhibition: while `source_rule_id` is `firing`, `target_rule_id`
    /// notifications are suppressed. Rejects zero IDs, self-inhibition, duplicates, and full table.
    pub fn addInhibition(*_, source_rule_id: u32, target_rule_id: u32): usize? {
        if (source_rule_id == 0 || target_rule_id == 0 || source_rule_id == target_rule_id) {
            return null;
        }
        let free_slot: usize = ALERT_MAX_INHIBITIONS;
        let i: usize = 0;
        while (i < ALERT_MAX_INHIBITIONS) : (i += 1) {
            if (inhibitions[i].active) {
                if (inhibitions[i].source_rule_id == source_rule_id &&
                    inhibitions[i].target_rule_id == target_rule_id)
                {
                    return null;
                }
            } else if (free_slot == ALERT_MAX_INHIBITIONS) {
                free_slot = i;
            }
        }
        if (free_slot >= ALERT_MAX_INHIBITIONS) return null;

        inhibitions[free_slot] = AlertInhibition{
            active: true,
            source_rule_id: source_rule_id,
            target_rule_id: target_rule_id,
        };
        return free_slot;
    }

    /// Removes a parent->child inhibition edge.
    pub fn removeInhibition(*_, source_rule_id: u32, target_rule_id: u32): bool {
        let i: usize = 0;
        while (i < ALERT_MAX_INHIBITIONS) : (i += 1) {
            if (inhibitions[i].active &&
                inhibitions[i].source_rule_id == source_rule_id &&
                inhibitions[i].target_rule_id == target_rule_id)
            {
                inhibitions[i] = AlertInhibition{};
                return true;
            }
        }
        return false;
    }

    /// Checks whether `(rule_id, group_hash)` falls inside any active silence window at `now_ms`.
    pub fn isSilenced(rule_id: u32, group_hash: u64, now_ms: u64): bool {
        let i: usize = 0;
        while (i < ALERT_MAX_SILENCES) : (i += 1) {
            const s = silences[i];
            if (!s.active) continue;
            if (now_ms < s.start_ms || now_ms >= s.end_ms) continue;
            if (s.target_kind == AlertSilenceTarget.by_rule && s.target_value == (rule_id as u64)) {
                return true;
            }
            if (s.target_kind == AlertSilenceTarget.by_group && group_hash != 0 && s.target_value == group_hash) {
                return true;
            }
        }
        return false;
    }

    /// Checks whether `target_rule_id` is currently inhibited by any `firing` source rule.
    pub fn isInhibited(target_rule_id: u32): bool {
        let i: usize = 0;
        while (i < ALERT_MAX_INHIBITIONS) : (i += 1) {
            const inh = inhibitions[i];
            if (!inh.active || inh.target_rule_id != target_rule_id) continue;
            if (src_slot := findRuleSlot(inh.source_rule_id)) {
                if (rules[src_slot].state == AlertState.firing) return true;
            }
        }
        return false;
    }

    /// Evaluates a single metric observation `(value, now_ms)` for `rule_id` and advances its
    /// 4-state hysteresis FSM (`inactive | pending | firing | resolved`).
    pub fn evaluate(*_, rule_id: u32, value: f64, now_ms: u64): AlertEvalOutcome {
        let out = AlertEvalOutcome{ value: value };
        const slot = findRuleSlot(rule_id) ?? return out;

        out.found = true;
        out.prev_state = rules[slot].state;
        out.new_state = rules[slot].state;

        if (!isFiniteF64(value)) {
            out.valid_sample = false;
            return out;
        }

        out.valid_sample = true;
        total_evaluations += 1;

        const cond = evaluateCompareOp(value, rules[slot].op, rules[slot].threshold);
        out.condition_met = cond;
        rules[slot].last_value = value;
        rules[slot].last_eval_ms = now_ms;

        const cur = rules[slot].state;
        let next = cur;
        let notify = AlertNotifyKind.none;

        if (cur == AlertState.inactive || cur == AlertState.resolved) {
            if (cond) {
                rules[slot].active_since_ms = now_ms;
                rules[slot].in_keep_firing = false;
                if (rules[slot].for_duration_ms == 0) {
                    next = AlertState.firing;
                    rules[slot].firing_since_ms = now_ms;
                    notify = AlertNotifyKind.fire;
                } else {
                    next = AlertState.pending;
                }
            } else {
                // One-shot `resolved` returns quietly to `inactive` on subsequent healthy samples.
                next = AlertState.inactive;
                rules[slot].in_keep_firing = false;
            }
        } else if (cur == AlertState.pending) {
            if (cond) {
                const elapsed: u64 = if (now_ms >= rules[slot].active_since_ms)
                    (now_ms - rules[slot].active_since_ms)
                else
                    0;
                if (elapsed >= rules[slot].for_duration_ms) {
                    next = AlertState.firing;
                    rules[slot].firing_since_ms = now_ms;
                    rules[slot].in_keep_firing = false;
                    notify = AlertNotifyKind.fire;
                }
            } else {
                // Transient spike cleared before `for_duration_ms` elapsed -> debounce back to inactive.
                next = AlertState.inactive;
                rules[slot].in_keep_firing = false;
            }
        } else {
            // cur == AlertState.firing
            if (cond) {
                next = AlertState.firing;
                rules[slot].in_keep_firing = false;
            } else if (rules[slot].keep_firing_for_ms == 0) {
                next = AlertState.resolved;
                rules[slot].in_keep_firing = false;
                notify = AlertNotifyKind.resolve;
            } else {
                if (!rules[slot].in_keep_firing) {
                    rules[slot].in_keep_firing = true;
                    rules[slot].keep_firing_since_ms = now_ms;
                    next = AlertState.firing;
                } else {
                    const hold_elapsed: u64 = if (now_ms >= rules[slot].keep_firing_since_ms)
                        (now_ms - rules[slot].keep_firing_since_ms)
                    else
                        0;
                    if (hold_elapsed >= rules[slot].keep_firing_for_ms) {
                        next = AlertState.resolved;
                        rules[slot].in_keep_firing = false;
                        notify = AlertNotifyKind.resolve;
                    } else {
                        next = AlertState.firing;
                    }
                }
            }
        }

        rules[slot].state = next;
        out.new_state = next;
        out.notify_kind = notify;

        const silenced = isSilenced(rule_id, rules[slot].group_hash, now_ms);
        const inhibited = isInhibited(rule_id);
        out.silenced = silenced;
        out.inhibited = inhibited;

        if (notify == AlertNotifyKind.fire) {
            total_fires += 1;
            if (silenced || inhibited) {
                out.should_notify = false;
                total_suppressed += 1;
            } else {
                out.should_notify = true;
            }
        } else if (notify == AlertNotifyKind.resolve) {
            total_resolves += 1;
            if (silenced || inhibited) {
                out.should_notify = false;
                total_suppressed += 1;
            } else {
                out.should_notify = true;
            }
        } else {
            out.should_notify = false;
        }

        return out;
    }

    /// Returns a read-only copy of `rule_id`, or `null` if not registered.
    pub fn getRule(rule_id: u32): AlertRuleEntry? {
        const slot = findRuleSlot(rule_id) ?? return null;
        return rules[slot];
    }

    /// Collects all currently `firing` alerts sorted by descending severity, then ascending `rule_id`.
    /// When `include_suppressed == false`, alerts that are silenced or inhibited are excluded.
    pub fn collectFiringAlerts(
        now_ms: u64,
        include_suppressed: bool,
        out_alerts: *[]FiringAlertSnapshot,
    ): usize {
        if (out_alerts.len == 0) return 0;

        let tmp: [ALERT_MAX_RULES]FiringAlertSnapshot = [FiringAlertSnapshot{}; ALERT_MAX_RULES];
        let count: usize = 0;

        let i: usize = 0;
        while (i < ALERT_MAX_RULES) : (i += 1) {
            if (!rules[i].active || rules[i].state != AlertState.firing) continue;
            const r = rules[i];
            const sil = isSilenced(r.rule_id, r.group_hash, now_ms);
            const inh = isInhibited(r.rule_id);
            if (!include_suppressed && (sil || inh)) continue;

            let snap = FiringAlertSnapshot{
                rule_id: r.rule_id,
                group_hash: r.group_hash,
                severity: r.severity,
                last_value: r.last_value,
                firing_since_ms: r.firing_since_ms,
                silenced: sil,
                inhibited: inh,
                in_keep_firing: r.in_keep_firing,
                name_len: r.name_len,
            };
            let k: usize = 0;
            while (k < r.name_len) : (k += 1) {
                snap.name_buf[k] = r.name_buf[k];
            }
            tmp[count] = snap;
            count += 1;
        }

        // Insertion sort by (severity descending, rule_id ascending)
        let j: usize = 1;
        while (j < count) : (j += 1) {
            const key = tmp[j];
            let k: usize = j;
            while (k > 0) {
                const prev = tmp[k - 1];
                const swap_needed = (severityRank(key.severity) > severityRank(prev.severity)) ||
                    (severityRank(key.severity) == severityRank(prev.severity) && key.rule_id < prev.rule_id);
                if (!swap_needed) break;
                tmp[k] = prev;
                k -= 1;
            }
            tmp[k] = key;
        }

        const n_out: usize = if (count < out_alerts.len) count else out_alerts.len;
        let w: usize = 0;
        while (w < n_out) : (w += 1) {
            out_alerts[w] = tmp[w];
        }
        return n_out;
    }

    /// Returns a point-in-time operational telemetry snapshot.
    pub fn stats(): AlertEngineStats {
        let st = AlertEngineStats{
            capacity: ALERT_MAX_RULES,
            total_evaluations: total_evaluations,
            total_fires: total_fires,
            total_resolves: total_resolves,
            total_suppressed: total_suppressed,
        };

        let i: usize = 0;
        while (i < ALERT_MAX_RULES) : (i += 1) {
            if (!rules[i].active) continue;
            st.rule_count += 1;
            if (rules[i].state == AlertState.inactive) {
                st.inactive_count += 1;
            } else if (rules[i].state == AlertState.pending) {
                st.pending_count += 1;
            } else if (rules[i].state == AlertState.firing) {
                st.firing_count += 1;
            } else {
                st.resolved_count += 1;
            }
        }

        let s: usize = 0;
        while (s < ALERT_MAX_SILENCES) : (s += 1) {
            if (silences[s].active) st.silence_count += 1;
        }

        let h: usize = 0;
        while (h < ALERT_MAX_INHIBITIONS) : (h += 1) {
            if (inhibitions[h].active) st.inhibition_count += 1;
        }

        return st;
    }
}
