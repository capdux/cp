// Origin: Pure-CP Stdlib (Zero-Alloc MCP Resource Subscription Registry, RFC 6570 URI Template Matcher & Debounced Fan-Out Notifier) | Created: 2026-11-22
// Pure CP zero-heap-allocation MCP resource subscription and incremental change notification engine:
//   - RFC 6570 simplified URI template validator & zero-copy parameter extractor (`{var}` & `{+path}`)
//   - Per-session exact URI & URI template subscription registry with deduplication and quota guards
//   - Session lifecycle bulk cleanup (`unsubscribeAllForSession`) clearing subscriptions and pending notifications
//   - Resource update fan-out (`notifyResourceUpdated`) with monotonic `etag_version` and subscriber deduplication
//   - Debounced/coalesced dirty-notification queue (`drainPendingNotifications`) supporting batched draining

/// Maximum concurrent subscriptions across all sessions (zero dynamic heap).
pub const MCP_RSUB_MAX_SUBSCRIPTIONS: usize = 128;

/// Maximum byte length of a resource URI or URI template.
pub const MCP_RSUB_MAX_URI_LEN: usize = 192;

/// Maximum template variables captured in a single RFC 6570 match.
pub const MCP_RSUB_MAX_TEMPLATE_PARAMS: usize = 8;

/// Maximum pending (coalesced) dirty notifications retained before draining.
pub const MCP_RSUB_MAX_PENDING_NOTIFS: usize = 128;

/// Default maximum subscriptions allowed per individual session.
pub const MCP_RSUB_DEFAULT_MAX_PER_SESSION: usize = 32;

const FNV1A_OFFSET: u64 = 14695981039346656037;
const FNV1A_PRIME: u64 = 1099511628211;

fn fnv1aHashBytes(bytes: []const u8): u64 {
    let h: u64 = FNV1A_OFFSET;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h = (h ^ (bytes[i] as u64)) *% FNV1A_PRIME;
    }
    return h;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isValidVarChar(ch: u8): bool {
    const is_lower: bool = ch >= 97 && ch <= 122; // 'a'..'z'
    const is_upper: bool = ch >= 65 && ch <= 90;  // 'A'..'Z'
    const is_digit: bool = ch >= 48 && ch <= 57;  // '0'..'9'
    return is_lower || is_upper || is_digit || ch == 95 || ch == 46 || ch == 45; // '_', '.', '-'
}

fn isValidUriByte(ch: u8): bool {
    // Reject ASCII control chars (<= 0x20 including space) and DEL (0x7F).
    return ch > 32 && ch != 127;
}

fn hasBrace(bytes: []const u8): bool {
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] == 123 || bytes[i] == 125) return true; // '{' or '}'
    }
    return false;
}

// ============================================================================
// 1. RFC 6570 Simplified URI Template Validator & Zero-Copy Matcher
// ============================================================================

/// Zero-copy byte offset span for a single matched `{var}` or `{+var}` template parameter.
pub struct McpTemplateParam {
    name_start: u16 = 0;
    name_len: u16 = 0;
    val_start: u16 = 0;
    val_len: u16 = 0;
}

/// Result of matching a concrete resource URI against an RFC 6570 simplified URI template.
pub struct McpTemplateMatchResult {
    matched: bool = false;
    param_count: usize = 0;
    params: [MCP_RSUB_MAX_TEMPLATE_PARAMS]McpTemplateParam = [McpTemplateParam{}; MCP_RSUB_MAX_TEMPLATE_PARAMS];
}

