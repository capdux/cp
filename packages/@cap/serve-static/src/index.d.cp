// Type definitions for CP

import { parseRequestUrl, isSafeRequestPath } from "../../parseurl/src/index.cp";
import { lookup, contentType } from "../../mime-types/src/index.cp";
import { generateStatEtag } from "../../etag-http/src/index.cp";
import { formatU64 } from "../../itoa/src/index.cp";
export class ServeStaticOptions {
    index_file: string,
    dotfiles: u8,
    max_age_secs: u64,
    immutable: bool,
    redirect_dirs: bool,
    fn defaultOpts(): ServeStaticOptions;
}

export class StaticResolution {
    status: u16,
    rel_path_len: usize,
    mime_type: string,
}

export fn sstPush(out: []u8, pos: usize, s: string): usize;
export fn sstIsGetOrHead(method: string): bool;
export fn hasDotfileSegment(pathname: string): bool;
export fn resolveStaticRequest(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution;
export fn formatCacheControl(max_age_secs: u64, immutable: bool, out: []u8): usize;
export fn formatStaticEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize;
export fn formatStaticContentType(file_path: string, out: []u8): usize;
