// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc SSE Fanout Broker & Last-Event-ID Ring Buffer) | Created: 2026-11-22
// Pure CP zero-heap-allocation WHATWG/RFC 8895 Server-Sent Events (SSE) multi-client fanout broker
// with bounded history ring buffer, topic filtering, Last-Event-ID resumption, slow-consumer lag resync,
// and injection-safe multi-line wire formatting:
//   - Fixed 64-event history ring (`SSE_BROKER_MAX_HISTORY`) & 32 client cursors (`SSE_BROKER_MAX_CLIENTS`)
//   - Header injection guard rejecting `\r`, `\n`, `\0` in `event_type`
//   - Exact `Last-Event-ID` catch-up & automatic `lagged_resynced` reporting on ring wrap-around
//   - Spec-compliant WHATWG `formatEventWire` splitting `\n` / `\r\n` payloads into `data:` lines

// ============================================================================
// 1. Constants & Low-Level Buffer Writers
// ============================================================================

/// Fixed history ring buffer capacity in events (zero dynamic heap allocations).
pub const SSE_BROKER_MAX_HISTORY: usize = 64;
/// Maximum concurrent subscribed SSE clients.
pub const SSE_BROKER_MAX_CLIENTS: usize = 32;
/// Maximum inline byte length for an SSE `event:` type name.
pub const SSE_EVENT_TYPE_MAX_LEN: usize = 32;
/// Maximum inline byte length for an SSE `data:` payload.
pub const SSE_DATA_MAX_LEN: usize = 256;

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn appendByte(out: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn appendU64Dec(out: *[]u8, pos: *usize, val: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let cur: u64 = val;
    if (cur == 0) {
        tmp[0] = 48; // '0'
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = (48 + (cur % 10)) as u8;
            cur /= 10;
            n += 1;
        }
    }
    if (pos.* > out.len || n > out.len - pos.*) {
        return false;
    }
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

// ============================================================================
// 2. Public Models: Stored Event, Client Cursor, Poll Result & Broker Stats
// ============================================================================

/// Inline stored SSE event inside the broker's circular history buffer.
pub struct SseStoredEvent {
    event_id: u64 = 0;
    topic_hash: u64 = 0;
    event_type_buf: [SSE_EVENT_TYPE_MAX_LEN]u8 = [0; SSE_EVENT_TYPE_MAX_LEN];
    event_type_len: usize = 0;
    data_buf: [SSE_DATA_MAX_LEN]u8 = [0; SSE_DATA_MAX_LEN];
    data_len: usize = 0;
    retry_ms: u32 = 0;
}

/// Per-client subscription state and delivery cursor.
pub struct SseClientCursor {
    client_id: u64 = 0;
    topic_filter_hash: u64 = 0;
    last_acked_id: u64 = 0;
    active: bool = false;
    lag_dropped_count: u64 = 0;
}

/// Delivery outcome status returned by `SseFanoutBroker.poll_next`.
pub enum SsePollStatus {
    has_event,
    empty,
    lagged_resynced,
    client_not_found,
}

/// Result of polling the next unread SSE event for a subscribed client.
pub struct SsePollResult {
    status: SsePollStatus = SsePollStatus.empty;
    event_id: u64 = 0;
    skipped_events: u64 = 0;
    event_idx: usize = 0;
}

/// Operational snapshot of `SseFanoutBroker`.
pub struct SseBrokerStats {
    active_clients: usize = 0;
    total_published: u64 = 0;
    ring_count: usize = 0;
    oldest_event_id: u64 = 0;
    latest_event_id: u64 = 0;
    total_lag_drops: u64 = 0;
}

// ============================================================================
// 3. WHATWG SSE Wire Formatter (Standalone + Static Method)
// ============================================================================

fn formatEventWireImpl(ev: SseStoredEvent, out_buf: *[]u8): usize? {
    let pos: usize = 0;

    // 1. id: <event_id>\n
    if (!appendBytes(out_buf, &pos, "id: ")) return null;
    if (!appendU64Dec(out_buf, &pos, ev.event_id)) return null;
    if (!appendByte(out_buf, &pos, 10)) return null;

    // 2. optional event: <event_type>\n
    if (ev.event_type_len > 0) {
        if (ev.event_type_len > SSE_EVENT_TYPE_MAX_LEN) return null;
        if (!appendBytes(out_buf, &pos, "event: ")) return null;
        if (!appendBytes(out_buf, &pos, ev.event_type_buf[0..ev.event_type_len])) return null;
        if (!appendByte(out_buf, &pos, 10)) return null;
    }

    // 3. optional retry: <retry_ms>\n
    if (ev.retry_ms > 0) {
        if (!appendBytes(out_buf, &pos, "retry: ")) return null;
        if (!appendU64Dec(out_buf, &pos, ev.retry_ms as u64)) return null;
        if (!appendByte(out_buf, &pos, 10)) return null;
    }

    // 4. data: lines split on \r\n, \n, or \r
    if (ev.data_len > SSE_DATA_MAX_LEN) return null;
    if (ev.data_len == 0) {
        if (!appendBytes(out_buf, &pos, "data: \n")) return null;
    } else {
        const data = ev.data_buf[0..ev.data_len];
        let line_start: usize = 0;
        let i: usize = 0;
        while (i < data.len) {
            if (data[i] == 13 || data[i] == 10) {
                if (!appendBytes(out_buf, &pos, "data: ")) return null;
                if (!appendBytes(out_buf, &pos, data[line_start..i])) return null;
                if (!appendByte(out_buf, &pos, 10)) return null;
                if (data[i] == 13 && i + 1 < data.len && data[i + 1] == 10) {
                    i += 2;
                } else {
                    i += 1;
                }
                line_start = i;
            } else {
                i += 1;
            }
        }
        // Emit trailing segment (including empty trailing line if data ended with newline)
        if (!appendBytes(out_buf, &pos, "data: ")) return null;
        if (!appendBytes(out_buf, &pos, data[line_start..data.len])) return null;
        if (!appendByte(out_buf, &pos, 10)) return null;
    }

    // 5. Blank line terminating the SSE event frame
    if (!appendByte(out_buf, &pos, 10)) return null;
    return pos;
}

/// Formats `ev` into WHATWG SSE wire format (`id:`, optional `event:`/`retry:`, multi-line `data:`, `\n\n`).
/// Returns total bytes written or `null` if `out_buf` is too small.
pub fn formatSseEventWire(ev: SseStoredEvent, out_buf: *[]u8): usize? {
    return formatEventWireImpl(ev, out_buf);
}

// ============================================================================
// 4. Zero-Alloc SSE Fanout Broker
// ============================================================================

/// Zero-heap-allocation SSE fanout broker with 64-event circular history and 32 client cursors.
pub struct SseFanoutBroker {
    ring: [SSE_BROKER_MAX_HISTORY]SseStoredEvent = [SseStoredEvent{}; SSE_BROKER_MAX_HISTORY];
    clients: [SSE_BROKER_MAX_CLIENTS]SseClientCursor = [SseClientCursor{}; SSE_BROKER_MAX_CLIENTS];
    active_clients: usize = 0;
    next_event_id: u64 = 1;
    ring_count: usize = 0;
    total_published: u64 = 0;
    total_lag_drops: u64 = 0;

    /// Initializes an empty SSE fanout broker.
    static fn init(): SseFanoutBroker {
        return SseFanoutBroker{
            ring: [SseStoredEvent{}; SSE_BROKER_MAX_HISTORY],
            clients: [SseClientCursor{}; SSE_BROKER_MAX_CLIENTS],
            active_clients: 0,
            next_event_id: 1,
            ring_count: 0,
            total_published: 0,
            total_lag_drops: 0,
        };
    }

    /// Formats `ev` into WHATWG SSE wire format (`id:`, optional `event:`/`retry:`, multi-line `data:`, `\n\n`).
    static fn formatEventWire(ev: SseStoredEvent, out_buf: *[]u8): usize? {
        return formatEventWireImpl(ev, out_buf);
    }

    fn latestEventId(): u64 {
        return _.next_event_id - 1;
    }

    fn oldestEventId(): u64 {
        if (_.ring_count == 0) {
            return 0;
        }
        return _.latestEventId() - ((_.ring_count as u64) - 1);
    }

    fn slotForEventId(eid: u64): usize {
        return ((eid - 1) % (SSE_BROKER_MAX_HISTORY as u64)) as usize;
    }

    fn findClientSlot(client_id: u64): usize? {
        if (client_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < SSE_BROKER_MAX_CLIENTS) : (i += 1) {
            if (_.clients[i].active && _.clients[i].client_id == client_id) {
                return i;
            }
        }
        return null;
    }

    /// Publishes an SSE event into the circular history ring and returns its monotonic `event_id`.
    /// Rejects oversized `event_type`/`data` or `event_type` containing CR/LF/NUL header-injection bytes.
    fn publish(
        *_,
        topic_hash: u64,
        event_type: []const u8,
        data: []const u8,
        retry_ms: u32,
    ): u64? {
        if (event_type.len > SSE_EVENT_TYPE_MAX_LEN || data.len > SSE_DATA_MAX_LEN) {
            return null;
        }
        // Guard against CR/LF/NUL header injection in event_type
        let k: usize = 0;
        while (k < event_type.len) : (k += 1) {
            const ch = event_type[k];
            if (ch == 10 || ch == 13 || ch == 0) {
                return null;
            }
        }

        const eid = _.next_event_id;
        _.next_event_id += 1;
        const slot = _.slotForEventId(eid);

        let ev = SseStoredEvent{
            event_id: eid,
            topic_hash: topic_hash,
            event_type_buf: [0; SSE_EVENT_TYPE_MAX_LEN],
            event_type_len: event_type.len,
            data_buf: [0; SSE_DATA_MAX_LEN],
            data_len: data.len,
            retry_ms: retry_ms,
        };
        let i: usize = 0;
        while (i < event_type.len) : (i += 1) {
            ev.event_type_buf[i] = event_type[i];
        }
        let j: usize = 0;
        while (j < data.len) : (j += 1) {
            ev.data_buf[j] = data[j];
        }

        _.ring[slot] = ev;
        if (_.ring_count < SSE_BROKER_MAX_HISTORY) {
            _.ring_count += 1;
        }
        _.total_published += 1;
        return eid;
    }

    /// Subscribes `client_id` with optional `topic_filter_hash` (`0` = receive all topics)
    /// and optional `last_event_id` (`0` = start from current tail; `> 0` = resume after `last_event_id`).
    fn subscribe(
        *_,
        client_id: u64,
        topic_filter_hash: u64,
        last_event_id: u64,
    ): bool {
        if (client_id == 0) {
            return false;
        }
        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < SSE_BROKER_MAX_CLIENTS) : (i += 1) {
            if (_.clients[i].active) {
                if (_.clients[i].client_id == client_id) {
                    return false; // duplicate client_id
                }
            } else if (free_idx == null) {
                free_idx = i;
            }
        }

        const slot = free_idx ?? return false;
        const latest = _.latestEventId();
        const start_acked: u64 = if (last_event_id == 0)
            latest
        else if (last_event_id > latest)
            latest
        else
            last_event_id;

        _.clients[slot] = SseClientCursor{
            client_id: client_id,
            topic_filter_hash: topic_filter_hash,
            last_acked_id: start_acked,
            active: true,
            lag_dropped_count: 0,
        };
        _.active_clients += 1;
        return true;
    }

    /// Unsubscribes `client_id` and reclaims its cursor slot for reuse.
    fn unsubscribe(*_, client_id: u64): bool {
        const slot = _.findClientSlot(client_id) ?? return false;
        _.clients[slot] = SseClientCursor{};
        _.active_clients -= 1;
        return true;
    }

    /// Polls the next unread matching event for `client_id`.
    /// If the client fell behind the oldest retained ring event, advances cursor to `oldest`,
    /// accounts for dropped events, and reports `SsePollStatus.lagged_resynced`.
    fn poll_next(*_, client_id: u64): SsePollResult {
        const cidx = _.findClientSlot(client_id) ?? return SsePollResult{
            status: SsePollStatus.client_not_found,
        };

        const latest = _.latestEventId();
        if (_.ring_count == 0 || _.clients[cidx].last_acked_id >= latest) {
            return SsePollResult{ status: SsePollStatus.empty };
        }

        const oldest = _.oldestEventId();
        const filter = _.clients[cidx].topic_filter_hash;
        let want_id = _.clients[cidx].last_acked_id + 1;
        let lagged = false;
        let skipped: u64 = 0;

        if (want_id < oldest) {
            lagged = true;
            skipped = oldest - want_id;
            _.clients[cidx].lag_dropped_count += skipped;
            _.total_lag_drops += skipped;
            want_id = oldest;
        }

        let cur_id = want_id;
        while (cur_id <= latest) : (cur_id += 1) {
            const slot = _.slotForEventId(cur_id);
            if (filter == 0 || _.ring[slot].topic_hash == filter) {
                _.clients[cidx].last_acked_id = cur_id;
                return SsePollResult{
                    status: if (lagged) SsePollStatus.lagged_resynced else SsePollStatus.has_event,
                    event_id: cur_id,
                    skipped_events: skipped,
                    event_idx: slot,
                };
            }
        }

        // All remaining events in ring belonged to other topics
        _.clients[cidx].last_acked_id = latest;
        if (lagged) {
            return SsePollResult{
                status: SsePollStatus.lagged_resynced,
                event_id: 0,
                skipped_events: skipped,
                event_idx: 0,
            };
        }
        return SsePollResult{ status: SsePollStatus.empty };
    }

    /// Returns a read-only copy of the stored event at `event_idx`, or `null` if out-of-range/empty.
    fn get_event(event_idx: usize): SseStoredEvent? {
        if (event_idx >= SSE_BROKER_MAX_HISTORY || _.ring[event_idx].event_id == 0) {
            return null;
        }
        return _.ring[event_idx];
    }

    /// Returns an operational snapshot of clients, ring occupancy, event ID bounds, and lag drops.
    fn stats(): SseBrokerStats {
        return SseBrokerStats{
            active_clients: _.active_clients,
            total_published: _.total_published,
            ring_count: _.ring_count,
            oldest_event_id: _.oldestEventId(),
            latest_event_id: _.latestEventId(),
            total_lag_drops: _.total_lag_drops,
        };
    }
}
