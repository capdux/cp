// Origin: Pure-CP Stdlib (MCP Resource URI Template Matcher & Subscription Dispatcher) | Created: 2026-11-22
// Zero-heap-allocation Model Context Protocol (MCP) resource URI template matcher (RFC 6570 simple
// variable segments) and fixed-slot client subscription fan-out dispatcher.

const std = @import("std");

/// Zero-copy slice capture for a single `{var}` parameter extracted from a resource URI.
pub struct UriParamCapture {
    name: string = "";
    value: string = "";
}

/// Fixed-slot descriptor for a registered MCP resource route (static URI or RFC 6570 template).
pub struct McpResourceRoute {
    route_id: u32 = 0;
    uri_template: string = "";
    mime_type: string = "";
    read_only: bool = true;
    occupied: bool = false;
}

/// Resolution result returned by `McpResourceRouter.resolveUri`.
pub struct McpResourceMatch {
    route_id: u32 = 0;
    slot_index: usize = 0;
    param_count: usize = 0;
    is_static: bool = false;
}

/// Fixed-slot subscription binding a client ID to a hashed resource URI.
pub struct McpSubscription {
    client_id: u32 = 0;
    uri_hash: u64 = 0;
    active: bool = false;
}

/// Cumulative operational telemetry snapshot for `McpResourceRouter`.
pub struct McpResourceRouterStats {
    route_count: usize = 0;
    route_capacity: usize = 0;
    subscription_count: usize = 0;
    subscription_capacity: usize = 0;
    total_resolves: u64 = 0;
    total_hits: u64 = 0;
    total_misses: u64 = 0;
    total_broadcasts: u64 = 0;
}

/// Computes a deterministic 64-bit FNV-1a hash for an MCP resource URI.
pub fn hashResourceUri(uri: string): u64 {
    let h: u64 = (u64(0xcbf29ce4) << 32) | u64(0x84222325);
    let i: usize = 0;
    while (i < uri.len) : (i += 1) {
        h = h ^ u64(uri[i]);
        h = h *% 0x100000001b3;
    }
    return h;
}