/// Validates a concrete resource URI or simplified RFC 6570 URI template (`{var}` / `{+var}`).
/// Rejects empty strings, oversized URIs, control/whitespace bytes, unmatched/nested braces,
/// empty `{}` placeholders, adjacent `}{` placeholders, and invalid variable names.
pub fn validateRfc6570Template(template: []const u8): bool {
    if (template.len == 0 || template.len > MCP_RSUB_MAX_URI_LEN) return false;

    let i: usize = 0;
    let param_cnt: usize = 0;
    let prev_was_expr: bool = false;

    while (i < template.len) {
        const ch: u8 = template[i];
        if (!isValidUriByte(ch)) return false;

        if (ch == 125) { // stray '}'
            return false;
        }
        if (ch == 123) { // '{'
            if (prev_was_expr) return false; // adjacent '}{' is ambiguous
            if (param_cnt >= MCP_RSUB_MAX_TEMPLATE_PARAMS) return false;
            i += 1;
            if (i >= template.len) return false;

            // Optional '+' reserved expansion operator (allows matching '/')
            if (template[i] == 43) { // '+'
                i += 1;
                if (i >= template.len) return false;
            }

            const name_start: usize = i;
            while (i < template.len && template[i] != 125) : (i += 1) {
                const vc: u8 = template[i];
                if (vc == 123 || !isValidVarChar(vc)) return false;
            }
            if (i >= template.len || template[i] != 125) return false;
            if (i == name_start) return false; // empty '{}' or '{+}'

            param_cnt += 1;
            prev_was_expr = true;
            i += 1; // skip '}'
        } else {
            prev_was_expr = false;
            i += 1;
        }
    }
    return true;
}

/// Matches concrete `uri` against an RFC 6570 simplified `template` (supporting `{var}` single-segment
/// and `{+var}` multi-segment path expressions) and extracts zero-copy parameter spans.
pub fn matchRfc6570Template(template: []const u8, uri: []const u8): McpTemplateMatchResult {
    let res = McpTemplateMatchResult{};
    if (uri.len == 0 || uri.len > MCP_RSUB_MAX_URI_LEN) return res;
    if (!validateRfc6570Template(template)) return res;

    // Fast path: static exact URI
    if (!hasBrace(template)) {
        res.matched = bytesEqual(template, uri);
        return res;
    }

    let ti: usize = 0;
    let ui: usize = 0;

    while (ti < template.len) {
        if (template[ti] != 123) { // literal character
            if (ui >= uri.len || uri[ui] != template[ti]) return McpTemplateMatchResult{};
            ti += 1;
            ui += 1;
            continue;
        }

        // Parse '{' ['+'] name '}'
        ti += 1;
        let allow_slash: bool = false;
        if (ti < template.len && template[ti] == 43) { // '+'
            allow_slash = true;
            ti += 1;
        }
        const name_start: usize = ti;
        while (ti < template.len && template[ti] != 125) : (ti += 1) {}
        const name_len: usize = ti - name_start;
        ti += 1; // skip '}'

        // Determine stopping literal delimiter after '}' (if any)
        const has_next_lit: bool = ti < template.len;
        const stop_ch: u8 = if (has_next_lit) template[ti] else 0;

        const val_start: usize = ui;
        if (!has_next_lit) {
            // Trailing placeholder: consumes to end of URI (for {var}, must not contain '/')
            while (ui < uri.len) : (ui += 1) {
                if (!allow_slash && uri[ui] == 47) return McpTemplateMatchResult{};
            }
        } else {
            // Scan until next literal delimiter character (respecting '/' for {var})
            while (ui < uri.len && uri[ui] != stop_ch) : (ui += 1) {
                if (!allow_slash && uri[ui] == 47) return McpTemplateMatchResult{};
            }
        }

        const val_len: usize = ui - val_start;
        if (val_len == 0) return McpTemplateMatchResult{}; // RFC 6570 capture must be non-empty

        if (res.param_count >= MCP_RSUB_MAX_TEMPLATE_PARAMS) return McpTemplateMatchResult{};
        res.params[res.param_count] = McpTemplateParam{
            name_start: name_start as u16,
            name_len: name_len as u16,
            val_start: val_start as u16,
            val_len: val_len as u16,
        };
        res.param_count += 1;
    }

    if (ui != uri.len) return McpTemplateMatchResult{};
    res.matched = true;
    return res;
}

// ============================================================================
// 2. Subscription Registry & Debounced Fan-Out Notification Models
// ============================================================================

