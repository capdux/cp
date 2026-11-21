// Source: Absorbed from /data/dev/bun/src/ptr/tagged_pointer.cp | Synced: 2026-11-21T12:50:27Z

pub const PTR_BITS: u6 = 49;
pub const TAG_BITS: u6 = 15;
pub const PTR_MASK: u64 = (@as(u64, 1) << 49) - 1;
pub const MAX_TAG: u15 = 0x7FFF;

pub type Tag = u15;
pub type Address = u49;

/// 64 位高位标签指针（低 49 位指针地址 + 高 15 位无符号标签，总大小严格 8 字节）
pub struct TaggedPointer {
    raw: u64 = 0,

    /// 构造空标签指针（地址 0，标签 0）
    static fn empty(): TaggedPointer {
        return TaggedPointer{ raw: 0 };
    }

    /// 构造空指针但携带指定 15 位标签
    static fn nullWithTag(tag_val: u15): TaggedPointer {
        return TaggedPointer{
            raw: (tag_val as u64) << PTR_BITS,
        };
    }

    /// 从整数地址与 15 位标签构造
    static fn fromAddress(addr: usize, tag_val: u15): TaggedPointer {
        const masked_addr: u64 = (addr as u64) & PTR_MASK;
        const shifted_tag: u64 = (tag_val as u64) << PTR_BITS;
        return TaggedPointer{
            raw: masked_addr | shifted_tag,
        };
    }

    /// 从类型化裸指针与 15 位标签构造
    static fn fromPtr(comptime T: type, ptr: *T!, tag_val: u15): TaggedPointer {
        let addr: usize = 0;
        unsafe {
            addr = ptr as usize;
        }
        return TaggedPointer.fromAddress(addr, tag_val);
    }

    /// 从只读类型化裸指针与 15 位标签构造
    static fn fromConstPtr(comptime T: type, ptr: *const T!, tag_val: u15): TaggedPointer {
        let addr: usize = 0;
        unsafe {
            addr = ptr as usize;
        }
        return TaggedPointer.fromAddress(addr, tag_val);
    }

    /// 从原始 64 位无符号整数位模式还原
    static fn fromU64(raw_val: u64): TaggedPointer {
        return TaggedPointer{ raw: raw_val };
    }

    /// 导出底层 64 位无符号整数位模式
    fn toU64(): u64 {
        return _.raw;
    }

    /// 提取低 49 位指针地址
    fn address(): usize {
        return (_.raw & PTR_MASK) as usize;
    }

    /// 提取高 15 位标签值
    fn tag(): u15 {
        return ((_.raw >> PTR_BITS) & 0x7FFF) as u15;
    }

    /// 判断底层指针地址是否为 null (0)
    fn isNull(): bool {
        return (_.raw & PTR_MASK) == 0;
    }

    /// 原地修改高 15 位标签，完整保留低 49 位地址
    fn setTag(*_, new_tag: u15) {
        _.raw = (_.raw & PTR_MASK) | ((new_tag as u64) << PTR_BITS);
    }

    /// 返回替换高 15 位标签后的新 TaggedPointer
    fn withTag(new_tag: u15): TaggedPointer {
        return TaggedPointer{
            raw: (_.raw & PTR_MASK) | ((new_tag as u64) << PTR_BITS),
        };
    }

    /// 原地修改低 49 位地址，完整保留高 15 位标签
    fn setAddress(*_, new_addr: usize) {
        const tag_bits = _.raw & ~PTR_MASK;
        _.raw = tag_bits | ((new_addr as u64) & PTR_MASK);
    }

    /// 还原为指定类型的裸指针；地址为 0 时安全返回 null
    fn getPtr(comptime T: type): (*T!)? {
        const addr = _.address();
        if (addr == 0) {
            return null;
        }
        unsafe {
            return addr as *T!;
        }
    }

    /// 还原为指定类型的只读裸指针；地址为 0 时安全返回 null
    fn getConstPtr(comptime T: type): (*const T!)? {
        const addr = _.address();
        if (addr == 0) {
            return null;
        }
        unsafe {
            return addr as *const T!;
        }
    }

    /// 按 64 位原始位模式比较相等
    fn eql(other: TaggedPointer): bool {
        return _.raw == other.raw;
    }
}

/// 8 字节双类型安全标签指针联合体（Tag: 0=Empty, 1=A, 2=B）
pub struct TaggedPtr2<A, B> {
    repr: TaggedPointer = TaggedPointer.empty(),

    static fn empty(): TaggedPtr2<A, B> {
        return TaggedPtr2<A, B>{ repr: TaggedPointer.empty() };
    }

    static fn fromA(ptr: *A!): TaggedPtr2<A, B> {
        return TaggedPtr2<A, B>{ repr: TaggedPointer.fromPtr(A, ptr, 1) };
    }

