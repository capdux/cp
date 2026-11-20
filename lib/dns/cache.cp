// Source: /data/dev/bun/src/dns/
// lib/dns/cache.cp
// High-speed TTL-aware DNS Resolver Cache, /etc/hosts Parser & Options (absorbed from bun/src/dns/dns.cp)

pub enum DnsFamily: u8 {
    Unspecified = 0,
    IPv4 = 4,
    IPv6 = 6,

    pub static fn fromString(s: []const u8): DnsFamily {
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
    pub family: DnsFamily = .Unspecified;
    pub backend: DnsBackend = .Cares;
    pub ttl_ms: u64 = 30000;
}

pub struct DnsCacheEntry {
    pub host: []const u8 = "";
    pub ipv4_packed: u32 = 0;
    pub ipv6_addr: [16]u8 = [0; 16];
    pub is_ipv6: bool = false;
    pub port: u16 = 0;
    pub expires_at_ms: u64 = 0;
    pub hits: u32 = 0;
    pub active: bool = false;
}

pub struct DnsCache<const CAP: usize> {
    pub entries: [CAP]DnsCacheEntry = undefined;
    pub count: usize = 0;
    pub total_hits: u64 = 0;
    pub total_misses: u64 = 0;

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
                ipv6_addr: [0; 16],
                is_ipv6: false,
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
                if (same && !_.entries[i].is_ipv6 && _.entries[i].port == port) {
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
                ipv6_addr: [0; 16],
                is_ipv6: false,
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

    pub fn putAAAA(*_, host: []const u8, ipv6_addr: [16]u8, port: u16, now_ms: u64, ttl_ms: u64): bool {
        let free_slot: usize = CAP;
        let i: usize = 0;
        while (i < CAP) {
            if (_.entries[i].active) {
                const same = _.entries[i].host == host;
                if (same && _.entries[i].is_ipv6 && _.entries[i].port == port) {
                    _.entries[i].ipv6_addr = ipv6_addr;
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
                ipv4_packed: 0,
                ipv6_addr: ipv6_addr,
                is_ipv6: true,
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
            if (_.entries[i].active && !_.entries[i].is_ipv6 && _.entries[i].port == port) {
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

    pub fn lookupAAAA(*_, host: []const u8, port: u16, now_ms: u64): ([16]u8)? {
        let i: usize = 0;
        while (i < CAP) {
            if (_.entries[i].active && _.entries[i].is_ipv6 && _.entries[i].port == port) {
                const same = _.entries[i].host == host;
                if (same) {
                    if (now_ms < _.entries[i].expires_at_ms) {
                        _.entries[i].hits += 1;
                        _.total_hits += 1;
                        return _.entries[i].ipv6_addr;
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

pub struct HostsEntry {
    pub host: []const u8 = "";
    pub ip_str: []const u8 = "";
    pub ipv4_packed: u32 = 0;
    pub is_ipv6: bool = false;
}

/// Zero-allocation `/etc/hosts` static table parser and lookup engine
pub struct HostsTable {
    pub entries: [64]HostsEntry = [HostsEntry{}; 64];
    pub count: usize = 0;

    pub static fn init(): HostsTable {
        return HostsTable{
            entries: [HostsEntry{}; 64],
            count: 0,
        };
    }

    /// Parses a dotted-quad IPv4 string (e.g. `"127.0.0.1"`) into a packed big-endian `u32` (`0x7F000001`).
    pub static fn parseIPv4(ip_str: []const u8): u32? {
        if (ip_str.len < 7 || ip_str.len > 15) return null;
        let octets: [4]u32 = [0, 0, 0, 0];
        let oct_idx: usize = 0;
        let acc: u32 = 0;
        let digits: usize = 0;
        let i: usize = 0;

        while (i < ip_str.len) {
            const c = ip_str[i];
            if (c == '.') {
                if (digits == 0 || oct_idx >= 3) return null;
                octets[oct_idx] = acc;
                oct_idx += 1;
                acc = 0;
                digits = 0;
            } else if (c >= '0' && c <= '9') {
                acc = acc * 10 + ((c - '0') as u32);
                if (acc > 255) return null;
                digits += 1;
            } else {
                return null;
            }
            i += 1;
        }
        if (oct_idx != 3 || digits == 0) return null;
        octets[3] = acc;

        return (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    }

    /// Parses `/etc/hosts` file text into a `HostsTable` without heap allocation.
    pub static fn parse(content: []const u8): HostsTable {
        let table = HostsTable.init();
        let line_start: usize = 0;
        let i: usize = 0;

        while (i <= content.len) {
            if (i == content.len || content[i] == '\n') {
                let line_end = i;
                if (line_end > line_start && content[line_end - 1] == '\r') {
                    line_end -= 1;
                }
                const raw_line = content[line_start..line_end];

                // Strip comments starting with '#'
                let comment_pos = raw_line.len;
                let k: usize = 0;
                while (k < raw_line.len) {
                    if (raw_line[k] == '#') {
                        comment_pos = k;
                        break;
                    }
                    k += 1;
                }
                const line = raw_line[0..comment_pos];

                // Extract first token (IP address) and subsequent tokens (hostnames/aliases)
                let cur: usize = 0;
                while (cur < line.len && (line[cur] == ' ' || line[cur] == '\t')) {
                    cur += 1;
                }
                if (cur < line.len) {
                    const ip_start = cur;
                    while (cur < line.len && line[cur] != ' ' && line[cur] != '\t') {
                        cur += 1;
                    }
                    const ip_slice = line[ip_start..cur];
                    const packed_opt = HostsTable.parseIPv4(ip_slice);
                    const ipv4_val = packed_opt ?? 0;
                    const is_v6 = packed_opt == null;

                    while (cur < line.len && table.count < 64) {
                        while (cur < line.len && (line[cur] == ' ' || line[cur] == '\t')) {
                            cur += 1;
                        }
                        if (cur >= line.len) break;
                        const h_start = cur;
                        while (cur < line.len && line[cur] != ' ' && line[cur] != '\t') {
                            cur += 1;
                        }
                        const host_slice = line[h_start..cur];
                        table.entries[table.count] = HostsEntry{
                            host: host_slice,
                            ip_str: ip_slice,
                            ipv4_packed: ipv4_val,
                            is_ipv6: is_v6,
                        };
                        table.count += 1;
                    }
                }

                line_start = i + 1;
            }
            i += 1;
        }
        return table;
    }

    pub fn entryCount(): usize {
        return _.count;
    }

    /// Looks up the first matching IP string for `host`.
    pub fn lookup(host: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].host == host) {
                return _.entries[i].ip_str;
            }
            i += 1;
        }
        return null;
    }

    /// Looks up the first matching packed IPv4 address (`u32`) for `host`.
    pub fn lookupIPv4(host: []const u8): u32? {
        let i: usize = 0;
        while (i < _.count) {
            if (!_.entries[i].is_ipv6 && _.entries[i].host == host) {
                return _.entries[i].ipv4_packed;
            }
            i += 1;
        }
        return null;
    }

    /// Looks up the first matching IPv6 string for `host`.
    pub fn lookupIPv6(host: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].is_ipv6 && _.entries[i].host == host) {
                return _.entries[i].ip_str;
            }
            i += 1;
        }
        return null;
    }
}
