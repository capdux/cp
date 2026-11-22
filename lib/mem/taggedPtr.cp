// Source: Absorbed from /data/dev/bun/src/bun_alloc/ | Synced: 2026-11-22T12:49:17Z
// lib/mem/tagged_ptr.cp
// 64-bit NaN-boxing / upper-16-bit & low-3-bit Tagged Pointers (absorbed from bun/src/ptr/tagged_pointer.cp)

pub const TAG_SHIFT: u6 = 48;
pub const ADDR_MASK: u64 = 0x0000FFFFFFFFFFFF;
pub const LOW_TAG_MASK: usize = 7;

// Upper 16-bit tagged pointer (48-bit canonical address + 16-bit tag payload)
pub struct TaggedPointer {
    raw: u64 = 0;

    pub static fn pack(addr: usize, tag_val: u16): TaggedPointer {
        const masked_addr = (addr as u64) & ADDR_MASK;
        const shifted_tag = (tag_val as u64) << 48;
        return TaggedPointer{
            raw: masked_addr | shifted_tag,
        };
    }

    pub static fn fromPtr(ptr: *!?, tag_val: u16): TaggedPointer {
        if (p := ptr) {
            let addr: usize = 0;
            unsafe {
                addr = @intFromPtr(p);
            }
            return TaggedPointer.pack(addr, tag_val);
        }
        return TaggedPointer.pack(0, tag_val);
    }

    pub static fn fromBits(raw_bits: u64): TaggedPointer {
        return TaggedPointer{ raw: raw_bits };
    }

    pub fn address(): usize {
        return (_.raw & ADDR_MASK) as usize;
    }

    pub fn tag(): u16 {
        return (_.raw >> 48) as u16;
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
        if (addr == 0) {
            return null;
        }
        unsafe {
            return addr as *!?;
        }
    }
}

// Low 3-bit tagged pointer for 8-byte aligned allocations
pub struct LowTagPointer {
    raw: usize = 0;

    pub static fn pack(aligned_addr: usize, low_tag: u8): LowTagPointer {
        const clean_addr = aligned_addr & ~LOW_TAG_MASK;
        const tag_bits = (low_tag as usize) & LOW_TAG_MASK;
        return LowTagPointer{
            raw: clean_addr | tag_bits,
        };
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
}

pub struct TaggedPtr {
    raw: u64 = 0;

    pub static fn pack(addr: usize, tag_val: u16): TaggedPtr {
        const tp = TaggedPointer.pack(addr, tag_val);
        return TaggedPtr { raw: tp.raw };
    }

    pub fn address(): usize {
        return (_.raw & ADDR_MASK) as usize;
    }

    pub fn tag(): u16 {
        return (_.raw >> 48) as u16;
    }

    pub fn withTag(new_tag: u16): TaggedPtr {
        return TaggedPtr.pack(_.address(), new_tag);
    }
}
