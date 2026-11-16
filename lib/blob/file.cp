// ============================================================================
// Absorbed from Bun: bun/src/runtime/webcore/Blob.cp
// Upstream Sync: Refer to bun/src/runtime/webcore/Blob.zig for future upstream updates.
// ============================================================================

import { Blob } from "./blob";
import { writeFileSync, readFileSync } from "cap:fs";

pub struct File {
    pub blob: Blob,
    pub name: string,
    pub lastModified: i64,
    pub size: usize,
    pub type: string,

    pub static fn init(data: []const u8, name: string, mime_type: string, last_modified: i64): File {
        let b = Blob.fromBytes(data, mime_type);
        return File{
            blob: b,
            name: name,
            lastModified: last_modified,
            size: b.size,
            type: b.type,
        };
    }

    pub static fn fromPath(path: string, mime_type: string): File? {
        let b = Blob.fromFile(path, mime_type);
        if (b != null) {
            let blob_val = b.?;
            let filename = File.extractFilename(path);
            return File{
                blob: blob_val,
                name: filename,
                lastModified: 0,
                size: blob_val.size,
                type: blob_val.type,
            };
        }
        return null;
    }

    pub fn bytes(): []const u8 {
        return _.blob.bytes();
    }

    pub fn text(): string {
        return _.blob.text();
    }

    pub fn slice(start: usize, end: usize, content_type: string): File {
        let sliced_blob = _.blob.slice(start, end, content_type);
        return File{
            blob: sliced_blob,
            name: _.name,
            lastModified: _.lastModified,
            size: sliced_blob.size,
            type: sliced_blob.type,
        };
    }

    pub fn save(dest_path: string): bool {
        let raw = _.bytes();
        writeFileSync(dest_path, raw);
        return true;
    }

    pub fn writeTo(dest_path: string): bool {
        return _.save(dest_path);
    }

    pub fn release() {
        _.blob.release();
    }

    pub fn deinit(*_) {
        _.release();
    }

    static fn extractFilename(path: string): string {
        if (path.len == 0) {
            return "";
        }
        let mut_idx = path.len;
        while (mut_idx > 0) {
            mut_idx = mut_idx - 1;
            let ch = path[mut_idx];
            if (ch == '/' || ch == '\\') {
                return path[(mut_idx + 1)..];
            }
        }
        return path;
    }
}