/// Kind of registered resource subscription pattern.
pub enum McpSubKind {
    exact_uri,
    uri_template,
}

/// Deterministic outcome status for `subscribe` / `unsubscribe` operations.
pub enum McpSubStatus {
    ok,
    already_subscribed,
    invalid_session,
    invalid_uri,
    capacity_full,
    session_limit_reached,
    not_found,
}

/// Stored subscription entry owned inline by `McpResourceSubHub`.
pub struct McpSubEntry {
    subscriber_id: u64 = 0;
    kind: McpSubKind = McpSubKind.exact_uri;
    pattern_buf: [MCP_RSUB_MAX_URI_LEN]u8 = [0; MCP_RSUB_MAX_URI_LEN];
    pattern_len: u16 = 0;
    pattern_hash: u64 = 0;
    last_etag_version: u64 = 0;
    active: bool = false;
}

/// Pending coalesced resource update notification waiting to be drained to a subscriber.
pub struct McpPendingNotification {
    subscriber_id: u64 = 0;
    uri_buf: [MCP_RSUB_MAX_URI_LEN]u8 = [0; MCP_RSUB_MAX_URI_LEN];
    uri_len: u16 = 0;
    uri_hash: u64 = 0;
    etag_version: u64 = 0;
    coalesced_count: u32 = 1;
    occupied: bool = false;

    /// Returns the concrete updated URI slice stored in this notification.
    fn uriSlice(): []const u8 {
        return _.uri_buf[0 .. _.uri_len as usize];
    }
}

/// Fan-out summary returned by `McpResourceSubHub.notifyResourceUpdated`.
pub struct McpResourceNotifyReport {
    etag_version: u64 = 0;
    matched_subscribers: usize = 0;
    enqueued_notifications: usize = 0;
    coalesced_notifications: usize = 0;
    dropped_notifications: usize = 0;
}

/// Aggregate operational telemetry for `McpResourceSubHub`.
pub struct McpResourceSubStats {
    active_subscriptions: usize = 0;
    pending_notifications: usize = 0;
    current_etag_version: u64 = 0;
    total_subscribes: u64 = 0;
    total_unsubscribes: u64 = 0;
    total_updates_published: u64 = 0;
    total_notifications_drained: u64 = 0;
    total_coalesced: u64 = 0;
}

// ============================================================================
// 3. Zero-Alloc MCP Resource Subscription Hub
// ============================================================================

/// Zero-heap-allocation MCP resource subscription registry, RFC 6570 template matcher,
/// and debounced incremental change fan-out notifier.
pub struct McpResourceSubHub {
    max_per_session: usize = MCP_RSUB_DEFAULT_MAX_PER_SESSION;
    subs: [MCP_RSUB_MAX_SUBSCRIPTIONS]McpSubEntry = [McpSubEntry{}; MCP_RSUB_MAX_SUBSCRIPTIONS];
    active_sub_count: usize = 0;
    pending: [MCP_RSUB_MAX_PENDING_NOTIFS]McpPendingNotification = [McpPendingNotification{}; MCP_RSUB_MAX_PENDING_NOTIFS];
    pending_count: usize = 0;
    current_etag_version: u64 = 0;
    total_subscribes: u64 = 0;
    total_unsubscribes: u64 = 0;
    total_updates_published: u64 = 0;
    total_notifications_drained: u64 = 0;
    total_coalesced: u64 = 0;

    /// Initializes a `McpResourceSubHub` with per-session subscription quota `max_per_session`.
    /// Requires `1 <= max_per_session <= MCP_RSUB_MAX_SUBSCRIPTIONS`.
    static fn init(max_per_session: usize): McpResourceSubHub? {
        if (max_per_session == 0 || max_per_session > MCP_RSUB_MAX_SUBSCRIPTIONS) return null;
        return McpResourceSubHub{
            max_per_session: max_per_session,
            subs: [McpSubEntry{}; MCP_RSUB_MAX_SUBSCRIPTIONS],
            active_sub_count: 0,
            pending: [McpPendingNotification{}; MCP_RSUB_MAX_PENDING_NOTIFS],
            pending_count: 0,
            current_etag_version: 0,
            total_subscribes: 0,
            total_unsubscribes: 0,
            total_updates_published: 0,
            total_notifications_drained: 0,
            total_coalesced: 0,
        };
    }

