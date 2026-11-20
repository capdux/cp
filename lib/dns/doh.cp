// Source: cp/lib/dns/doh.cp (RFC 8484 DNS-over-HTTPS Wire & JSON Codec)
// lib/dns/doh.cp
// RFC 8484 DNS-over-HTTPS (DoH) GET/POST Wire Builder & Cloudflare/Google JSON Response Parser

const std = @import("std");
import { Base64 } from "cap:encoding";
import { JSON, JsonValue } from "cap:json";
import { NSType } from "./_cares.cp";
import { DnsRCode, DnsWireCodec } from "./packet.cp";

pub err DohError {
    EmptyWireQuery,
    InvalidEndpoint,
    InvalidQueryPacket,
    InvalidJsonResponse,
    MissingField,
    InvalidFieldType,
}

pub struct DohHeader {
    pub name: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct DohPostHeaders {
    pub content_type: []const u8 = "application/dns-message";
    pub accept: []const u8 = "application/dns-message";
    pub headers: [2]DohHeader = [
        DohHeader{ name: "Content-Type", value: "application/dns-message" },
        DohHeader{ name: "Accept", value: "application/dns-message" },
    ];
}

/// RFC 8484 DNS-over-HTTPS request builder for GET (`?dns=<base64url>`) and POST (`application/dns-message`).
pub struct DohQueryBuilder {
    pub const MEDIA_TYPE: []const u8 = "application/dns-message";

    /// Builds an RFC 8484 Section 4.1 GET request URL with unpadded Base64URL-encoded `wire_query`.
    pub static fn buildGetUrl(alloc: std.mem.Allocator, endpoint: []const u8, wire_query: []const u8): ([]u8)! {
        if (endpoint.len == 0) return DohError.InvalidEndpoint;
        if (wire_query.len == 0) return DohError.EmptyWireQuery;

        let sep: []const u8 = "?";
        const last = endpoint[endpoint.len - 1];
        if (last == '?' || last == '&') {
            sep = "";
        } else if (std.mem.indexOfScalar(u8, endpoint, '?') != null) {
            sep = "&";
        }

        const param_prefix = "dns=";
        const b64_len = Base64.encodeLenURLSafe(wire_query.len);
        const total_len = endpoint.len + sep.len + param_prefix.len + b64_len;

        let out = alloc.alloc(u8, total_len)?;
        errdefer alloc.free(out);

        let pos: usize = 0;
        @memcpy(out[pos .. pos + endpoint.len], endpoint);
        pos += endpoint.len;
        if (sep.len > 0) {
            @memcpy(out[pos .. pos + sep.len], sep);
            pos += sep.len;
        }
        @memcpy(out[pos .. pos + param_prefix.len], param_prefix);
        pos += param_prefix.len;

        const written = Base64.encodeURLSafe(out[pos..total_len], wire_query);
        return out[0 .. pos + written];
    }

    /// Convenience helper: builds an RFC 1035 DNS wire query packet and encodes it into a DoH GET URL.
    pub static fn buildDnsQueryGetUrl(
        alloc: std.mem.Allocator,
        endpoint: []const u8,
        id: u16,
        domain: []const u8,
        qtype: NSType,
    ): ([]u8)! {
        let pkt_buf: [512]u8 = [0; 512];
        const pkt_len = DnsWireCodec.buildQueryPacket(id, domain, qtype, &pkt_buf);
        if (pkt_len == 0) return DohError.InvalidQueryPacket;
        return DohQueryBuilder.buildGetUrl(alloc, endpoint, pkt_buf[0..pkt_len]);
    }

    /// Decodes a Base64URL `dns` query parameter value back into raw DNS wire bytes.
    pub static fn decodeDnsParam(alloc: std.mem.Allocator, b64url: []const u8): ([]u8)! {
        if (b64url.len == 0) return DohError.EmptyWireQuery;
        return Base64.decodeURLSafeAlloc(alloc, b64url);
    }

    /// Returns the RFC 8484 Section 4.1 / 6 HTTP headers required for DoH POST requests.
    pub static fn buildPostHeaders(): DohPostHeaders {
        return DohPostHeaders{};
    }
}

pub struct DohJsonQuestion {
    pub name: []u8 = [];
    pub rtype: u16 = 0;
}

pub struct DohJsonAnswer {
    pub name: []u8 = [];
    pub rtype: u16 = 0;
    pub ttl: u32 = 0;
    pub data: []u8 = [];

    pub fn nsType(): NSType {
        return DnsWireCodec.nsTypeFromU16(_.rtype);
    }
}

/// Parser for Cloudflare / Google DNS-over-HTTPS JSON responses (`application/dns-json`).
pub struct DohJsonResponse {
    pub status: u16 = 0;
    pub tc: bool = false;
    pub rd: bool = false;
    pub ra: bool = false;
    pub ad: bool = false;
    pub cd: bool = false;
    pub questions: []DohJsonQuestion = [];
    pub answers: []DohJsonAnswer = [];

