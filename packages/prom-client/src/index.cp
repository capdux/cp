// @cap/prom-text — Zero-heap Prometheus / OpenMetrics text exposition serializer
// MIT License (clean-room CP implementation)

import { isAlpha, isAlnum } from "cap:ascii";
import { writeStr, writeI64 } from "cap:fmt";

pub const METRIC_COUNTER: u8 = 0;
pub const METRIC_GAUGE: u8 = 1;
pub const METRIC_HISTOGRAM: u8 = 2;
pub const METRIC_SUMMARY: u8 = 3;

pub struct Label {
    pub key: []const u8,
    pub val: []const u8,
}

pub fn isValidMetricName(name: []const u8) -> bool {
    if (name.len == 0) {
        return false;
    }
    const c0: u8 = name[0];
    if (!isAlpha(c0) && c0 != '_' as u8 && c0 != ':' as u8) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        const c: u8 = name[i];
        if (!isAlnum(c) && c != '_' as u8 && c != ':' as u8) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isValidLabelName(name: []const u8) -> bool {
    if (name.len == 0) {
        return false;
    }
    const c0: u8 = name[0];
    if (!isAlpha(c0) && c0 != '_' as u8) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        const c: u8 = name[i];
        if (!isAlnum(c) && c != '_' as u8) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct PromWriter {
    pub buf: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(buf: []u8) -> PromWriter {
        return PromWriter {
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    fn writeByte(*_, b: u8) -> bool {
        if (_.overflow || _.pos >= _.buf.len) {
            _.overflow = true;
            return false;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8) -> bool {
        if (_.overflow || s.len > _.buf.len - _.pos) {
            _.overflow = true;
            return false;
        }
        _.pos = writeStr(s, _.buf, _.pos);
        return true;
    }

    fn writeInt(*_, v: i64) -> bool {
        let tmp: [24]u8 = undefined;
        const n = writeI64(v, tmp[0..], 0);
        return _.writeSlice(tmp[0..n]);
    }

    fn writeEscapedHelp(*_, help: []const u8) -> bool {
        let i: usize = 0;
        while (i < help.len) {
            const c: u8 = help[i];
            if (c == '\\' as u8) {
                if (!_.writeSlice("\\\\")) {
                    return false;
                }
            } else if (c == '\n' as u8) {
                if (!_.writeSlice("\\n")) {
                    return false;
                }
            } else {
                if (!_.writeByte(c)) {
                    return false;
                }
            }
            i += 1;
        }
        return true;
    }

    fn writeEscapedLabelVal(*_, val: []const u8) -> bool {
        let i: usize = 0;
        while (i < val.len) {
            const c: u8 = val[i];
            if (c == '\\' as u8) {
                if (!_.writeSlice("\\\\")) {
                    return false;
                }
            } else if (c == '"' as u8) {
                if (!_.writeSlice("\\\"")) {
                    return false;
                }
            } else if (c == '\n' as u8) {
                if (!_.writeSlice("\\n")) {
                    return false;
                }
            } else {
                if (!_.writeByte(c)) {
                    return false;
                }
            }
            i += 1;
        }
        return true;
    }

    pub fn writeHelp(*_, name: []const u8, help: []const u8) -> bool {
        if (_.overflow || !isValidMetricName(name)) {
            return false;
        }
        const saved: usize = _.pos;
        if (!_.writeSlice("# HELP ") || !_.writeSlice(name) || !_.writeByte(' ' as u8)
            || !_.writeEscapedHelp(help) || !_.writeByte('\n' as u8)) {
            _.pos = saved;
            return false;
        }
        return true;
    }

    pub fn writeType(*_, name: []const u8, kind: u8) -> bool {
        if (_.overflow || !isValidMetricName(name)) {
            return false;
        }
        let type_str: []const u8 = "";
        if (kind == METRIC_COUNTER) {
            type_str = "counter";
        } else if (kind == METRIC_GAUGE) {
            type_str = "gauge";
        } else if (kind == METRIC_HISTOGRAM) {
            type_str = "histogram";
        } else if (kind == METRIC_SUMMARY) {
            type_str = "summary";
        } else {
            return false;
        }

        const saved: usize = _.pos;
        if (!_.writeSlice("# TYPE ") || !_.writeSlice(name) || !_.writeByte(' ' as u8)
            || !_.writeSlice(type_str) || !_.writeByte('\n' as u8)) {
            _.pos = saved;
            return false;
        }
        return true;
    }

    fn writeSamplePrefix(*_, name: []const u8, labels: []const Label) -> bool {
        if (_.overflow || !isValidMetricName(name)) {
            return false;
        }
        let i: usize = 0;
        while (i < labels.len) {
            if (!isValidLabelName(labels[i].key)) {
                return false;
            }
            i += 1;
        }

        if (!_.writeSlice(name)) {
            return false;
        }
        if (labels.len > 0) {
            if (!_.writeByte('{' as u8)) {
                return false;
            }
            i = 0;
            while (i < labels.len) {
                if (i > 0) {
                    if (!_.writeByte(',' as u8)) {
                        return false;
                    }
                }
                if (!_.writeSlice(labels[i].key) || !_.writeSlice("=\"")
                    || !_.writeEscapedLabelVal(labels[i].val) || !_.writeByte('"' as u8)) {
                    return false;
                }
                i += 1;
            }
            if (!_.writeByte('}' as u8)) {
                return false;
            }
        }
        return _.writeByte(' ' as u8);
    }

    pub fn writeSampleInt(*_, name: []const u8, labels: []const Label, value: i64) -> bool {
        const saved: usize = _.pos;
        if (!_.writeSamplePrefix(name, labels) || !_.writeInt(value) || !_.writeByte('\n' as u8)) {
            _.pos = saved;
            return false;
        }
        return true;
    }

    pub fn writeSampleWithTs(
        *_,
        name: []const u8,
        labels: []const Label,
        value: i64,
        ts_ms: i64,
    ) -> bool {
        const saved: usize = _.pos;
        if (!_.writeSamplePrefix(name, labels) || !_.writeInt(value) || !_.writeByte(' ' as u8)
            || !_.writeInt(ts_ms) || !_.writeByte('\n' as u8)) {
            _.pos = saved;
            return false;
        }
        return true;
    }

    pub fn written() -> []const u8 {
        return _.buf[0.._.pos];
    }

    pub fn len() -> usize {
        return _.pos;
    }

    pub fn isOverflow() -> bool {
        return _.overflow;
    }
}