    /// Returns the number of active subscriptions held by `subscriber_id`.
    fn sessionSubscriptionCount(subscriber_id: u64): usize {
        if (subscriber_id == 0) return 0;
        let cnt: usize = 0;
        let i: usize = 0;
        while (i < MCP_RSUB_MAX_SUBSCRIPTIONS) : (i += 1) {
            if (_.subs[i].active && _.subs[i].subscriber_id == subscriber_id) {
                cnt += 1;
            }
        }
        return cnt;
    }

    /// Registers a subscription for `subscriber_id` on an exact resource URI or RFC 6570 URI template.
    /// Automatically deduplicates identical `(subscriber_id, pattern)` pairs and enforces quotas.
    fn subscribe(*_, subscriber_id: u64, uri_or_template: []const u8): McpSubStatus {
        if (subscriber_id == 0) return McpSubStatus.invalid_session;
        if (!validateRfc6570Template(uri_or_template)) return McpSubStatus.invalid_uri;

        const pat_hash: u64 = fnv1aHashBytes(uri_or_template);
        let session_cnt: usize = 0;
        let free_slot: usize = MCP_RSUB_MAX_SUBSCRIPTIONS;

        let i: usize = 0;
        while (i < MCP_RSUB_MAX_SUBSCRIPTIONS) : (i += 1) {
            if (_.subs[i].active) {
                if (_.subs[i].subscriber_id == subscriber_id) {
                    session_cnt += 1;
                    if (_.subs[i].pattern_hash == pat_hash &&
                        bytesEqual(_.subs[i].pattern_buf[0 .. _.subs[i].pattern_len as usize], uri_or_template)) {
                        return McpSubStatus.already_subscribed;
                    }
                }
            } else if (free_slot == MCP_RSUB_MAX_SUBSCRIPTIONS) {
                free_slot = i;
            }
        }

        if (session_cnt >= _.max_per_session) return McpSubStatus.session_limit_reached;
        if (free_slot >= MCP_RSUB_MAX_SUBSCRIPTIONS) return McpSubStatus.capacity_full;

        let entry = McpSubEntry{
            subscriber_id: subscriber_id,
            kind: if (hasBrace(uri_or_template)) McpSubKind.uri_template else McpSubKind.exact_uri,
            pattern_buf: [0; MCP_RSUB_MAX_URI_LEN],
            pattern_len: uri_or_template.len as u16,
            pattern_hash: pat_hash,
            last_etag_version: _.current_etag_version,
            active: true,
        };
        let k: usize = 0;
        while (k < uri_or_template.len) : (k += 1) {
            entry.pattern_buf[k] = uri_or_template[k];
        }
        _.subs[free_slot] = entry;
        _.active_sub_count += 1;
        _.total_subscribes += 1;
        return McpSubStatus.ok;
    }

    /// Removes a specific `(subscriber_id, uri_or_template)` subscription.
    fn unsubscribe(*_, subscriber_id: u64, uri_or_template: []const u8): McpSubStatus {
        if (subscriber_id == 0) return McpSubStatus.invalid_session;
        if (uri_or_template.len == 0 || uri_or_template.len > MCP_RSUB_MAX_URI_LEN) {
            return McpSubStatus.invalid_uri;
        }

        const pat_hash: u64 = fnv1aHashBytes(uri_or_template);
        let i: usize = 0;
        while (i < MCP_RSUB_MAX_SUBSCRIPTIONS) : (i += 1) {
            if (_.subs[i].active && _.subs[i].subscriber_id == subscriber_id &&
                _.subs[i].pattern_hash == pat_hash &&
                bytesEqual(_.subs[i].pattern_buf[0 .. _.subs[i].pattern_len as usize], uri_or_template)) {
                _.subs[i] = McpSubEntry{};
                if (_.active_sub_count > 0) {
                    _.active_sub_count -= 1;
                }
                _.total_unsubscribes += 1;
                return McpSubStatus.ok;
            }
        }
        return McpSubStatus.not_found;
    }

