// Source: cp/lib/kafka_wire/protocol.cp (Kafka Wire Protocol frames, headers, ApiVersions, Metadata & StreamFramer)
const std = @import("std");
import { KafkaError, KafkaReader, KafkaWriter } from "./kafkaWire.cp";

pub enum ApiKey: i16 {
    Produce = 0,
    Fetch = 1,
    ListOffsets = 2,
    Metadata = 3,
    OffsetCommit = 8,
    OffsetFetch = 9,
    FindCoordinator = 10,
    JoinGroup = 11,
    Heartbeat = 12,
    LeaveGroup = 13,
    SyncGroup = 14,
    DescribeGroups = 15,
    ListGroups = 16,
    SaslHandshake = 17,
    ApiVersions = 18,
    CreateTopics = 19,
    DeleteTopics = 20,
    SaslAuthenticate = 36,
}

pub struct RequestHeader {
    api_key: i16,
    api_version: i16,
    correlation_id: i32,
    client_id: ([]const u8)?,
    header_version: u8,

    pub fn encode(w: *KafkaWriter): KafkaError!void {
        w.writeI16(_.api_key)?;
        w.writeI16(_.api_version)?;
        w.writeI32(_.correlation_id)?;
        w.writeNullableString(_.client_id)?;
        if (_.header_version >= 2) {
            w.writeEmptyTaggedFields()?;
        }
    }

    pub static fn decode(r: *KafkaReader, header_version: u8): KafkaError!RequestHeader {
        const api_key = r.readI16()?;
        const api_version = r.readI16()?;
        const correlation_id = r.readI32()?;
        const client_id = r.readNullableString()?;
        if (header_version >= 2) {
            r.skipTaggedFields()?;
        }
        return RequestHeader{
            .api_key = api_key,
            .api_version = api_version,
            .correlation_id = correlation_id,
            .client_id = client_id,
            .header_version = header_version,
        };
    }
}

pub struct ResponseHeader {
    correlation_id: i32,
    header_version: u8,

    pub fn encode(w: *KafkaWriter): KafkaError!void {
        w.writeI32(_.correlation_id)?;
        if (_.header_version >= 1) {
            w.writeEmptyTaggedFields()?;
        }
    }

    pub static fn decode(r: *KafkaReader, header_version: u8): KafkaError!ResponseHeader {
        const correlation_id = r.readI32()?;
        if (header_version >= 1) {
            r.skipTaggedFields()?;
        }
        return ResponseHeader{
            .correlation_id = correlation_id,
            .header_version = header_version,
        };
    }
}

pub struct ApiVersionRange {
    api_key: i16,
    min_version: i16,
    max_version: i16,
}

pub struct ApiVersionsReq {
    client_software_name: ([]const u8)?,
    client_software_version: ([]const u8)?,

    pub fn encode(w: *KafkaWriter, api_version: i16): KafkaError!void {
        if (api_version >= 3) {
            w.writeCompactString(_.client_software_name)?;
            w.writeCompactString(_.client_software_version)?;
            w.writeEmptyTaggedFields()?;
        }
    }

    pub static fn decode(r: *KafkaReader, api_version: i16): KafkaError!ApiVersionsReq {
        if (api_version >= 3) {
            const name = r.readCompactString()?;
            const ver = r.readCompactString()?;
            r.skipTaggedFields()?;
            return ApiVersionsReq{
                .client_software_name = name,
                .client_software_version = ver,
            };
        }
        return ApiVersionsReq{
            .client_software_name = null,
            .client_software_version = null,
        };
    }
}

