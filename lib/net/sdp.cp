// Source: cp/lib/sdp/sdp.cp (RFC 8866 / RFC 4566 WebRTC SDP Session Description Parser & Builder)

const std = @import("std");

/// Errors returned by `cap:sdp` parsing and formatting operations.
pub err SdpError {
    InvalidVersion,
    MissingOrigin,
    MissingSessionName,
    MalformedLine,
    InvalidMediaLine,
    InvalidPort,
    OutOfMemory,
}

/// Represents a single SDP `a=` attribute line.
/// Flag attributes (e.g. `a=sendrecv`) have `value == null`.
/// Key-value attributes (e.g. `a=ice-ufrag:xxxx`) have `value != null`.
pub class SdpAttribute {
    pub key: []const u8,
    pub value: ([]const u8)?,
}

/// Represents a parsed RTP codec from an `a=rtpmap:` attribute.
pub class RtpCodec {
    pub payload_type: u8,
    pub name: []const u8,
    pub clock_rate: u32,
    pub encoding_params: ([]const u8)?,
}

/// Represents a parsed ICE candidate from an `a=candidate:` attribute.
pub class IceCandidate {
    pub foundation: []const u8,
    pub component: u32,
    pub transport: []const u8,
    pub priority: u64,
    pub address: []const u8,
    pub port: u16,
    pub cand_type: []const u8,
}

/// Represents a single `m=` media section and its associated `c=` and `a=` lines.
pub class MediaSection {
    pub media_type: []const u8,
    pub port: u16,
    pub proto: []const u8,
    pub formats: [][]const u8,
    pub connection: ([]const u8)?,
    pub attributes: []SdpAttribute,

    /// Releases the owned `formats` and `attributes` slices.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.formats.len > 0) {
            allocator.free(_.formats);
        }
        if (_.attributes.len > 0) {
            allocator.free(_.attributes);
        }
        _.formats = _.formats[0..0];
        _.attributes = _.attributes[0..0];
    }

    /// Returns the value of the first attribute matching `key`, or `null`.
    pub fn getAttr(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            const attr = _.attributes[i];
            if (std.mem.eql(u8, attr.key, key)) {
                return attr.value;
            }
        }
        return null;
    }

    /// Returns `true` if any attribute in this media section has `key`.
    pub fn hasFlag(key: []const u8): bool {
        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            if (std.mem.eql(u8, _.attributes[i].key, key)) {
                return true;
            }
        }
        return false;
    }

    /// Extracts all valid `a=rtpmap:` codec entries in this media section.
    pub fn rtpmaps(allocator: std.mem.Allocator): ([]RtpCodec) ! SdpError {
        let list: std.ArrayList(RtpCodec) = .empty;
        errdefer list.deinit(allocator);

        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            const attr = _.attributes[i];
            if (!std.mem.eql(u8, attr.key, "rtpmap")) {
                continue;
            }
            const val = attr.value orelse continue;
            const trimmed = std.mem.trim(u8, val, " \t");
            const sp = std.mem.indexOfScalar(u8, trimmed, ' ') orelse continue;

            const pt_str = trimmed[0..sp];
            const pt = std.fmt.parseInt(u8, pt_str, 10) catch continue;

            const rest = std.mem.trimLeft(u8, trimmed[sp + 1 ..], " \t");
            const slash1 = std.mem.indexOfScalar(u8, rest, '/') orelse continue;
            const codec_name = rest[0..slash1];
            const after_name = rest[slash1 + 1 ..];

            let clock_str = after_name;
            let params: ([]const u8)? = null;
            if (slash2 := std.mem.indexOfScalar(u8, after_name, '/')) {
                clock_str = after_name[0..slash2];
                const p = after_name[slash2 + 1 ..];
                if (p.len > 0) {
                    params = p;
                }
            }

            const clock_rate = std.fmt.parseInt(u32, clock_str, 10) catch continue;
            list.append(allocator, RtpCodec{
                payload_type: pt,
                name: codec_name,
                clock_rate: clock_rate,
                encoding_params: params,
            }) catch return SdpError.OutOfMemory;
        }

        return list.toOwnedSlice(allocator) catch return SdpError.OutOfMemory;
    }

    /// Extracts all valid `a=candidate:` ICE candidate entries in this media section.
    pub fn candidates(allocator: std.mem.Allocator): ([]IceCandidate) ! SdpError {
        let list: std.ArrayList(IceCandidate) = .empty;
        errdefer list.deinit(allocator);

        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            const attr = _.attributes[i];
            if (!std.mem.eql(u8, attr.key, "candidate")) {
                continue;
            }
            const val = attr.value orelse continue;
            let it = std.mem.tokenizeScalar(u8, val, ' ');

            const foundation = it.next() orelse continue;
            const comp_str = it.next() orelse continue;
            const transport = it.next() orelse continue;
            const prio_str = it.next() orelse continue;
            const address = it.next() orelse continue;
            const port_str = it.next() orelse continue;
            const typ_kw = it.next() orelse continue;
            const cand_type = it.next() orelse continue;

            if (!std.mem.eql(u8, typ_kw, "typ")) {
                continue;
            }

            const component = std.fmt.parseInt(u32, comp_str, 10) catch continue;
            const priority = std.fmt.parseInt(u64, prio_str, 10) catch continue;
            const cand_port = std.fmt.parseInt(u16, port_str, 10) catch continue;

            list.append(allocator, IceCandidate{
                foundation: foundation,
                component: component,
                transport: transport,
                priority: priority,
                address: address,
                port: cand_port,
                cand_type: cand_type,
            }) catch return SdpError.OutOfMemory;
        }

        return list.toOwnedSlice(allocator) catch return SdpError.OutOfMemory;
    }

    /// Returns the media direction attribute (`sendrecv`, `sendonly`, `recvonly`, `inactive`) if present.
    pub fn direction(): ([]const u8)? {
        if (_.hasFlag("sendrecv")) {
            return "sendrecv";
        }
        if (_.hasFlag("sendonly")) {
            return "sendonly";
        }
        if (_.hasFlag("recvonly")) {
            return "recvonly";
        }
        if (_.hasFlag("inactive")) {
            return "inactive";
        }
        return null;
    }
}

