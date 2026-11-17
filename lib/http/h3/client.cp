// Source: Bun path-not-recorded
/**
 * CP HTTP/3 Client Session & Stream Manager (cap:http)
 * Ported from Bun.js http/H3Client.cp & http/h3_client/
 */

pub const H3StreamType = enum {
    Control,
    QpackEncoder,
    QpackDecoder,
    RequestBidi,
};

pub const H3Stream = struct {
    id: u64 = 0,
    stream_type: H3StreamType = H3StreamType.RequestBidi,
    closed: bool = false,

    pub fn close(*_) {
        _.closed = true;
    }

    pub fn isOpen(*_): bool {
        return !_.closed;
    }
};

pub const H3ClientSession = struct {
    next_bidi_stream_id: u64 = 0, // Client-initiated bidi stream IDs (0, 4, 8, 12...)
    connected: bool = false,
    active_streams: usize = 0,

    pub fn newRequestStream(*_) H3Stream {
        const sid = _.next_bidi_stream_id;
        _.next_bidi_stream_id += 4;
        _.active_streams += 1;
        let s = H3Stream{};
        s.id = sid;
        s.stream_type = .RequestBidi;
        s.closed = false;
        return s;
    }
};