pub struct ApiVersionsResp {
    error_code: i16,
    api_keys: []const ApiVersionRange,
    throttle_time_ms: i32,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.api_keys);
    }

    pub fn encode(w: *KafkaWriter, api_version: i16): KafkaError!void {
        w.writeI16(_.error_code)?;
        if (api_version >= 3) {
            w.writeCompactArrayLen(_.api_keys.len)?;
            for (item of _.api_keys) {
                w.writeI16(item.api_key)?;
                w.writeI16(item.min_version)?;
                w.writeI16(item.max_version)?;
                w.writeEmptyTaggedFields()?;
            }
            w.writeI32(_.throttle_time_ms)?;
            w.writeEmptyTaggedFields()?;
        } else {
            w.writeI32(@as(i32, @intCast(_.api_keys.len)))?;
            for (item of _.api_keys) {
                w.writeI16(item.api_key)?;
                w.writeI16(item.min_version)?;
                w.writeI16(item.max_version)?;
            }
            if (api_version >= 1) {
                w.writeI32(_.throttle_time_ms)?;
            }
        }
    }

    pub static fn decode(alloc: std.mem.Allocator, r: *KafkaReader, api_version: i16): KafkaError!ApiVersionsResp {
        const error_code = r.readI16()?;
        let count: usize = 0;
        if (api_version >= 3) {
            count = (r.readCompactArrayLen()?) orelse 0;
        } else {
            const raw_len = r.readI32()?;
            if (raw_len < 0) {
                return KafkaError.InvalidLength;
            }
            count = @as(usize, @intCast(raw_len));
        }

        let list = std.ArrayList(ApiVersionRange).empty;
        errdefer list.deinit(alloc);

        let i: usize = 0;
        while (i < count) {
            const k = r.readI16()?;
            const min_v = r.readI16()?;
            const max_v = r.readI16()?;
            if (api_version >= 3) {
                r.skipTaggedFields()?;
            }
            list.append(alloc, ApiVersionRange{
                .api_key = k,
                .min_version = min_v,
                .max_version = max_v,
            }) catch return KafkaError.OutOfMemory;
            i += 1;
        }

        let throttle: i32 = 0;
        if (api_version >= 1) {
            throttle = r.readI32()?;
        }
        if (api_version >= 3) {
            r.skipTaggedFields()?;
        }

        const owned = list.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
        return ApiVersionsResp{
            .error_code = error_code,
            .api_keys = owned,
            .throttle_time_ms = throttle,
        };
    }
}

pub struct MetadataReq {
    topics: ([]const []const u8)?,
    allow_auto_topic_creation: bool,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (ts := _.topics) {
            alloc.free(ts);
        }
    }

    pub fn encode(w: *KafkaWriter, api_version: i16): KafkaError!void {
        if (api_version >= 9) {
            if (ts := _.topics) {
                w.writeCompactArrayLen(ts.len)?;
                for (name of ts) {
                    w.writeCompactString(name)?;
                    w.writeEmptyTaggedFields()?;
                }
            } else {
                w.writeCompactArrayLen(null)?;
            }
            w.writeU8(if (_.allow_auto_topic_creation) 1 else 0)?;
            w.writeEmptyTaggedFields()?;
        } else {
            if (ts := _.topics) {
                w.writeI32(@as(i32, @intCast(ts.len)))?;
                for (name of ts) {
                    w.writeString(name)?;
                }
            } else {
                w.writeI32(-1)?;
            }
            if (api_version >= 4) {
                w.writeU8(if (_.allow_auto_topic_creation) 1 else 0)?;
            }
        }
    }

    pub static fn decode(alloc: std.mem.Allocator, r: *KafkaReader, api_version: i16): KafkaError!MetadataReq {
        let topics: ([]const []const u8)? = null;
        if (api_version >= 9) {
            if (count := r.readCompactArrayLen()?) {
                let list = std.ArrayList(string).empty;
                errdefer list.deinit(alloc);
                let i: usize = 0;
                while (i < count) {
                    const name = r.readCompactString()?;
                    r.skipTaggedFields()?;
                    list.append(alloc, name orelse "") catch return KafkaError.OutOfMemory;
                    i += 1;
                }
                topics = list.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
            }
        } else {
            const raw_len = r.readI32()?;
            if (raw_len == -1) {
                topics = null;
            } else if (raw_len < -1) {
                return KafkaError.InvalidLength;
            } else {
                const count = @as(usize, @intCast(raw_len));
                let list = std.ArrayList(string).empty;
                errdefer list.deinit(alloc);
                let i: usize = 0;
                while (i < count) {
                    const name = r.readString()?;
                    list.append(alloc, name) catch return KafkaError.OutOfMemory;
                    i += 1;
                }
                topics = list.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
            }
        }

        let allow_auto = true;
        if (api_version >= 4) {
            allow_auto = (r.readU8()? != 0);
        }
        if (api_version >= 9) {
            r.skipTaggedFields()?;
        }

        return MetadataReq{
            .topics = topics,
            .allow_auto_topic_creation = allow_auto,
        };
    }
}

