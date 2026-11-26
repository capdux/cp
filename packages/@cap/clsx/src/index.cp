// @cap/clsx — Zero-heap conditional CSS class name builder & deduplicator
// MIT License (clean-room CP implementation)

pub struct CondClass {
    pub name: []const u8,
    pub enabled: bool,
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8 || c == '\n' as u8 || c == '\r' as u8;
}

fn bytesEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct ClassBuilder {
    pub buf: []u8,
    pub pos: usize,
    pub overflow: bool,

    pub static fn init(buf: []u8) -> ClassBuilder {
        return ClassBuilder {
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.overflow = false;
    }

    pub fn has(token: []const u8) -> bool {
        if (token.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.pos) {
            while (i < _.pos && _.buf[i] == ' ' as u8) {
                i += 1;
            }
            const start: usize = i;
            while (i < _.pos && _.buf[i] != ' ' as u8) {
                i += 1;
            }
            if (i > start && bytesEq(_.buf[start..i], token)) {
                return true;
            }
        }
        return false;
    }

    fn appendToken(*_, tok: []const u8) -> bool {
        if (_.overflow) {
            return false;
        }
        let needed: usize = tok.len;
        if (_.pos > 0) {
            needed += 1;
        }
        if (needed > _.buf.len - _.pos) {
            _.overflow = true;
            return false;
        }
        if (_.pos > 0) {
            _.buf[_.pos] = ' ' as u8;
            _.pos += 1;
        }
        let k: usize = 0;
        while (k < tok.len) {
            _.buf[_.pos] = tok[k];
            _.pos += 1;
            k += 1;
        }
        return true;
    }

    pub fn add(*_, cls: []const u8) -> bool {
        if (_.overflow) {
            return false;
        }
        let i: usize = 0;
        while (i < cls.len) {
            while (i < cls.len && isWs(cls[i])) {
                i += 1;
            }
            const start: usize = i;
            while (i < cls.len && !isWs(cls[i])) {
                i += 1;
            }
            if (i > start) {
                if (!_.appendToken(cls[start..i])) {
                    return false;
                }
            }
        }
        return true;
    }

    pub fn addIf(*_, cond: bool, cls: []const u8) -> bool {
        if (!cond) {
            return !_.overflow;
        }
        return _.add(cls);
    }

    pub fn addUnique(*_, cls: []const u8) -> bool {
        if (_.overflow) {
            return false;
        }
        let i: usize = 0;
        while (i < cls.len) {
            while (i < cls.len && isWs(cls[i])) {
                i += 1;
            }
            const start: usize = i;
            while (i < cls.len && !isWs(cls[i])) {
                i += 1;
            }
            if (i > start) {
                const tok: []const u8 = cls[start..i];
                if (!_.has(tok)) {
                    if (!_.appendToken(tok)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    pub fn addConds(*_, items: []const CondClass) -> bool {
        if (_.overflow) {
            return false;
        }
        let i: usize = 0;
        while (i < items.len) {
            if (items[i].enabled) {
                if (!_.add(items[i].name)) {
                    return false;
                }
            }
            i += 1;
        }
        return true;
    }

    pub fn build() -> []const u8 {
        return _.buf[0.._.pos];
    }

    pub fn len() -> usize {
        return _.pos;
    }

    pub fn isOverflow() -> bool {
        return _.overflow;
    }
}

pub fn joinClasses(classes: []const []const u8, dst: []u8) -> i64 {
    let b: ClassBuilder = ClassBuilder.init(dst);
    let i: usize = 0;
    while (i < classes.len) {
        if (!b.add(classes[i])) {
            return -1;
        }
        i += 1;
    }
    return b.len() as i64;
}

pub fn joinCondClasses(items: []const CondClass, dst: []u8) -> i64 {
    let b: ClassBuilder = ClassBuilder.init(dst);
    if (!b.addConds(items)) {
        return -1;
    }
    return b.len() as i64;
}