type StrSlice = []const u8;

class MediaBuilder {
    media_type: []const u8,
    port: u16,
    proto: []const u8,
    formats: std.ArrayList(StrSlice),
    connection: ([]const u8)?,
    attributes: std.ArrayList(SdpAttribute),

    fn deinit(*_, allocator: std.mem.Allocator) {
        _.formats.deinit(allocator);
        _.attributes.deinit(allocator);
    }
}

/// Represents a parsed RFC 8866 / RFC 4566 SDP Session Description.
pub class SdpSession {
    pub version: u32,
    pub origin: []const u8,
    pub session_name: []const u8,
    pub connection: ([]const u8)?,
    pub timing: []const u8,
    pub attributes: []SdpAttribute,
    pub media: []MediaSection,

    /// Releases all owned slices of this `SdpSession` and its `MediaSection` entries.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.attributes.len > 0) {
            allocator.free(_.attributes);
        }
        let i: usize = 0;
        while (i < _.media.len) : (i += 1) {
            _.media[i].deinit(allocator);
        }
        if (_.media.len > 0) {
            allocator.free(_.media);
        }
        _.attributes = _.attributes[0..0];
        _.media = _.media[0..0];
    }

    /// Returns the value of the first session-level attribute matching `key`, or `null`.
    pub fn getAttr(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            const attr = _.attributes[i];
            if (std.mem.eql(u8, attr.key, key)) {
                return attr.value;
            }
        }
        return null;
    }

    /// Finds the first `MediaSection` matching `media_type` (e.g. `"audio"`, `"video"`, `"application"`).
    pub fn findMedia(media_type: []const u8): (*const MediaSection)? {
        const media = _.media;
        let i: usize = 0;
        while (i < media.len) : (i += 1) {
            if (std.mem.eql(u8, media[i].media_type, media_type)) {
                return &media[i];
            }
        }
        return null;
    }

    /// Parses a raw SDP payload (supporting both `\r\n` and `\n` line endings) into an `SdpSession`.
    pub static fn parse(allocator: std.mem.Allocator, raw_sdp: []const u8): SdpSession ! SdpError {
        let has_version = false;
        let version: u32 = 0;
        let has_origin = false;
        let origin: []const u8 = "";
        let has_session_name = false;
        let session_name: []const u8 = "";
        let connection: ([]const u8)? = null;
        let timing: []const u8 = "";

        let sess_attrs: std.ArrayList(SdpAttribute) = .empty;
        errdefer sess_attrs.deinit(allocator);

        let media_builders: std.ArrayList(MediaBuilder) = .empty;
        errdefer {
            let mi: usize = 0;
            while (mi < media_builders.items.len) : (mi += 1) {
                media_builders.items[mi].deinit(allocator);
            }
            media_builders.deinit(allocator);
        }

        let line_it = std.mem.splitScalar(u8, raw_sdp, '\n');
        while (raw_line := line_it.next()) {
            let line = raw_line;
            if (line.len > 0 && line[line.len - 1] == '\r') {
                line = line[0 .. line.len - 1];
            }
            if (line.len == 0) {
                continue;
            }
            if (line.len < 2 || line[1] != '=') {
                return SdpError.MalformedLine;
            }

            const tag = line[0];
            const val = line[2..];

            switch (tag) {
                'v' => {
                    const v = std.fmt.parseInt(u32, val, 10) catch return SdpError.InvalidVersion;
                    if (v != 0) {
                        return SdpError.InvalidVersion;
                    }
                    version = v;
                    has_version = true;
                },
                'o' => {
                    if (val.len == 0) {
                        return SdpError.MissingOrigin;
                    }
                    origin = val;
                    has_origin = true;
                },
                's' => {
                    session_name = val;
                    has_session_name = true;
                },
                'c' => {
                    if (media_builders.items.len > 0) {
                        media_builders.items[media_builders.items.len - 1].connection = val;
                    } else {
                        connection = val;
                    }
                },
                't' => {
                    timing = val;
                },
                'a' => {
                    let attr_key = val;
                    let attr_val: ([]const u8)? = null;
                    if (colon_idx := std.mem.indexOfScalar(u8, val, ':')) {
                        attr_key = val[0..colon_idx];
                        attr_val = val[colon_idx + 1 ..];
                    }
                    const attr = SdpAttribute{
                        key: attr_key,
                        value: attr_val,
                    };
                    if (media_builders.items.len > 0) {
                        media_builders.items[media_builders.items.len - 1].attributes.append(allocator, attr) catch return SdpError.OutOfMemory;
                    } else {
                        sess_attrs.append(allocator, attr) catch return SdpError.OutOfMemory;
                    }
                },
                'm' => {
                    let tok = std.mem.tokenizeScalar(u8, val, ' ');
                    const media_type = tok.next() orelse return SdpError.InvalidMediaLine;
                    const port_raw = tok.next() orelse return SdpError.InvalidMediaLine;
                    const proto = tok.next() orelse return SdpError.InvalidMediaLine;

                    let port_str = port_raw;
                    if (slash_idx := std.mem.indexOfScalar(u8, port_raw, '/')) {
                        port_str = port_raw[0..slash_idx];
                    }
                    const port = std.fmt.parseInt(u16, port_str, 10) catch return SdpError.InvalidPort;

                    let fmts: std.ArrayList(StrSlice) = .empty;
                    errdefer fmts.deinit(allocator);
                    while (fmt_tok := tok.next()) {
                        fmts.append(allocator, fmt_tok) catch return SdpError.OutOfMemory;
                    }
                    if (fmts.items.len == 0) {
                        return SdpError.InvalidMediaLine;
                    }

                    media_builders.append(allocator, MediaBuilder{
                        media_type: media_type,
                        port: port,
                        proto: proto,
                        formats: fmts,
                        connection: null,
                        attributes: .empty,
                    }) catch return SdpError.OutOfMemory;
                },
                else => {
                    // Other valid RFC 8866 descriptors (i=, u=, e=, p=, b=, z=, k=, r=) are ignored safely.
                },
            }
        }

        if (!has_version) {
            return SdpError.InvalidVersion;
        }
        if (!has_origin) {
            return SdpError.MissingOrigin;
        }
        if (!has_session_name) {
            return SdpError.MissingSessionName;
        }

        let media_sections = allocator.alloc(MediaSection, media_builders.items.len) catch return SdpError.OutOfMemory;
        let built_count: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < built_count) : (k += 1) {
                media_sections[k].deinit(allocator);
            }
            allocator.free(media_sections);
        }

        while (built_count < media_builders.items.len) : (built_count += 1) {
            let mb = &media_builders.items[built_count];
            const fmts_slice = mb.formats.toOwnedSlice(allocator) catch return SdpError.OutOfMemory;
            errdefer allocator.free(fmts_slice);
            const attrs_slice = mb.attributes.toOwnedSlice(allocator) catch return SdpError.OutOfMemory;

            media_sections[built_count] = MediaSection{
                media_type: mb.media_type,
                port: mb.port,
                proto: mb.proto,
                formats: fmts_slice,
                connection: mb.connection,
                attributes: attrs_slice,
            };
        }
        media_builders.deinit(allocator);

        const sess_attrs_slice = sess_attrs.toOwnedSlice(allocator) catch return SdpError.OutOfMemory;

        return SdpSession{
            version: version,
            origin: origin,
            session_name: session_name,
            connection: connection,
            timing: timing,
            attributes: sess_attrs_slice,
            media: media_sections,
        };
    }

    /// Formats this `SdpSession` back into an RFC 8866 compliant SDP string with `\r\n` line endings.
    pub fn format(allocator: std.mem.Allocator): ([]u8) ! SdpError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        let writer = out.writer(allocator);
        writer.print("v={d}\r\n", .{_.version}) catch return SdpError.OutOfMemory;
        writer.print("o={s}\r\n", .{_.origin}) catch return SdpError.OutOfMemory;
        writer.print("s={s}\r\n", .{_.session_name}) catch return SdpError.OutOfMemory;
        if (conn := _.connection) {
            writer.print("c={s}\r\n", .{conn}) catch return SdpError.OutOfMemory;
        }
        if (_.timing.len > 0) {
            writer.print("t={s}\r\n", .{_.timing}) catch return SdpError.OutOfMemory;
        }

        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            const attr = _.attributes[i];
            if (v := attr.value) {
                writer.print("a={s}:{s}\r\n", .{ attr.key, v }) catch return SdpError.OutOfMemory;
            } else {
                writer.print("a={s}\r\n", .{attr.key}) catch return SdpError.OutOfMemory;
            }
        }

        let mi: usize = 0;
        while (mi < _.media.len) : (mi += 1) {
            const m = _.media[mi];
            writer.print("m={s} {d} {s}", .{ m.media_type, m.port, m.proto }) catch return SdpError.OutOfMemory;
            let fi: usize = 0;
            while (fi < m.formats.len) : (fi += 1) {
                writer.print(" {s}", .{m.formats[fi]}) catch return SdpError.OutOfMemory;
            }
            writer.writeAll("\r\n") catch return SdpError.OutOfMemory;

            if (mconn := m.connection) {
                writer.print("c={s}\r\n", .{mconn}) catch return SdpError.OutOfMemory;
            }

            let ai: usize = 0;
            while (ai < m.attributes.len) : (ai += 1) {
                const mattr = m.attributes[ai];
                if (mv := mattr.value) {
                    writer.print("a={s}:{s}\r\n", .{ mattr.key, mv }) catch return SdpError.OutOfMemory;
                } else {
                    writer.print("a={s}\r\n", .{mattr.key}) catch return SdpError.OutOfMemory;
                }
            }
        }

        return out.toOwnedSlice(allocator) catch return SdpError.OutOfMemory;
    }
}
