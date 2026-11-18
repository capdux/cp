// Source: bun/src/ptr/tagged_pointer.cp
// High-bit and low-bit tagged pointers and zero-virtual-table union abstractions
import std from "std";

pub const TAG_SHIFT: u6 = 48;
pub const ADDR_MASK: u64 = 0x0000FFFFFFFFFFFF;
pub const TAG_MASK: u64 = (0xFFFF as u64) << TAG_SHIFT;
pub const LOW_TAG_MASK: usize = 0x7;

pub struct TaggedPointer {
    raw: u64 = 0,

    pub static fn pack(addr: usize, tag_val: u16): TaggedPointer {
        const masked_addr = (addr as u64) & ADDR_MASK;
        const shifted_tag = (tag_val as u64) << TAG_SHIFT;
        return TaggedPointer{
            raw: masked_addr | shifted_tag,
        };
    }

    pub static fn fromPtr(ptr: auto, tag_val: u16): TaggedPointer {
        const PtrType = @TypeOf(ptr);
        if (PtrType == @TypeOf(null)) {
            return TaggedPointer.pack(0, tag_val);
        }
        let addr: usize = 0;
        unsafe {
            addr = @intFromPtr(ptr);
        }
        return TaggedPointer.pack(addr, tag_val);
    }

    pub static fn fromBits(raw_bits: u64): TaggedPointer {
        return TaggedPointer{ raw: raw_bits };
    }

    pub fn address(): usize {
        return (_.raw & ADDR_MASK) as usize;
    }

    pub fn tag(): u16 {
        return ((_.raw >> TAG_SHIFT) & 0xFFFF) as u16;
    }

    pub fn withTag(new_tag: u16): TaggedPointer {
        return TaggedPointer.pack(_.address(), new_tag);
    }

    pub fn isNull(): bool {
        return (_.raw & ADDR_MASK) == 0;
    }

    pub fn bits(): u64 {
        return _.raw;
    }

    pub fn toRawPtr(): *!? {
        const addr = _.address();
        if (addr == 0) return null;
        unsafe {
            return addr as *!?;
        }
    }

    pub fn get(comptime Target: type): *Target {
        const addr = _.address();
        if (addr == 0) {
            @panic("TaggedPointer.get called on null address");
        }
        unsafe {
            return addr as *Target;
        }
    }
}

pub struct LowTagPointer {
    raw: usize = 0,

    pub static fn pack(aligned_addr: usize, low_tag: u8): LowTagPointer {
        const clean_addr = aligned_addr & ~LOW_TAG_MASK;
        const tag_bits = (low_tag as usize) & LOW_TAG_MASK;
        return LowTagPointer{
            raw: clean_addr | tag_bits,
        };
    }

    pub static fn fromPtr(ptr: auto, low_tag: u8): LowTagPointer {
        const PtrType = @TypeOf(ptr);
        if (PtrType == @TypeOf(null)) {
            return LowTagPointer.pack(0, low_tag);
        }
        let addr: usize = 0;
        unsafe {
            addr = @intFromPtr(ptr);
        }
        return LowTagPointer.pack(addr, low_tag);
    }

    pub fn address(): usize {
        return _.raw & ~LOW_TAG_MASK;
    }

    pub fn tag(): u8 {
        return (_.raw & LOW_TAG_MASK) as u8;
    }

    pub fn withTag(new_tag: u8): LowTagPointer {
        return LowTagPointer.pack(_.address(), new_tag);
    }

    pub fn isNull(): bool {
        return _.address() == 0;
    }

    pub fn toRawPtr(): *!? {
        const addr = _.address();
        if (addr == 0) return null;
        unsafe {
            return addr as *!?;
        }
    }

    pub fn get(comptime Target: type): *Target {
        const addr = _.address();
        if (addr == 0) {
            @panic("LowTagPointer.get called on null address");
        }
        unsafe {
            return addr as *Target;
        }
    }
}