pub struct BrokerMeta {
    node_id: i32,
    host: []const u8,
    port: i32,
    rack: ([]const u8)?,
}

pub struct PartitionMeta {
    error_code: i16,
    partition_index: i32,
    leader_id: i32,
    replica_nodes: []const i32,
    isr_nodes: []const i32,
}

pub struct TopicMeta {
    error_code: i16,
    name: ([]const u8)?,
    is_internal: bool,
    partitions: []const PartitionMeta,
}

pub struct MetadataResp {
    throttle_time_ms: i32,
    brokers: []const BrokerMeta,
    cluster_id: ([]const u8)?,
    controller_id: i32,
    topics: []const TopicMeta,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.brokers);
        for (t of _.topics) {
            for (p of t.partitions) {
                alloc.free(p.replica_nodes);
                alloc.free(p.isr_nodes);
            }
            alloc.free(t.partitions);
        }
        alloc.free(_.topics);
    }

    pub fn encode(w: *KafkaWriter, api_version: i16): KafkaError!void {
        if (api_version >= 3) {
            w.writeI32(_.throttle_time_ms)?;
        }
        if (api_version >= 9) {
            w.writeCompactArrayLen(_.brokers.len)?;
            for (b of _.brokers) {
                w.writeI32(b.node_id)?;
                w.writeCompactString(b.host)?;
                w.writeI32(b.port)?;
                w.writeCompactString(b.rack)?;
                w.writeEmptyTaggedFields()?;
            }
            w.writeCompactString(_.cluster_id)?;
            w.writeI32(_.controller_id)?;
            w.writeCompactArrayLen(_.topics.len)?;
            for (t of _.topics) {
                w.writeI16(t.error_code)?;
                w.writeCompactString(t.name)?;
                w.writeU8(if (t.is_internal) 1 else 0)?;
                w.writeCompactArrayLen(t.partitions.len)?;
                for (p of t.partitions) {
                    w.writeI16(p.error_code)?;
                    w.writeI32(p.partition_index)?;
                    w.writeI32(p.leader_id)?;
                    w.writeI32(0)?;
                    w.writeCompactArrayLen(p.replica_nodes.len)?;
                    for (rn of p.replica_nodes) {
                        w.writeI32(rn)?;
                    }
                    w.writeCompactArrayLen(p.isr_nodes.len)?;
                    for (in_node of p.isr_nodes) {
                        w.writeI32(in_node)?;
                    }
                    w.writeCompactArrayLen(0)?;
                    w.writeEmptyTaggedFields()?;
                }
                w.writeEmptyTaggedFields()?;
            }
            w.writeEmptyTaggedFields()?;
        } else {
            w.writeI32(@as(i32, @intCast(_.brokers.len)))?;
            for (b of _.brokers) {
                w.writeI32(b.node_id)?;
                w.writeString(b.host)?;
                w.writeI32(b.port)?;
                if (api_version >= 1) {
                    w.writeNullableString(b.rack)?;
                }
            }
            if (api_version >= 2) {
                w.writeNullableString(_.cluster_id)?;
            }
            if (api_version >= 1) {
                w.writeI32(_.controller_id)?;
            }
            w.writeI32(@as(i32, @intCast(_.topics.len)))?;
            for (t of _.topics) {
                w.writeI16(t.error_code)?;
                w.writeString(t.name orelse "")?;
                if (api_version >= 1) {
                    w.writeU8(if (t.is_internal) 1 else 0)?;
                }
                w.writeI32(@as(i32, @intCast(t.partitions.len)))?;
                for (p of t.partitions) {
                    w.writeI16(p.error_code)?;
                    w.writeI32(p.partition_index)?;
                    w.writeI32(p.leader_id)?;
                    w.writeI32(@as(i32, @intCast(p.replica_nodes.len)))?;
                    for (rn of p.replica_nodes) {
                        w.writeI32(rn)?;
                    }
                    w.writeI32(@as(i32, @intCast(p.isr_nodes.len)))?;
                    for (in_node of p.isr_nodes) {
                        w.writeI32(in_node)?;
                    }
                }
            }
        }
    }

    pub static fn decode(alloc: std.mem.Allocator, r: *KafkaReader, api_version: i16): KafkaError!MetadataResp {
        let throttle_time_ms: i32 = 0;
        if (api_version >= 3) {
            throttle_time_ms = r.readI32()?;
        }

        let brokers_count: usize = 0;
        if (api_version >= 9) {
            brokers_count = (r.readCompactArrayLen()?) orelse 0;
        } else {
            const raw_b = r.readI32()?;
            if (raw_b < 0) {
                return KafkaError.InvalidLength;
            }
            brokers_count = @as(usize, @intCast(raw_b));
        }

        let brokers = std.ArrayList(BrokerMeta).empty;
        errdefer brokers.deinit(alloc);

        let bi: usize = 0;
        while (bi < brokers_count) {
            const node_id = r.readI32()?;
            let host: []const u8 = "";
            let port: i32 = 0;
            let rack: ([]const u8)? = null;
            if (api_version >= 9) {
                host = (r.readCompactString()?) orelse "";
                port = r.readI32()?;
                rack = r.readCompactString()?;
                r.skipTaggedFields()?;
            } else {
                host = r.readString()?;
                port = r.readI32()?;
                if (api_version >= 1) {
                    rack = r.readNullableString()?;
                }
            }
            brokers.append(alloc, BrokerMeta{
                .node_id = node_id,
                .host = host,
                .port = port,
                .rack = rack,
            }) catch return KafkaError.OutOfMemory;
            bi += 1;
        }

        let cluster_id: ([]const u8)? = null;
        if (api_version >= 9) {
            cluster_id = r.readCompactString()?;
        } else if (api_version >= 2) {
            cluster_id = r.readNullableString()?;
        }

        let controller_id: i32 = -1;
        if (api_version >= 1) {
            controller_id = r.readI32()?;
        }

        let topics_count: usize = 0;
        if (api_version >= 9) {
            topics_count = (r.readCompactArrayLen()?) orelse 0;
        } else {
            const raw_t = r.readI32()?;
            if (raw_t < 0) {
                return KafkaError.InvalidLength;
            }
            topics_count = @as(usize, @intCast(raw_t));
        }

        let topics = std.ArrayList(TopicMeta).empty;
        errdefer {
            for (t of topics.items) {
                for (p of t.partitions) {
                    alloc.free(p.replica_nodes);
                    alloc.free(p.isr_nodes);
                }
                alloc.free(t.partitions);
            }
            topics.deinit(alloc);
        }

        let ti: usize = 0;
        while (ti < topics_count) {
            const t_err = r.readI16()?;
            let t_name: ([]const u8)? = null;
            let is_internal = false;
            if (api_version >= 9) {
                t_name = r.readCompactString()?;
                is_internal = (r.readU8()? != 0);
            } else {
                t_name = r.readString()?;
                if (api_version >= 1) {
                    is_internal = (r.readU8()? != 0);
                }
            }

            let part_count: usize = 0;
            if (api_version >= 9) {
                part_count = (r.readCompactArrayLen()?) orelse 0;
            } else {
                const raw_p = r.readI32()?;
                if (raw_p < 0) {
                    return KafkaError.InvalidLength;
                }
                part_count = @as(usize, @intCast(raw_p));
            }

            let parts = std.ArrayList(PartitionMeta).empty;
            errdefer {
                for (p of parts.items) {
                    alloc.free(p.replica_nodes);
                    alloc.free(p.isr_nodes);
                }
                parts.deinit(alloc);
            }

            let pi: usize = 0;
            while (pi < part_count) {
                const p_err = r.readI16()?;
                const p_idx = r.readI32()?;
                const p_leader = r.readI32()?;
                if (api_version >= 7) {
                    * = r.readI32()?;
                }
                const rep_nodes = decodeI32Array(alloc, r, api_version >= 9)?;
                errdefer alloc.free(rep_nodes);
                const isr_nodes = decodeI32Array(alloc, r, api_version >= 9)?;
                errdefer alloc.free(isr_nodes);
                if (api_version >= 5) {
                    const off_nodes = decodeI32Array(alloc, r, api_version >= 9)?;
                    alloc.free(off_nodes);
                }
                if (api_version >= 9) {
                    r.skipTaggedFields()?;
                }
                parts.append(alloc, PartitionMeta{
                    .error_code = p_err,
                    .partition_index = p_idx,
                    .leader_id = p_leader,
                    .replica_nodes = rep_nodes,
                    .isr_nodes = isr_nodes,
                }) catch return KafkaError.OutOfMemory;
                pi += 1;
            }

            if (api_version >= 9) {
                r.skipTaggedFields()?;
            }

            const owned_parts = parts.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
            topics.append(alloc, TopicMeta{
                .error_code = t_err,
                .name = t_name,
                .is_internal = is_internal,
                .partitions = owned_parts,
            }) catch return KafkaError.OutOfMemory;
            ti += 1;
        }

        if (api_version >= 9) {
            r.skipTaggedFields()?;
        }

        const owned_brokers = brokers.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
        errdefer alloc.free(owned_brokers);
        const owned_topics = topics.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;

        return MetadataResp{
            .throttle_time_ms = throttle_time_ms,
            .brokers = owned_brokers,
            .cluster_id = cluster_id,
            .controller_id = controller_id,
            .topics = owned_topics,
        };
    }
}

