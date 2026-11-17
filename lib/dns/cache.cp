// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/dns/cache.cp
// High-speed TTL-aware DNS Resolver Cache & Options (absorbed from bun/src/dns/dns.cp)

pub enum DnsFamily: u8 {
    Unspecified = 0,
    IPv4 = 4,
    IPv6 = 6,

    static fn fromString(s: []const u8): DnsFamily {
        if (s.len == 4 && (s[0] == 73 || s[0] == 105) && s[3] == 52) {
            return .IPv4;
        }
        if (s.len == 4 && (s[0] == 73 || s[0] == 105) && s[3] == 54) {
            return .IPv6;
        }
        return .Unspecified;
    }
}

pub enum DnsBackend: u8 {
    Cares = 0,
    System = 1,
    GetAddrInfo = 2,
}

pub struct ResolveOptions {
    family: DnsFamily = .Unspecified;
    backend: DnsBackend = .Cares;
    ttl_ms: u64 = 30000;
}

pub struct DnsCacheEntry {
    host: []const u8 = "";
    ipv4_packed: u32 = 0;
    port: u16 = 0;
    expires_at_ms: u64 = 0;
    hits: u32 = 0;
    active: bool = false;
}

pub struct DnsCache<const CAP: usize> {
    entries: [CAP]DnsCacheEntry = undefined;
    count: usize = 0;
    total_hits: u64 = 0;
    total_misses: u64 = 0;

    pub static fn init(): DnsCache<CAP> {
        let c = DnsCache<CAP>{
            entries: undefined,
            count: 0,
            total_hits: 0,
            total_misses: 0,
        };
        let i: usize = 0;
        while (i < CAP) {
            c.entries[i] = DnsCacheEntry{
                host: "",
                ipv4_packed: 0,
                port: 0,
                expires_at_ms: 0,
                hits: 0,
                active: false,
            };
            i += 1;
        }
        return c;
    }

    pub fn put(*_, host: []const u8, ipv4_packed: u32, port: u16, now_ms: u64, ttl_ms: u64): bool {
        // Update existing or find free/expired slot
        let free_slot: usize = CAP;
        let i: usize = 0;
        while (i < CAP) {
            if (_.entries[i].active) {
                const same = _.entries[i].host == host;
                if (same && _.entries[i].port == port) {
                    _.entries[i].ipv4_packed = ipv4_packed;
                    _.entries[i].expires_at_ms = now_ms + ttl_ms;
                    return true;
                }
                if (now_ms >= _.entries[i].expires_at_ms && free_slot == CAP) {
                    _.entries[i].active = false;
                    _.count -= 1;
                    free_slot = i;
                }
            } else if (free_slot == CAP) {
                free_slot = i;
            }
            i += 1;
        }

        if (free_slot < CAP) {
            _.entries[free_slot] = DnsCacheEntry{
                host: host,
                ipv4_packed: ipv4_packed,
                port: port,
                expires_at_ms: now_ms + ttl_ms,
                hits: 0,
                active: true,
            };
            _.count += 1;
            return true;
        }
        return false;
    }

    pub fn lookup(*_, host: []const u8, port: u16, now_ms: u64): u32? {
        let i: usize = 0;
        while (i < CAP) {
            if (_.entries[i].active && _.entries[i].port == port) {
                const same = _.entries[i].host == host;
                if (same) {
                    if (now_ms < _.entries[i].expires_at_ms) {
                        _.entries[i].hits += 1;
                        _.total_hits += 1;
                        return _.entries[i].ipv4_packed;
                    } else {
                        _.entries[i].active = false;
                        _.count -= 1;
                        _.total_misses += 1;
                        return null;
                    }
                }
            }
            i += 1;
        }
        _.total_misses += 1;
        return null;
    }

    pub fn evictExpired(*_, now_ms: u64): usize {
        let evicted: usize = 0;
        let i: usize = 0;
        while (i < CAP) {
            if (_.entries[i].active && now_ms >= _.entries[i].expires_at_ms) {
                _.entries[i].active = false;
                _.count -= 1;
                evicted += 1;
            }
            i += 1;
        }
        return evicted;
    }

    pub fn activeCount(): usize {
        return _.count;
    }
}