    pub fn rcode(): DnsRCode {
        return switch (_.status) {
            0 => DnsRCode.NoError,
            1 => DnsRCode.FormErr,
            2 => DnsRCode.ServFail,
            3 => DnsRCode.NXDomain,
            4 => DnsRCode.NotImp,
            else => DnsRCode.Refused,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (q of _.questions) {
            if (q.name.len > 0) alloc.free(q.name);
        }
        if (_.questions.len > 0) {
            alloc.free(_.questions);
        }
        for (a of _.answers) {
            if (a.name.len > 0) alloc.free(a.name);
            if (a.data.len > 0) alloc.free(a.data);
        }
        if (_.answers.len > 0) {
            alloc.free(_.answers);
        }
        _.questions = [];
        _.answers = [];
    }

    static fn readRequiredU16(obj: JsonValue, key: []const u8): u16! {
        const val = obj.get(key) ?? return DohError.MissingField;
        const n = val.asNumber() ?? return DohError.InvalidFieldType;
        if (n < 0 || n > 65535) return DohError.InvalidFieldType;
        return (n as u16);
    }

    static fn readOptionalBool(obj: JsonValue, key: []const u8): bool! {
        const val = obj.get(key) ?? return false;
        return val.asBool() ?? return DohError.InvalidFieldType;
    }

    /// Parses a DoH JSON response body into an owned `DohJsonResponse`.
    /// Call `.deinit(alloc)` when finished.
    pub static fn parse(alloc: std.mem.Allocator, json_bytes: []const u8): DohJsonResponse! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();

        const root = JSON.parse(tmp_arena.allocator(), json_bytes) catch return DohError.InvalidJsonResponse;
        if (!root.isObject()) return DohError.InvalidJsonResponse;

        let resp = DohJsonResponse{
            status: DohJsonResponse.readRequiredU16(root, "Status")?,
            tc: DohJsonResponse.readOptionalBool(root, "TC")?,
            rd: DohJsonResponse.readOptionalBool(root, "RD")?,
            ra: DohJsonResponse.readOptionalBool(root, "RA")?,
            ad: DohJsonResponse.readOptionalBool(root, "AD")?,
            cd: DohJsonResponse.readOptionalBool(root, "CD")?,
        };
        errdefer resp.deinit(alloc);

        if (q_val := root.get("Question")) {
            const q_arr = q_val.asArray() ?? return DohError.InvalidFieldType;
            if (q_arr.len > 0) {
                let q_list: std.ArrayList(DohJsonQuestion) = .empty;
                errdefer {
                    for (q of q_list.items) {
                        if (q.name.len > 0) alloc.free(q.name);
                    }
                    q_list.deinit(alloc);
                }
                for (item of q_arr) {
                    if (!item.isObject()) return DohError.InvalidFieldType;
                    const name_val = item.get("name") ?? return DohError.MissingField;
                    const name_str = name_val.asString() ?? return DohError.InvalidFieldType;
                    const rtype = DohJsonResponse.readRequiredU16(item, "type")?;
                    const owned_name = alloc.dupe(u8, name_str)?;
                    errdefer alloc.free(owned_name);
                    q_list.append(alloc, DohJsonQuestion{
                        name: owned_name,
                        rtype: rtype,
                    })?;
                }
                resp.questions = q_list.toOwnedSlice(alloc)?;
            }
        }

        if (a_val := root.get("Answer")) {
            const a_arr = a_val.asArray() ?? return DohError.InvalidFieldType;
            if (a_arr.len > 0) {
                let a_list: std.ArrayList(DohJsonAnswer) = .empty;
                errdefer {
                    for (a of a_list.items) {
                        if (a.name.len > 0) alloc.free(a.name);
                        if (a.data.len > 0) alloc.free(a.data);
                    }
                    a_list.deinit(alloc);
                }
                for (item of a_arr) {
                    if (!item.isObject()) return DohError.InvalidFieldType;
                    const name_val = item.get("name") ?? return DohError.MissingField;
                    const name_str = name_val.asString() ?? return DohError.InvalidFieldType;
                    const rtype = DohJsonResponse.readRequiredU16(item, "type")?;
                    const ttl_val = item.get("TTL") ?? return DohError.MissingField;
                    const ttl_num = ttl_val.asNumber() ?? return DohError.InvalidFieldType;
                    if (ttl_num < 0 || ttl_num > 4294967295) return DohError.InvalidFieldType;
                    const data_val = item.get("data") ?? return DohError.MissingField;
                    const data_str = data_val.asString() ?? return DohError.InvalidFieldType;

                    const owned_name = alloc.dupe(u8, name_str)?;
                    errdefer alloc.free(owned_name);
                    const owned_data = alloc.dupe(u8, data_str)?;
                    errdefer alloc.free(owned_data);

                    a_list.append(alloc, DohJsonAnswer{
                        name: owned_name,
                        rtype: rtype,
                        ttl: ttl_num as u32,
                        data: owned_data,
                    })?;
                }
                resp.answers = a_list.toOwnedSlice(alloc)?;
            }
        }

        return resp;
    }
}
