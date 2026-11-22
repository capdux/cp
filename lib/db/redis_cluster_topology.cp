// Source: cp/lib/redis_cluster/topology.cp (Redis Cluster Topology, O(1) Slot Router & Redirect Parser)

const std = @import("std");
import { REDIS_CLUSTER_SLOTS, keySlot } from "./redis_cluster_crc16.cp";

pub const ClusterError = error{
    InvalidSlotRange,
    InvalidSlot,
    SlotNotCovered,
    InvalidRedirect,
    CrossSlot,
    ClusterDown,
    TryAgain,
    OutOfMemory,
};

pub struct NodeEndpoint {
    pub host: []const u8,
    pub port: u16,

    pub fn init(host: []const u8, port: u16): NodeEndpoint {
        return NodeEndpoint{
            .host = host,
            .port = port,
        };
    }

    pub fn equals(other: NodeEndpoint): bool {
        return _.port == other.port and std.mem.eql(u8, _.host, other.host);
    }
}

pub struct SlotRange {
    pub start: u16,
    pub end: u16,
    pub master: NodeEndpoint,
    pub replicas: []const NodeEndpoint,
}

pub enum RedirectKind {
    Moved,
    Ask,
    ClusterDown,
    TryAgain,
    CrossSlot,
}

pub struct ClusterRedirect {
    pub kind: RedirectKind,
    pub slot: u16,
    pub endpoint: NodeEndpoint?,
}

pub struct ClusterTopology {
    alloc: std.mem.Allocator,
    nodes: std.ArrayList(NodeEndpoint),
    slot_table: [16384]i32,
    covered_count: u32,

    pub fn init(alloc: std.mem.Allocator): ClusterTopology {
        let table: [16384]i32 = undefined;
        let i: usize = 0;
        while (i < 16384) {
            table[i] = -1;
            i += 1;
        }
        return ClusterTopology{
            .alloc = alloc,
            .nodes = std.ArrayList(NodeEndpoint).empty,
            .slot_table = table,
            .covered_count = 0,
        };
    }

    pub fn deinit(*_) {
        for (node of _.nodes.items) {
            _.alloc.free(node.host);
        }
        _.nodes.deinit(_.alloc);
    }

    fn getOrAddNode(*_, host: []const u8, port: u16): ClusterError!i32 {
        let idx: usize = 0;
        while (idx < _.nodes.items.len) {
            const node = _.nodes.items[idx];
            if (node.port == port and std.mem.eql(u8, node.host, host)) {
                return @as(i32, @intCast(idx));
            }
            idx += 1;
        }
        const owned_host = _.alloc.dupe(u8, host) catch return ClusterError.OutOfMemory;
        const new_idx = _.nodes.items.len;
        _.nodes.append(_.alloc, NodeEndpoint{
            .host = owned_host,
            .port = port,
        }) catch {
            _.alloc.free(owned_host);
            return ClusterError.OutOfMemory;
        };
        return @as(i32, @intCast(new_idx));
    }

    pub fn addSlotRange(
        *_,
        start: u16,
        end: u16,
        master: NodeEndpoint,
        replicas: []const NodeEndpoint,
    ): ClusterError!void {
        * = replicas;
        if (start > end or end >= REDIS_CLUSTER_SLOTS) {
            return ClusterError.InvalidSlotRange;
        }
        const node_idx = try _.getOrAddNode(master.host, master.port);
        let slot: u32 = @as(u32, start);
        const end_u32: u32 = @as(u32, end);
        while (slot <= end_u32) {
            if (_.slot_table[slot] < 0) {
                _.covered_count += 1;
            }
            _.slot_table[slot] = node_idx;
            slot += 1;
        }
    }

    pub fn routeSlot(slot: u16): ClusterError!NodeEndpoint {
        if (slot >= REDIS_CLUSTER_SLOTS) {
            return ClusterError.InvalidSlot;
        }
        const idx = _.slot_table[slot];
        if (idx < 0) {
            return ClusterError.SlotNotCovered;
        }
        return _.nodes.items[@as(usize, @intCast(idx))];
    }

