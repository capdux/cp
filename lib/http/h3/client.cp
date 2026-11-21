// Source: Absorbed from /data/dev/bun/src/http/h3_client/ClientSession.cp | Synced: 2026-11-20T08:56:59Z
// lib/http/h3/client.cp
// CP HTTP/3 Client Session & Stream Manager (cap:http)

pub enum H3StreamType {
    Control,
    QpackEncoder,
    QpackDecoder,
    RequestBidi,
}

pub struct H3Stream {
    pub id: u64 = 0;
    pub stream_type: H3StreamType = H3StreamType.RequestBidi;
    pub closed: bool = false;

    pub fn close(*_) {
        _.closed = true;
    }

    pub fn isOpen(): bool {
        return !_.closed;
    }
}

pub struct H3ClientSession {
    pub next_bidi_stream_id: u64 = 0; // Client-initiated bidi stream IDs (0, 4, 8, 12...)
    pub connected: bool = false;
    pub active_streams: usize = 0;

    pub fn newRequestStream(*_): H3Stream {
        const sid = _.next_bidi_stream_id;
        _.next_bidi_stream_id += 4;
        _.active_streams += 1;
        return H3Stream{
            id: sid,
            stream_type: H3StreamType.RequestBidi,
            closed: false,
        };
    }
}
