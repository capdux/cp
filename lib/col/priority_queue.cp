// Source: Zig Standard Library: std.PriorityQueue (wrapped by Antigravity AI)
// lib/col/priority_queue.cp
// Generic min-heap Priority Queue (smallest element popped first)

const std = @import("std");

pub struct PriorityQueue<T> {
    items: std.ArrayList(T),

    pub static fn init(): PriorityQueue<T> {
        zig {
            return @This(){ .items = .empty };
        }
    }

    pub fn deinit(*_) {
        zig { self.items.deinit(std.heap.page_allocator); }
    }

    pub fn add(*_, item: T): !void {
        zig {
            try self.items.append(std.heap.page_allocator, item);
            var i = self.items.items.len - 1;
            while (i > 0) {
                const p = (i - 1) / 2;
                if (self.items.items[i] < self.items.items[p]) {
                    std.mem.swap(T, &self.items.items[i], &self.items.items[p]);
                    i = p;
                } else break;
            }
        }
    }

    pub fn pop(*_): T? {
        zig {
            const n = self.items.items.len;
            if (n == 0) return null;
            const top = self.items.items[0];
            self.items.items[0] = self.items.items[n - 1];
            self.items.items.len = n - 1;
            const m = n - 1;
            var i: usize = 0;
            while (true) {
                const l = 2 * i + 1;
                const r = l + 1;
                var s = i;
                if (l < m and self.items.items[l] < self.items.items[s]) s = l;
                if (r < m and self.items.items[r] < self.items.items[s]) s = r;
                if (s == i) break;
                std.mem.swap(T, &self.items.items[i], &self.items.items[s]);
                i = s;
            }
            return top;
        }
    }

    pub fn len(*_): usize {
        zig { return self.items.items.len; }
    }
}
