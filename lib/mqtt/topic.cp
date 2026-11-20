// Source: cp/lib/mqtt/topic.cp (MQTT §4.7 Topic Name/Filter Validation & Wildcard Matching)
const std = @import("std");
import { MqttError } from "./types.cp";

pub fn validateTopicName(topic: []const u8): MqttError!void {
    if (topic.len == 0 or topic.len > 65535) {
        return MqttError.InvalidTopicName;
    }
    if (!std.unicode.utf8ValidateSlice(topic)) {
        return MqttError.InvalidUtf8;
    }
    let i: usize = 0;
    while (i < topic.len) {
        const c = topic[i];
        if (c == 0 or c == '+' or c == '#') {
            return MqttError.InvalidTopicName;
        }
        i += 1;
    }
}

pub fn validateTopicFilter(filter: []const u8): MqttError!void {
    if (filter.len == 0 or filter.len > 65535) {
        return MqttError.InvalidTopicFilter;
    }
    if (!std.unicode.utf8ValidateSlice(filter)) {
        return MqttError.InvalidUtf8;
    }

    let level_start: usize = 0;
    let i: usize = 0;
    while (i <= filter.len) {
        if (i == filter.len or filter[i] == '/') {
            const level = filter[level_start..i];
            let j: usize = 0;
            while (j < level.len) {
                const c = level[j];
                if (c == 0) {
                    return MqttError.InvalidTopicFilter;
                }
                if (c == '+' and level.len != 1) {
                    return MqttError.InvalidTopicFilter;
                }
                if (c == '#') {
                    if (level.len != 1 or i != filter.len) {
                        return MqttError.InvalidTopicFilter;
                    }
                }
                j += 1;
            }
            level_start = i + 1;
        } else if (filter[i] == 0) {
            return MqttError.InvalidTopicFilter;
        }
        i += 1;
    }
}

pub fn topicMatches(filter: []const u8, topic: []const u8): bool {
    if (filter.len == 0 or topic.len == 0) {
        return false;
    }

    // MQTT §4.7.2: Topics beginning with '$' must not match filters starting with '+' or '#'
    if (topic[0] == '$' and (filter[0] == '+' or filter[0] == '#')) {
        return false;
    }

    let f_pos: usize = 0;
    let t_pos: usize = 0;

    while (true) {
        // Find end of current filter level
        let f_end = f_pos;
        while (f_end < filter.len and filter[f_end] != '/') {
            f_end += 1;
        }
        const f_level = filter[f_pos..f_end];

        // Multi-level wildcard '#' matches current and all subsequent levels (including zero levels)
        if (f_level.len == 1 and f_level[0] == '#') {
            return f_end == filter.len;
        }

        // If topic has no more levels, no match unless next filter level is "/#"
        if (t_pos > topic.len) {
            return false;
        }

        let t_end = t_pos;
        while (t_end < topic.len and topic[t_end] != '/') {
            t_end += 1;
        }
        const t_level = topic[t_pos..t_end];

        if (!(f_level.len == 1 and f_level[0] == '+')) {
            if (!std.mem.eql(u8, f_level, t_level)) {
                return false;
            }
        }

        const f_has_more = f_end < filter.len;
        const t_has_more = t_end < topic.len;

        if (!f_has_more and !t_has_more) {
            return true;
        }

        if (f_has_more and !t_has_more) {
            // Check MQTT §4.7.1.2: "sport/tennis/player1/#" matches "sport/tennis/player1"
            const rem = filter[f_end + 1 ..];
            return std.mem.eql(u8, rem, "#");
        }

        if (!f_has_more and t_has_more) {
            return false;
        }

        f_pos = f_end + 1;
        t_pos = t_end + 1;
    }
}
