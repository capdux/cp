// Type definitions for CP

import { encodeUrl } from "../../encodeurl/src/index.cp";
import { generateStatEtag, compareWeak } from "../../etag-http/src/index.cp";
import { RANGE_OK, parseRange, rangeByteLength } from "../../range-parser/src/index.cp";
import { lookup } from "../../mime-types/src/index.cp";
export fn sndStrEq(a: string, b: string): bool;
export fn sndIsWs(b: u8): bool;
export fn sndTrimWs(s: string): string;
export fn sndHasMaliciousPath(req_path: string): bool;
export fn sndContainsDotfile(req_path: string): bool;
export fn sndIfNoneMatchMatches(if_none_match: string, current_etag: string): bool;
export class SendOptions {
    root: string,
    dotfiles: u8,
    etag: bool,
    accept_ranges: bool,
    max_age_secs: u64,
    immutable: bool,
    fn defaults(root: string): SendOptions;
}

export class SendPlan {
    status: u16,
    content_type: string,
    is_partial: bool,
    range_start: u64,
    range_end: u64,
    content_length: u64,
    redirect_location_len: usize,
    fn errorPlan(status: u16): SendPlan;
}

export fn planSendFile(req_path: string, file_size: u64, mtime_ms: u64, range_header: string, if_none_match: string, opts: SendOptions, etag_out: []u8): SendPlan;
export fn formatRedirectUrl(path: string, out: []u8): usize;
