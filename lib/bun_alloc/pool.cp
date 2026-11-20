// Source: cp/lib/bun_alloc/pool.cp (fixed-size O(1) object pool allocator)

import std from "std";

const Allocator = std.mem.Allocator;

pub static fn PoolAllocator(comptime T: type): type {
    return struct {
        const Self = @This();

        const Node = struct {
            data: T,
            next: (*Node)?,
        };
        const NodeSlice = []Node;

        backing_allocator: Allocator,
        chunks: std.ArrayList(NodeSlice),
        free_list: (*Node)?,
        active_count: usize,
        total_capacity: usize,
        chunk_size: usize,
        max_objects: usize,

        pub static fn init(allocator: Allocator): Self {
            return Self.initWithOptions(allocator, 32, 0);
        }

        pub static fn initWithOptions(allocator: Allocator, chunk_size: usize, max_objects: usize): Self {
            const effective_chunk = if (chunk_size == 0) 1 else chunk_size;
            return Self{
                backing_allocator: allocator,
                chunks: std.ArrayList(NodeSlice).empty,
                free_list: null,
                active_count: 0,
                total_capacity: 0,
                chunk_size: effective_chunk,
                max_objects: max_objects,
            };
        }

        pub fn deinit(*_) {
            for (chunk of _.chunks.items) {
                _.backing_allocator.free(chunk);
            }
            _.chunks.deinit(_.backing_allocator);
            _.free_list = null;
            _.active_count = 0;
            _.total_capacity = 0;
        }

        fn growChunk(*_, count: usize): !void {
            if (count == 0) {
                return;
            }
            if (_.max_objects > 0 and _.total_capacity >= _.max_objects) {
                return error.OutOfMemory;
            }
            let alloc_count = count;
            if (_.max_objects > 0) {
                const avail = _.max_objects - _.total_capacity;
                if (alloc_count > avail) {
                    alloc_count = avail;
                }
            }
            let chunk = _.backing_allocator.alloc(Node, alloc_count)?;
            errdefer _.backing_allocator.free(chunk);
            _.chunks.append(_.backing_allocator, chunk)?;

            let i: usize = alloc_count;
            while (i > 0) {
                i -= 1;
                chunk[i].next = _.free_list;
                _.free_list = &chunk[i];
            }
            _.total_capacity += alloc_count;
        }

        pub fn prewarm(*_, count: usize): !void {
            if (count <= _.total_capacity) {
                return;
            }
            const needed = count - _.total_capacity;
            _.growChunk(needed)?;
        }

        pub fn create(*_): !*T {
            if (_.free_list == null) {
                _.growChunk(_.chunk_size)?;
            }
            let node = _.free_list orelse return error.OutOfMemory;
            _.free_list = node.next;
            node.next = null;
            _.active_count += 1;
            return &node.data;
        }

        pub fn recycle(*_, ptr: *T) {
            let node: *Node = unsafe { @fieldParentPtr("data", ptr) };
            node.next = _.free_list;
            _.free_list = node;
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
        }

        pub fn activeCount(): usize {
            return _.active_count;
        }

        pub fn capacity(): usize {
            return _.total_capacity;
        }
    };
}