    /// Removes all subscriptions and pending notifications belonging to `subscriber_id`.
    /// Returns the number of subscriptions removed.
    fn unsubscribeAllForSession(*_, subscriber_id: u64): usize {
        if (subscriber_id == 0) return 0;

        let removed: usize = 0;
        let i: usize = 0;
        while (i < MCP_RSUB_MAX_SUBSCRIPTIONS) : (i += 1) {
            if (_.subs[i].active && _.subs[i].subscriber_id == subscriber_id) {
                _.subs[i] = McpSubEntry{};
                removed += 1;
            }
        }
        if (removed <= _.active_sub_count) {
            _.active_sub_count -= removed;
        } else {
            _.active_sub_count = 0;
        }
        _.total_unsubscribes += removed as u64;

        let p: usize = 0;
        while (p < MCP_RSUB_MAX_PENDING_NOTIFS) : (p += 1) {
            if (_.pending[p].occupied && _.pending[p].subscriber_id == subscriber_id) {
                _.pending[p] = McpPendingNotification{};
                if (_.pending_count > 0) {
                    _.pending_count -= 1;
                }
            }
        }
        return removed;
    }

    fn enqueueOrCoalesceNotification(
        *_,
        subscriber_id: u64,
        resource_uri: []const u8,
        uri_hash: u64,
        etag_version: u64,
        report: *McpResourceNotifyReport,
    ) {
        let free_slot: usize = MCP_RSUB_MAX_PENDING_NOTIFS;
        let i: usize = 0;
        while (i < MCP_RSUB_MAX_PENDING_NOTIFS) : (i += 1) {
            if (_.pending[i].occupied) {
                if (_.pending[i].subscriber_id == subscriber_id &&
                    _.pending[i].uri_hash == uri_hash &&
                    bytesEqual(_.pending[i].uri_buf[0 .. _.pending[i].uri_len as usize], resource_uri)) {
                    // Debounce / coalesce duplicate dirty notification for same (subscriber, uri)
                    _.pending[i].etag_version = etag_version;
                    _.pending[i].coalesced_count += 1;
                    report.coalesced_notifications += 1;
                    _.total_coalesced += 1;
                    return;
                }
            } else if (free_slot == MCP_RSUB_MAX_PENDING_NOTIFS) {
                free_slot = i;
            }
        }

        if (free_slot >= MCP_RSUB_MAX_PENDING_NOTIFS) {
            report.dropped_notifications += 1;
            return;
        }

        let notif = McpPendingNotification{
            subscriber_id: subscriber_id,
            uri_buf: [0; MCP_RSUB_MAX_URI_LEN],
            uri_len: resource_uri.len as u16,
            uri_hash: uri_hash,
            etag_version: etag_version,
            coalesced_count: 1,
            occupied: true,
        };
        let k: usize = 0;
        while (k < resource_uri.len) : (k += 1) {
            notif.uri_buf[k] = resource_uri[k];
        }
        _.pending[free_slot] = notif;
        _.pending_count += 1;
        report.enqueued_notifications += 1;
    }

