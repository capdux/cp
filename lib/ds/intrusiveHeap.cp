// Source: Absorbed from /data/dev/bun/src/io/heap.cp | Synced: 2026-11-24T07:05:00Z
// Zero-allocation intrusive pairing heap with O(1) amortized insert/meld and O(log N) deleteMin/remove.

/// Intrusive linkage field to embed as `heap: IntrusiveField(Self) = .{}` inside node structs.
pub fn IntrusiveField(comptime T: type): type {
    return struct {
        child: (*T!)? = null,
        prev: (*T!)? = null,
        next: (*T!)? = null,

        static fn empty(): @This() {
            return .{};
        }
    };
}

/// Intrusive min-pairing-heap over nodes of type `T` that contain a field `heap: IntrusiveField(T)`.
/// `less` is a function `fn (ctx: Context, a: *const T, b: *const T) bool` returning true if `a < b`.
pub fn IntrusiveHeap(
    comptime T: type,
    comptime Context: type,
    comptime less: auto,
): type {
    return struct {
        const Self = @This();

        root: (*T!)? = null,
        context: Context,

        static fn init(ctx: Context): Self {
            return Self{ root: null, context: ctx };
        }

        fn isEmpty(): bool {
            return _.root == null;
        }

        fn peek(): (*T!)? {
            return _.root;
        }

        fn count(): usize {
            return Self.countInternal(_.root);
        }

        static fn countInternal(node: (*T!)?): usize {
            if (cur := node) {
                let total: usize = 1;
                total += Self.countInternal(cur.heap.child);
                total += Self.countInternal(cur.heap.next);
                return total;
            }
            return 0;
        }

        fn findMax(): (*T!)? {
            if (r := _.root) {
                return Self.findMaxInternal(_.context, r, r);
            }
            return null;
        }

        static fn findMaxInternal(ctx: Context, node: *T!, current_max: *T!): *T! {
            let max_so_far = current_max;
            if (less(ctx, max_so_far, node)) {
                max_so_far = node;
            }
            if (child := node.heap.child) {
                max_so_far = Self.findMaxInternal(ctx, child, max_so_far);
            }
            if (next_sib := node.heap.next) {
                max_so_far = Self.findMaxInternal(ctx, next_sib, max_so_far);
            }
            return max_so_far;
        }

        fn insert(*_, v: *T!) {
            v.heap = IntrusiveField(T).empty();
            if (r := _.root) {
                _.root = _.meld(v, r);
            } else {
                _.root = v;
            }
        }

        fn deleteMin(*_): (*T!)? {
            if (r := _.root) {
                if (child := r.heap.child) {
                    _.root = _.combineSiblings(child);
                } else {
                    _.root = null;
                }
                r.heap = IntrusiveField(T).empty();
                return r;
            }
            return null;
        }

        fn remove(*_, v: *T!) {
            if (prev := v.heap.prev) {
                if (nxt := v.heap.next) {
                    nxt.heap.prev = prev;
                }
                if (prev.heap.child == v) {
                    prev.heap.child = v.heap.next;
                } else {
                    prev.heap.next = v.heap.next;
                }
                v.heap.prev = null;
                v.heap.next = null;

                if (child := v.heap.child) {
                    v.heap.child = null;
                    const merged = _.combineSiblings(child);
                    if (r := _.root) {
                        _.root = _.meld(merged, r);
                    } else {
                        _.root = merged;
                    }
                }
            } else {
                std.debug.assert(_.root.? == v);
                * = _.deleteMin();
            }
        }

        fn meld(*_, a: *T!, b: *T!): *T! {
            std.debug.assert(a.heap.next == null);

            if (less(_.context, a, b)) {
                b.heap.prev = a;
                if (b_next := b.heap.next) {
                    a.heap.next = b_next;
                    b_next.heap.prev = a;
                    b.heap.next = null;
                }
                if (a_child := a.heap.child) {
                    b.heap.next = a_child;
                    a_child.heap.prev = b;
                }
                a.heap.child = b;
                return a;
            }

            b.heap.prev = a.heap.prev;
            a.heap.prev = b;
            if (b_child := b.heap.child) {
                a.heap.next = b_child;
                b_child.heap.prev = a;
            }
            b.heap.child = a;
            return b;
        }

        fn combineSiblings(*_, left: *T!): *T! {
            left.heap.prev = null;
            let a: *T! = left;
            let root: (*T!)? = null;
            while (true) {
                if (b_opt := a.heap.next) {
                    a.heap.next = null;
                    const merged = _.meld(a, b_opt);
                    if (next_a := merged.heap.next) {
                        a = next_a;
                    } else {
                        root = merged;
                        break;
                    }
                } else {
                    root = a;
                    break;
                }
            }

            let cur: *T! = root.?;
            while (true) {
                if (b_prev := cur.heap.prev) {
                    b_prev.heap.next = null;
                    cur = _.meld(b_prev, cur);
                } else {
                    return cur;
                }
            }
        }
    };
}
