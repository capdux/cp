// Source: Bun path-not-recorded
/**
 * CP HTTP Header Value Iterator (cap:http)
 * Ported from Bun.js http/HeaderValueIterator.cp
 */


pub const HeaderValueIterator = struct {
    raw: []const u8 = "",
    pos: usize = 0,

    pub fn init(input: []const u8) HeaderValueIterator {
        let it = HeaderValueIterator{};
        it.raw = std.mem.trim(u8, input, " \t");
        it.pos = 0;
        return it;
    }

    pub fn next(*_): ([]const u8)? {
        if (_.pos >= _.raw.len) {
            return null;
        }

        const start = _.pos;
        let end = start;
        while (end < _.raw.len and _.raw[end] != ',') : (end += 1) {}

        const slice = std.mem.trim(u8, _.raw[start..end], " \t");
        _.pos = if (end < _.raw.len) end + 1 else _.raw.len;

        if (slice.len == 0) {
            return _.next();
        }
        return slice;
    }
};
