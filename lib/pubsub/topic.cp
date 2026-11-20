// Source: cp/lib/pubsub/topic.cp (NATS & MQTT Wildcard Topic Matching Engine)
// Zero-allocation NATS (`.` / `*` / `>`) and MQTT (`/` / `+` / `#`) wildcard
// topic validation and matching engine for cap:pubsub.

const std = @import("std");

/// Wildcard topic dialect supported by `TopicMatcher` and `PubSubBroker`.
pub enum TopicStyle: u8 {
    Nats = 0,
    Mqtt = 1,

    /// Returns the hierarchical segment separator byte (`'.'` for NATS, `'/'` for MQTT).
    fn separator(): u8 {
        return switch (_) {
            .Nats => '.',
            .Mqtt => '/',
        };
    }

    /// Returns the single-segment wildcard byte (`'*'` for NATS, `'+'` for MQTT).
    fn singleWildcard(): u8 {
        return switch (_) {
            .Nats => '*',
            .Mqtt => '+',
        };
    }

    /// Returns the multi-segment tail wildcard byte (`'>'` for NATS, `'#'` for MQTT).
    fn multiWildcard(): u8 {
        return switch (_) {
            .Nats => '>',
            .Mqtt => '#',
        };
    }
}

/// Pure static wildcard topic validator and matcher for NATS and MQTT dialects.
pub struct TopicMatcher {
    /// Validates a subscription `pattern` under `style`.
    /// Rejects empty patterns, empty segments (leading, trailing, or consecutive separators),
    /// partial wildcard tokens (e.g. `"foo*"` or `"a+"`), and non-trailing `>` / `#` wildcards.
    pub static fn isValidPattern(style: TopicStyle, pattern: string): bool {
        if (pattern.len == 0) {
            return false;
        }
        const sep = style.separator();
        const single = style.singleWildcard();
        const multi = style.multiWildcard();

        let pos: usize = 0;
        while (true) {
            let end: usize = pos;
            while (end < pattern.len && pattern[end] != sep) : (end += 1) {}
            const seg = pattern[pos..end];
            if (seg.len == 0) {
                return false;
            }

            let has_single = false;
            let has_multi = false;
            for (i of 0..seg.len) {
                const c = seg[i];
                if (c == single) {
                    has_single = true;
                } else if (c == multi) {
                    has_multi = true;
                }
            }

            if (has_single && seg.len != 1) {
                return false;
            }
            if (has_multi) {
                if (seg.len != 1) {
                    return false;
                }
                // Multi-level wildcard (`>` or `#`) is only valid as the final segment.
                if (end < pattern.len) {
                    return false;
                }
            }

            if (end == pattern.len) {
                break;
            }
            pos = end + 1;
        }
        return true;
    }

    /// Validates a concrete publish `topic` under `style`.
    /// Rejects empty topics, empty segments, and any occurrence of style wildcards.
    pub static fn isValidPublishTopic(style: TopicStyle, topic: string): bool {
        if (topic.len == 0) {
            return false;
        }
        const sep = style.separator();
        const single = style.singleWildcard();
        const multi = style.multiWildcard();

        let pos: usize = 0;
        while (true) {
            let end: usize = pos;
            while (end < topic.len && topic[end] != sep) : (end += 1) {}
            if (end == pos) {
                return false;
            }
            let i: usize = pos;
            while (i < end) : (i += 1) {
                const c = topic[i];
                if (c == single || c == multi) {
                    return false;
                }
            }
            if (end == topic.len) {
                break;
            }
            pos = end + 1;
        }
        return true;
    }

    /// Returns `true` if `pattern` matches `topic` under `style`.
    /// Invalid patterns or publish topics containing wildcards always return `false`.
    /// - NATS `>`: matches 1 or more trailing segments (`"a.>"` matches `"a.b"`, not `"a"`).
    /// - MQTT `#`: matches 0 or more trailing segments, parent-inclusive (`"a/#"` matches `"a"` and `"a/b"`).
    pub static fn matches(style: TopicStyle, pattern: string, topic: string): bool {
        if (!TopicMatcher.isValidPattern(style, pattern)) {
            return false;
        }
        if (!TopicMatcher.isValidPublishTopic(style, topic)) {
            return false;
        }

        const sep = style.separator();
        const single = style.singleWildcard();
        const multi = style.multiWildcard();

        let p_pos: usize = 0;
        let t_pos: usize = 0;
        let p_done = false;
        let t_done = false;

        while (!p_done) {
            let p_end: usize = p_pos;
            while (p_end < pattern.len && pattern[p_end] != sep) : (p_end += 1) {}
            const p_seg = pattern[p_pos..p_end];
            if (p_end == pattern.len) {
                p_done = true;
            } else {
                p_pos = p_end + 1;
            }

            if (p_seg.len == 1 && p_seg[0] == multi) {
                if (style == .Mqtt) {
                    return true;
                }
                return !t_done;
            }

            if (t_done) {
                return false;
            }

            let t_end: usize = t_pos;
            while (t_end < topic.len && topic[t_end] != sep) : (t_end += 1) {}
            const t_seg = topic[t_pos..t_end];
            if (t_end == topic.len) {
                t_done = true;
            } else {
                t_pos = t_end + 1;
            }

            if (p_seg.len == 1 && p_seg[0] == single) {
                continue;
            }
            if (!std.mem.eql(u8, p_seg, t_seg)) {
                return false;
            }
        }

        return t_done;
    }
}