fn isWhitespaceByte(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

/// Validates that `template` is a non-empty, well-formed static URI or RFC 6570 simple `{var}`
/// template without empty/nested/unclosed/adjacent-undelimited variable expressions.
pub fn isValidUriTemplate(template: string): bool {
    if (template.len == 0) return false;
    let i: usize = 0;
    while (i < template.len) {
        const ch = template[i];
        if (isWhitespaceByte(ch)) return false;
        if (ch == '}') return false;
        if (ch == '{') {
            let j = i + 1;
            let closed = false;
            while (j < template.len) : (j += 1) {
                const c = template[j];
                if (c == '}') {
                    closed = true;
                    break;
                }
                if (c == '{' || c == '/' || isWhitespaceByte(c)) {
                    return false;
                }
            }
            if (!closed) return false;
            if (j <= i + 1) return false;
            const next_i = j + 1;
            if (next_i < template.len && template[next_i] == '{') {
                return false;
            }
            i = next_i;
            continue;
        }
        i += 1;
    }
    return true;
}

/// Returns `true` when `template` contains no `{var}` placeholders.
pub fn isStaticUriTemplate(template: string): bool {
    let i: usize = 0;
    while (i < template.len) : (i += 1) {
        if (template[i] == '{' || template[i] == '}') {
            return false;
        }
    }
    return template.len > 0;
}

/// Matches `uri` against an RFC 6570 simple `{var}` URI `template` (e.g. `db://{schema}/{table}`
/// matching `db://public/users`). Captures zero-copy parameter slices into `out_params` (truncation-safe)
/// and returns the total number of matched variables, or `null` if the template is invalid or does not match.
pub fn matchUriTemplate(
    template: string,
    uri: string,
    out_params: *[]UriParamCapture,
): usize? {
    if (template.len == 0 || uri.len == 0) return null;

    let ti: usize = 0;
    let ui: usize = 0;
    let param_count: usize = 0;

    while (ti < template.len) {
        const tch = template[ti];
        if (tch == '}') return null;

        if (tch == '{') {
            let close_idx = ti + 1;
            let found_close = false;
            while (close_idx < template.len) : (close_idx += 1) {
                const c = template[close_idx];
                if (c == '}') {
                    found_close = true;
                    break;
                }
                if (c == '{' || c == '/' || isWhitespaceByte(c)) {
                    return null;
                }
            }
            if (!found_close) return null;
            if (close_idx <= ti + 1) return null;

            const var_name = template[ti + 1 .. close_idx];
            const next_ti = close_idx + 1;
            if (next_ti < template.len && template[next_ti] == '{') {
                return null;
            }

            const val_start = ui;
            if (next_ti >= template.len) {
                while (ui < uri.len && uri[ui] != '/') : (ui += 1) {}
            } else {
                const delim = template[next_ti];
                while (ui < uri.len) : (ui += 1) {
                    const uch = uri[ui];
                    if (uch == delim) {
                        break;
                    }
                    if (uch == '/') {
                        return null;
                    }
                }
            }

            if (ui == val_start) return null;
            const val_slice = uri[val_start .. ui];

            if (param_count < out_params.len) {
                out_params[param_count] = UriParamCapture{
                    name: var_name,
                    value: val_slice,
                };
            }
            param_count += 1;
            ti = next_ti;
            continue;
        }

        if (ui >= uri.len || uri[ui] != tch) {
            return null;
        }
        ti += 1;
        ui += 1;
    }

    if (ui != uri.len) return null;
    return param_count;
}

/// Fixed-capacity, zero-heap-allocation MCP resource URI template router and subscription dispatcher.
pub struct McpResourceRouter {
    routes: []McpResourceRoute;
    subscriptions: []McpSubscription;
    route_count: usize = 0;
    subscription_count: usize = 0;
    total_resolves: u64 = 0;
    total_hits: u64 = 0;
    total_misses: u64 = 0;
    total_broadcasts: u64 = 0;

    /// Initializes a router backed by caller-owned fixed route and subscription slot pools.
    static fn init(
        routes: *[]McpResourceRoute,
        subscriptions: *[]McpSubscription,
    ): McpResourceRouter {
        let i: usize = 0;
        while (i < routes.len) : (i += 1) {
            routes[i] = McpResourceRoute{};
        }
        let j: usize = 0;
        while (j < subscriptions.len) : (j += 1) {
            subscriptions[j] = McpSubscription{};
        }
        return McpResourceRouter{
            routes: routes,
            subscriptions: subscriptions,
            route_count: 0,
            subscription_count: 0,
            total_resolves: 0,
            total_hits: 0,
            total_misses: 0,
            total_broadcasts: 0,
        };
    }

    /// Registers a static resource URI or RFC 6570 `{var}` template into the fixed route table.
    /// Rejects `route_id == 0`, invalid templates, duplicate `route_id`, duplicate `uri_template`,
    /// or full route pool by returning `null`.
    fn registerTemplate(
        *_,
        route_id: u32,
        uri_template: string,
        mime_type: string,
        read_only: bool,
    ): usize? {
        if (route_id == 0) return null;
        if (!isValidUriTemplate(uri_template)) return null;

        let free_slot: usize? = null;
        let i: usize = 0;
        while (i < _.routes.len) : (i += 1) {
            if (_.routes[i].occupied) {
                if (_.routes[i].route_id == route_id) return null;
                if (std.mem.eql(u8, _.routes[i].uri_template, uri_template)) return null;
            } else if (free_slot == null) {
                free_slot = i;
            }
        }

        if (slot := free_slot) {
            _.routes[slot] = McpResourceRoute{
                route_id: route_id,
                uri_template: uri_template,
                mime_type: mime_type,
                read_only: read_only,
                occupied: true,
            };
            _.route_count += 1;
            return slot;
        }
        return null;
    }

    /// Removes a registered route by `route_id`. Returns `false` if not found.
    fn unregisterRoute(*_, route_id: u32): bool {
        if (route_id == 0) return false;
        let i: usize = 0;
        while (i < _.routes.len) : (i += 1) {
            if (_.routes[i].occupied && _.routes[i].route_id == route_id) {
                _.routes[i] = McpResourceRoute{};
                if (_.route_count > 0) {
                    _.route_count -= 1;
                }
                return true;
            }
        }
        return false;
    }

    /// Returns a copy of the route descriptor at `slot_index`, or `null` if out of bounds or empty.
    fn getRoute(slot_index: usize): McpResourceRoute? {
        if (slot_index >= _.routes.len) return null;
        if (!_.routes[slot_index].occupied) return null;
        return _.routes[slot_index];
    }

    /// Resolves `uri` against registered routes, prioritizing exact static URI matches before
    /// falling back to RFC 6570 template matches in slot order.
    fn resolveUri(
        *_,
        uri: string,
        out_params: *[]UriParamCapture,
    ): McpResourceMatch? {
        _.total_resolves += 1;
        if (uri.len == 0) {
            _.total_misses += 1;
            return null;
        }

        // Pass 1: exact static routes take precedence over parameterized templates.
        let i: usize = 0;
        while (i < _.routes.len) : (i += 1) {
            if (!_.routes[i].occupied) continue;
            const tmpl = _.routes[i].uri_template;
            if (isStaticUriTemplate(tmpl) && std.mem.eql(u8, tmpl, uri)) {
                _.total_hits += 1;
                return McpResourceMatch{
                    route_id: _.routes[i].route_id,
                    slot_index: i,
                    param_count: 0,
                    is_static: true,
                };
            }
        }

        // Pass 2: parameterized URI templates in registration slot order.
        let j: usize = 0;
        while (j < _.routes.len) : (j += 1) {
            if (!_.routes[j].occupied) continue;
            const tmpl = _.routes[j].uri_template;
            if (isStaticUriTemplate(tmpl)) continue;
            if (param_cnt := matchUriTemplate(tmpl, uri, out_params)) {
                _.total_hits += 1;
                return McpResourceMatch{
                    route_id: _.routes[j].route_id,
                    slot_index: j,
                    param_count: param_cnt,
                    is_static: false,
                };
            }
        }

        _.total_misses += 1;
        return null;
    }

    /// Subscribes `client_id` to change notifications for `uri`.
    /// Duplicate `(client_id, uri)` subscriptions are idempotently deduplicated without consuming
    /// additional slots. Returns `false` if `client_id == 0`, `uri` is empty, or the pool is full.
    fn subscribe(*_, client_id: u32, uri: string): bool {
        if (client_id == 0 || uri.len == 0) return false;
        const h = hashResourceUri(uri);

        let free_slot: usize? = null;
        let i: usize = 0;
        while (i < _.subscriptions.len) : (i += 1) {
            if (_.subscriptions[i].active) {
                if (_.subscriptions[i].client_id == client_id && _.subscriptions[i].uri_hash == h) {
                    return true;
                }
            } else if (free_slot == null) {
                free_slot = i;
            }
        }

        if (slot := free_slot) {
            _.subscriptions[slot] = McpSubscription{
                client_id: client_id,
                uri_hash: h,
                active: true,
            };
            _.subscription_count += 1;
            return true;
        }
        return false;
    }

    /// Unsubscribes `client_id` from `uri`. Returns `false` if no matching subscription exists.
    fn unsubscribe(*_, client_id: u32, uri: string): bool {
        if (client_id == 0 || uri.len == 0) return false;
        const h = hashResourceUri(uri);

        let i: usize = 0;
        while (i < _.subscriptions.len) : (i += 1) {
            if (_.subscriptions[i].active && _.subscriptions[i].client_id == client_id && _.subscriptions[i].uri_hash == h) {
                _.subscriptions[i] = McpSubscription{};
                if (_.subscription_count > 0) {
                    _.subscription_count -= 1;
                }
                return true;
            }
        }
        return false;
    }

    /// Removes all active subscriptions held by `client_id` (e.g. on disconnect) and returns
    /// the number of cleared entries.
    fn unsubscribeClient(*_, client_id: u32): usize {
        if (client_id == 0) return 0;
        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.subscriptions.len) : (i += 1) {
            if (_.subscriptions[i].active && _.subscriptions[i].client_id == client_id) {
                _.subscriptions[i] = McpSubscription{};
                if (_.subscription_count > 0) {
                    _.subscription_count -= 1;
                }
                removed += 1;
            }
        }
        return removed;
    }

    /// Collects all subscribed `client_id`s for `uri` into `out_client_ids` (truncation-safe)
    /// and returns the number of client IDs written.
    fn collectSubscribers(*_, uri: string, out_client_ids: *[]u32): usize {
        _.total_broadcasts += 1;
        if (uri.len == 0) return 0;
        const h = hashResourceUri(uri);

        let written: usize = 0;
        let i: usize = 0;
        while (i < _.subscriptions.len) : (i += 1) {
            if (_.subscriptions[i].active && _.subscriptions[i].uri_hash == h) {
                if (written < out_client_ids.len) {
                    out_client_ids[written] = _.subscriptions[i].client_id;
                    written += 1;
                }
            }
        }
        return written;
    }

    /// Returns a read-only telemetry snapshot of route/subscription occupancy and traffic counters.
    fn stats(): McpResourceRouterStats {
        return McpResourceRouterStats{
            route_count: _.route_count,
            route_capacity: _.routes.len,
            subscription_count: _.subscription_count,
            subscription_capacity: _.subscriptions.len,
            total_resolves: _.total_resolves,
            total_hits: _.total_hits,
            total_misses: _.total_misses,
            total_broadcasts: _.total_broadcasts,
        };
    }
}
