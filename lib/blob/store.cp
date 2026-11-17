// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/runtime/webcore/blob/Store.cp
// Upstream Sync: Refer to bun/src/runtime/webcore/blob/Store.zig for future upstream updates.
// ============================================================================

pub enum BlobKind: u8 {
    Bytes = 0,
    File = 1,
}

pub struct BlobStore {
    pub kind: BlobKind,
    pub bytes: []const u8,
    pub path: string,
    pub ref_count: usize,

    pub static fn fromBytes(data: []const u8): *BlobStore {
        let raw: [*]u8 = @alloc(@sizeOf(BlobStore));
        let ptr: *BlobStore = @ptrCast(@alignCast(raw));
        ptr.kind = BlobKind.Bytes;
        ptr.bytes = data;
        ptr.path = "";
        ptr.ref_count = 1;
        return ptr;
    }

    pub static fn createFromBytes(data: []const u8): *BlobStore {
        return BlobStore.fromBytes(data);
    }

    pub static fn fromFile(file_path: string, cached_data: []const u8): *BlobStore {
        let raw: [*]u8 = @alloc(@sizeOf(BlobStore));
        let ptr: *BlobStore = @ptrCast(@alignCast(raw));
        ptr.kind = BlobKind.File;
        ptr.bytes = cached_data;
        ptr.path = file_path;
        ptr.ref_count = 1;
        return ptr;
    }

    pub static fn createFromFile(file_path: string, cached_data: []const u8): *BlobStore {
        return BlobStore.fromFile(file_path, cached_data);
    }

    pub fn ref(*_) {
        _.ref_count = _.ref_count + 1;
    }

    pub fn deref(*_) {
        if (_.ref_count > 0) {
            _.ref_count = _.ref_count - 1;
        }
    }
}