fn decodeI32Array(alloc: std.mem.Allocator, r: *KafkaReader, compact: bool): KafkaError![]const i32 {
    let count: usize = 0;
    if (compact) {
        count = (r.readCompactArrayLen()?) orelse 0;
    } else {
        const raw = r.readI32()?;
        if (raw < 0) {
            return KafkaError.InvalidLength;
        }
        count = @as(usize, @intCast(raw));
    }
    let list = std.ArrayList(i32).empty;
    errdefer list.deinit(alloc);
    let i: usize = 0;
    while (i < count) {
        const v = r.readI32()?;
        list.append(alloc, v) catch return KafkaError.OutOfMemory;
        i += 1;
    }
    return list.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
}

pub struct KafkaStreamFramer {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    max_frame_bytes: usize,

    pub fn init(alloc: std.mem.Allocator, max_frame_bytes: usize): KafkaStreamFramer {
        return KafkaStreamFramer{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
            .max_frame_bytes = max_frame_bytes,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn push(*_, chunk: []const u8): KafkaError!void {
        _.buf.appendSlice(_.alloc, chunk) catch return KafkaError.OutOfMemory;
    }

    pub fn nextFrame(*_): KafkaError!([]u8)? {
        if (_.buf.items.len < 4) {
            return null;
        }
        const raw_len = std.mem.readInt(i32, _.buf.items[0..4], .big);
        if (raw_len < 0) {
            return KafkaError.InvalidLength;
        }
        const frame_len: usize = @as(usize, @intCast(raw_len));
        if (frame_len > _.max_frame_bytes) {
            return KafkaError.FrameTooLarge;
        }
        if (_.buf.items.len < 4 + frame_len) {
            return null;
        }
        const out = _.alloc.alloc(u8, frame_len) catch return KafkaError.OutOfMemory;
        @memcpy(out, _.buf.items[4 .. 4 + frame_len]);
        const rem = _.buf.items.len - (4 + frame_len);
        if (rem > 0) {
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[4 + frame_len ..]);
        }
        _.buf.items.len = rem;
        return out;
    }

    pub static fn framePayload(alloc: std.mem.Allocator, payload: []const u8): KafkaError![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);
        let hdr: [4]u8 = undefined;
        std.mem.writeInt(i32, &hdr, @as(i32, @intCast(payload.len)), .big);
        out.appendSlice(alloc, &hdr) catch return KafkaError.OutOfMemory;
        out.appendSlice(alloc, payload) catch return KafkaError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return KafkaError.OutOfMemory;
    }
}