    pub fn routeKey(key: []const u8): ClusterError!NodeEndpoint {
        const slot = keySlot(key);
        return _.routeSlot(slot);
    }

    pub fn applyMovedRedirect(
        *_,
        slot: u16,
        host: []const u8,
        port: u16,
    ): ClusterError!void {
        if (slot >= REDIS_CLUSTER_SLOTS) {
            return ClusterError.InvalidSlot;
        }
        const node_idx = try _.getOrAddNode(host, port);
        if (_.slot_table[slot] < 0) {
            _.covered_count += 1;
        }
        _.slot_table[slot] = node_idx;
    }

    pub fn isFullCoverage(): bool {
        return _.covered_count == @as(u32, REDIS_CLUSTER_SLOTS);
    }

    pub fn coveredSlots(): u32 {
        return _.covered_count;
    }
}

fn trimRedirectMsg(raw: []const u8): []const u8 {
    let s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len > 0 and s[0] == '-') {
        s = s[1..];
    }
    return s;
}

fn parseHostPort(addr: []const u8): ClusterError!NodeEndpoint {
    // Support IPv6 bracketed form "[::1]:6379" and IPv4/hostname "127.0.0.1:7001"
    let colon_idx: usize? = null;
    let i: usize = addr.len;
    while (i > 0) {
        i -= 1;
        if (addr[i] == ':') {
            colon_idx = i;
            break;
        }
    }
    if (colon_idx == null) {
        return ClusterError.InvalidRedirect;
    }
    const split = colon_idx.?;
    if (split == 0 or split + 1 >= addr.len) {
        return ClusterError.InvalidRedirect;
    }
    let host = addr[0..split];
    const port_str = addr[split + 1 ..];
    if (host.len >= 2 and host[0] == '[' and host[host.len - 1] == ']') {
        host = host[1 .. host.len - 1];
    }
    if (host.len == 0) {
        return ClusterError.InvalidRedirect;
    }
    const port = std.fmt.parseInt(u16, port_str, 10) catch return ClusterError.InvalidRedirect;
    return NodeEndpoint.init(host, port);
}

pub fn parseRedirectError(err_msg: []const u8): ClusterError!ClusterRedirect {
    const msg = trimRedirectMsg(err_msg);
    if (msg.len == 0) {
        return ClusterError.InvalidRedirect;
    }

    if (std.mem.startsWith(u8, msg, "CLUSTERDOWN")) {
        return ClusterRedirect{
            .kind = RedirectKind.ClusterDown,
            .slot = 0,
            .endpoint = null,
        };
    }
    if (std.mem.startsWith(u8, msg, "TRYAGAIN")) {
        return ClusterRedirect{
            .kind = RedirectKind.TryAgain,
            .slot = 0,
            .endpoint = null,
        };
    }
    if (std.mem.startsWith(u8, msg, "CROSSSLOT")) {
        return ClusterRedirect{
            .kind = RedirectKind.CrossSlot,
            .slot = 0,
            .endpoint = null,
        };
    }

    let it = std.mem.tokenizeScalar(u8, msg, ' ');
    const tag = it.next() orelse return ClusterError.InvalidRedirect;
    let kind: RedirectKind = RedirectKind.Moved;
    if (std.mem.eql(u8, tag, "MOVED")) {
        kind = RedirectKind.Moved;
    } else if (std.mem.eql(u8, tag, "ASK")) {
        kind = RedirectKind.Ask;
    } else {
        return ClusterError.InvalidRedirect;
    }

    const slot_str = it.next() orelse return ClusterError.InvalidRedirect;
    const addr_str = it.next() orelse return ClusterError.InvalidRedirect;
    const slot = std.fmt.parseInt(u16, slot_str, 10) catch return ClusterError.InvalidRedirect;
    if (slot >= REDIS_CLUSTER_SLOTS) {
        return ClusterError.InvalidSlot;
    }
    const endpoint = try parseHostPort(addr_str);
    return ClusterRedirect{
        .kind = kind,
        .slot = slot,
        .endpoint = endpoint,
    };
}