    static fn fromB(ptr: *B!): TaggedPtr2<A, B> {
        return TaggedPtr2<A, B>{ repr: TaggedPointer.fromPtr(B, ptr, 2) };
    }

    static fn fromU64(raw_val: u64): TaggedPtr2<A, B> {
        return TaggedPtr2<A, B>{ repr: TaggedPointer.fromU64(raw_val) };
    }

    fn toU64(): u64 {
        return _.repr.toU64();
    }

    fn tag(): u15 {
        return _.repr.tag();
    }

    fn isEmpty(): bool {
        return _.repr.tag() == 0 || _.repr.isNull();
    }

    fn isA(): bool {
        return _.repr.tag() == 1 && !_.repr.isNull();
    }

    fn isB(): bool {
        return _.repr.tag() == 2 && !_.repr.isNull();
    }

    fn asA(): (*A!)? {
        if (!_.isA()) {
            return null;
        }
        return _.repr.getPtr(A);
    }

    fn asB(): (*B!)? {
        if (!_.isB()) {
            return null;
        }
        return _.repr.getPtr(B);
    }

    fn setA(*_, ptr: *A!) {
        _.repr = TaggedPointer.fromPtr(A, ptr, 1);
    }

    fn setB(*_, ptr: *B!) {
        _.repr = TaggedPointer.fromPtr(B, ptr, 2);
    }

    fn clear(*_) {
        _.repr = TaggedPointer.empty();
    }
}

/// 8 字节四类型安全标签指针联合体（Tag: 0=Empty, 1=A, 2=B, 3=C, 4=D）
pub struct TaggedPtr4<A, B, C, D> {
    repr: TaggedPointer = TaggedPointer.empty(),

    static fn empty(): TaggedPtr4<A, B, C, D> {
        return TaggedPtr4<A, B, C, D>{ repr: TaggedPointer.empty() };
    }

    static fn fromA(ptr: *A!): TaggedPtr4<A, B, C, D> {
        return TaggedPtr4<A, B, C, D>{ repr: TaggedPointer.fromPtr(A, ptr, 1) };
    }

    static fn fromB(ptr: *B!): TaggedPtr4<A, B, C, D> {
        return TaggedPtr4<A, B, C, D>{ repr: TaggedPointer.fromPtr(B, ptr, 2) };
    }

    static fn fromC(ptr: *C!): TaggedPtr4<A, B, C, D> {
        return TaggedPtr4<A, B, C, D>{ repr: TaggedPointer.fromPtr(C, ptr, 3) };
    }

    static fn fromD(ptr: *D!): TaggedPtr4<A, B, C, D> {
        return TaggedPtr4<A, B, C, D>{ repr: TaggedPointer.fromPtr(D, ptr, 4) };
    }

    static fn fromU64(raw_val: u64): TaggedPtr4<A, B, C, D> {
        return TaggedPtr4<A, B, C, D>{ repr: TaggedPointer.fromU64(raw_val) };
    }

    fn toU64(): u64 {
        return _.repr.toU64();
    }

    fn tag(): u15 {
        return _.repr.tag();
    }

    fn isEmpty(): bool {
        return _.repr.tag() == 0 || _.repr.isNull();
    }

    fn isA(): bool {
        return _.repr.tag() == 1 && !_.repr.isNull();
    }

    fn isB(): bool {
        return _.repr.tag() == 2 && !_.repr.isNull();
    }

    fn isC(): bool {
        return _.repr.tag() == 3 && !_.repr.isNull();
    }

    fn isD(): bool {
        return _.repr.tag() == 4 && !_.repr.isNull();
    }

    fn asA(): (*A!)? {
        if (!_.isA()) {
            return null;
        }
        return _.repr.getPtr(A);
    }

    fn asB(): (*B!)? {
        if (!_.isB()) {
            return null;
        }
        return _.repr.getPtr(B);
    }

    fn asC(): (*C!)? {
        if (!_.isC()) {
            return null;
        }
        return _.repr.getPtr(C);
    }

    fn asD(): (*D!)? {
        if (!_.isD()) {
            return null;
        }
        return _.repr.getPtr(D);
    }

    fn setA(*_, ptr: *A!) {
        _.repr = TaggedPointer.fromPtr(A, ptr, 1);
    }

    fn setB(*_, ptr: *B!) {
        _.repr = TaggedPointer.fromPtr(B, ptr, 2);
    }

    fn setC(*_, ptr: *C!) {
        _.repr = TaggedPointer.fromPtr(C, ptr, 3);
    }

    fn setD(*_, ptr: *D!) {
        _.repr = TaggedPointer.fromPtr(D, ptr, 4);
    }

    fn clear(*_) {
        _.repr = TaggedPointer.empty();
    }
}
