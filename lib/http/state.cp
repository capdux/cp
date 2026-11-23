// Source: Absorbed from /data/dev/bun/src/http/InternalState.cp | Synced: 2026-11-23T01:58:52Z
// lib/http/internal_state.cp
// CP HTTP Internal State & Protocols (cap:http)

import { Encoding } from "./encoding";
import { CertificateInfo } from "./cert";

pub enum HTTPVerboseLevel {
    None,
    Headers,
    Curl,
}

pub enum HTTPUpgradeState: u2 {
    None = 0,
    Pending = 1,
    Upgraded = 2,
}

pub enum Protocol: u2 {
    Http1_1 = 0,
    Http2 = 1,
    Http3 = 2,
}

pub enum ShouldContinue {
    ContinueStreaming,
    Finished,
}

pub enum HTTPStage {
    Pending,
    Opened,
    Headers,
    Body,
    BodyChunk,
    Fail,
    Done,
    ProxyHandshake,
    ProxyHeaders,
    ProxyBody,
}

pub enum Stage: u8 {
    Pending,
    Connect,
    Done,
    Fail,
}

pub struct Flags {
    pub disable_timeout: bool = false;
    pub disable_keepalive: bool = false;
    pub disable_decompression: bool = false;
    pub did_have_handshaking_error: bool = false;
    pub force_last_modified: bool = false;
    pub redirected: bool = false;
    pub proxy_tunneling: bool = false;
    pub reject_unauthorized: bool = true;
    pub is_preconnect_only: bool = false;
    pub is_streaming_request_body: bool = false;
    pub defer_fail_until_connecting_is_complete: bool = false;
    pub upgrade_state: HTTPUpgradeState = HTTPUpgradeState.None;
    pub protocol: Protocol = Protocol.Http1_1;
    pub force_http2: bool = false;
    pub force_http1: bool = false;
    pub force_http3: bool = false;
    pub h3_retried: bool = false;
}

pub struct InternalStateFlags {
    pub allow_keepalive: bool = true;
    pub received_last_chunk: bool = false;
    pub did_set_content_encoding: bool = false;
    pub is_redirect_pending: bool = false;
    pub is_libdeflate_fast_path_disabled: bool = false;
    pub resend_request_body_on_redirect: bool = false;
}

pub struct InternalState {
    pub flags: InternalStateFlags = InternalStateFlags{};
    pub transfer_encoding: Encoding = Encoding.Identity;
    pub encoding: Encoding = Encoding.Identity;
    pub stage: Stage = Stage.Pending;
    pub request_stage: HTTPStage = HTTPStage.Pending;
    pub response_stage: HTTPStage = HTTPStage.Pending;
    pub content_length: usize? = null;
    pub total_body_received: usize = 0;
    pub request_body: []const u8 = "";
    pub request_sent_len: usize = 0;
    pub certificate_info: CertificateInfo? = null;

    pub fn isChunkedEncoding(): bool {
        return _.transfer_encoding == Encoding.Chunked;
    }

    pub fn isDone(): bool {
        if (_.isChunkedEncoding()) {
            return _.flags.received_last_chunk;
        }
        if (cl := _.content_length) {
            return _.total_body_received >= cl;
        }
        return _.flags.received_last_chunk;
    }
}
