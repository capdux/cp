// Atomic reference-counted slice in CP
// Enables zero-copy, lock-free, immutable data sharing across threads and actors

pub fn ArcSlice(comptime T: type): type {
    const Header = struct {
        ref_count: std.atomic.Value(usize),
        allocator: std.mem.Allocator,
    };

    return struct {
        const Self = @This();

        items: []T = [],
        header: (*Header)? = null,

        pub fn init(allocator: std.mem.Allocator, data: []const T): !Self {
            let hdr = try allocator.create(Header);
            errdefer allocator.destroy(hdr);

            const items = try allocator.alloc(T, data.len);
            errdefer allocator.free(items);

            hdr.* = .{
                .ref_count = std.atomic.Value(usize).init(1),
                .allocator = allocator,
            };
            @memcpy(items, data);

            return Self{
                .items = items,
                .header = hdr,
            };
        }

        pub fn clone(*_): Self {
            if (hdr := _.header) {
                * = hdr.ref_count.fetchAdd(1, .monotonic);
            }
            return Self{
                .items = _.items,
                .header = _.header,
            };
        }

        pub fn release(*_) {
            if (hdr := _.header) {
                _.header = null;
                if (hdr.ref_count.fetchSub(1, .acq_rel) == 1) {
                    hdr.allocator.free(_.items);
                    hdr.allocator.destroy(hdr);
                }
            }
        }

        pub fn deinit(*_) {
            _.release();
        }

        pub fn slice(*_): []const T {
            return _.items;
        }

        pub fn len(*_): usize {
            return _.items.len;
        }

        pub fn refCount(*_): usize {
            if (hdr := _.header) {
                return hdr.ref_count.load(.monotonic);
            }
            return 0;
        }

        pub fn get(*_, index: usize): T? {
            if (index >= _.items.len) return null;
            return _.items[index];
        }
    };
}
