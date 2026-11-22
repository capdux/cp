// Source: Absorbed from /data/dev/bun/src/interchange/json.cp | Synced: 2026-11-22T12:49:17Z
// Thread-local memory pool for JSON parsing (Ported from Bun.js HashMapPool)

const std = @import("std");

pub struct JsonBufferNode {
    pub data: [4096]u8 = undefined,
    pub next: *JsonBufferNode? = null,
}

pub struct JsonBufferPool {
    zig {
        threadlocal var pool_head: ?*JsonBufferNode = null;
    }

    pub static fn acquire(allocator: std.mem.Allocator): *JsonBufferNode {
        zig {
            if (pool_head) |node| {
                pool_head = node.next;
                node.next = null;
                return node;
            }
            const node = allocator.create(JsonBufferNode) catch unreachable;
            node.next = null;
            return node;
        }
    }

    pub static fn recycle(node: *JsonBufferNode) {
        zig {
            node.next = pool_head;
            pool_head = node;
        }
    }

    pub static fn release(node: *JsonBufferNode) {
        JsonBufferPool.recycle(node);
    }
}