    /// Publishes an update for concrete `resource_uri`, incrementing `etag_version`, matching all
    /// exact and RFC 6570 template subscriptions, deduplicating matched `subscriber_id`s into
    /// `out_subscriber_ids`, and enqueueing/coalescing pending notifications.
    fn notifyResourceUpdated(
        *_,
        resource_uri: []const u8,
        out_subscriber_ids: *[]u64,
    ): McpResourceNotifyReport {
        let report = McpResourceNotifyReport{};
        if (resource_uri.len == 0 || resource_uri.len > MCP_RSUB_MAX_URI_LEN) return report;
        if (hasBrace(resource_uri)) return report;
        let chk: usize = 0;
        while (chk < resource_uri.len) : (chk += 1) {
            if (!isValidUriByte(resource_uri[chk])) return report;
        }

        _.current_etag_version += 1;
        _.total_updates_published += 1;
        const ver: u64 = _.current_etag_version;
        report.etag_version = ver;

        const uri_hash: u64 = fnv1aHashBytes(resource_uri);

        // Deduplicated list of affected subscribers for this update event.
        let matched_ids: [MCP_RSUB_MAX_SUBSCRIPTIONS]u64 = [0; MCP_RSUB_MAX_SUBSCRIPTIONS];
        let matched_cnt: usize = 0;

        let i: usize = 0;
        while (i < MCP_RSUB_MAX_SUBSCRIPTIONS) : (i += 1) {
            if (!_.subs[i].active) continue;
            const pat: []const u8 = _.subs[i].pattern_buf[0 .. _.subs[i].pattern_len as usize];
            let is_match: bool = false;
            if (_.subs[i].kind == McpSubKind.exact_uri) {
                is_match = _.subs[i].pattern_hash == uri_hash && bytesEqual(pat, resource_uri);
            } else {
                const m = matchRfc6570Template(pat, resource_uri);
                is_match = m.matched;
            }
            if (!is_match) continue;

            _.subs[i].last_etag_version = ver;
            const sid: u64 = _.subs[i].subscriber_id;

            let already_listed: bool = false;
            let s: usize = 0;
            while (s < matched_cnt) : (s += 1) {
                if (matched_ids[s] == sid) {
                    already_listed = true;
                    break;
                }
            }
            if (!already_listed && matched_cnt < MCP_RSUB_MAX_SUBSCRIPTIONS) {
                matched_ids[matched_cnt] = sid;
                matched_cnt += 1;
            }
        }

        report.matched_subscribers = matched_cnt;
        let m_idx: usize = 0;
        while (m_idx < matched_cnt) : (m_idx += 1) {
            const sid: u64 = matched_ids[m_idx];
            if (m_idx < out_subscriber_ids.len) {
                out_subscriber_ids[m_idx] = sid;
            }
            _.enqueueOrCoalesceNotification(sid, resource_uri, uri_hash, ver, *report);
        }
        return report;
    }

    /// Drains pending coalesced notifications into `out_notifs`.
    /// Pass `subscriber_id = 0` to drain across all sessions, or a specific session ID to drain only its queue.
    /// Supports batched draining: only notifications copied into `out_notifs` are removed.
    fn drainPendingNotifications(
        *_,
        subscriber_id: u64,
        out_notifs: *[]McpPendingNotification,
    ): usize {
        if (out_notifs.len == 0 || _.pending_count == 0) return 0;

        let drained: usize = 0;
        let i: usize = 0;
        while (i < MCP_RSUB_MAX_PENDING_NOTIFS && drained < out_notifs.len) : (i += 1) {
            if (!_.pending[i].occupied) continue;
            if (subscriber_id != 0 && _.pending[i].subscriber_id != subscriber_id) continue;

            out_notifs[drained] = _.pending[i];
            _.pending[i] = McpPendingNotification{};
            if (_.pending_count > 0) {
                _.pending_count -= 1;
            }
            drained += 1;
        }
        _.total_notifications_drained += drained as u64;
        return drained;
    }

    /// Returns aggregate subscription, queue, ETag, and coalescing statistics.
    fn stats(): McpResourceSubStats {
        return McpResourceSubStats{
            active_subscriptions: _.active_sub_count,
            pending_notifications: _.pending_count,
            current_etag_version: _.current_etag_version,
            total_subscribes: _.total_subscribes,
            total_unsubscribes: _.total_unsubscribes,
            total_updates_published: _.total_updates_published,
            total_notifications_drained: _.total_notifications_drained,
            total_coalesced: _.total_coalesced,
        };
    }
}
