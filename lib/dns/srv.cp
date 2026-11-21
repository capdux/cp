// Source: Absorbed from /data/dev/bun/src/dns.zig (RFC 2782 DNS SRV Record Parser & Weighted Selector)

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn readU16Be(buf: []const u8, off: usize): u16 {
    return (u16(buf[off]) << 8) | u16(buf[off + 1]);
}

fn parseU16Dec(s: []const u8): u16? {
    if (s.len == 0 || s.len > 5) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + u32(c - '0');
        if (acc > 65535) {
            return null;
        }
        i += 1;
    }
    return acc as u16;
}

fn isLabelChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '-';
}

fn isValidSrvLabel(s: []const u8): bool {
    if (s.len == 0 || s.len > 63) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isLabelChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidDomainTarget(s: []const u8): bool {
    if (s.len == 0 || s.len > 255) {
        return false;
    }
    if (s.len == 1 && s[0] == '.') {
        return true;
    }
    let label_len: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '.') {
            if (label_len == 0 || label_len > 63) {
                return false;
            }
            label_len = 0;
        } else if (isLabelChar(c) || c == '_') {
            label_len += 1;
            if (label_len > 63) {
                return false;
            }
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}

fn nextToken(s: []const u8, pos: *usize): ([]const u8)? {
    while (pos.* < s.len && isSpace(s[pos.*])) {
        pos.* += 1;
    }
    if (pos.* >= s.len) {
        return null;
    }
    const start = pos.*;
    while (pos.* < s.len && !isSpace(s[pos.*])) {
        pos.* += 1;
    }
    return s[start..pos.*];
}

/// RFC 2782 SRV RDATA 二进制线缆头部（前 6 字节大端三元组 + 后续目标域名线缆切片）
export struct SrvWireHeader {
    priority: u16;
    weight: u16;
    port: u16;
    target_wire: []const u8;
}

/// RFC 2782 DNS SRV 服务发现记录（零堆分配切片视图）
export struct SrvRecord {
    priority: u16;
    weight: u16;
    port: u16;
    target: []const u8;

    /// RFC 2782：`target == "."` 表示该域名明确不提供对应服务
    fn isServiceUnavailable(): bool {
        return _.target.len == 1 && _.target[0] == '.';
    }

    /// 解析文本格式 SRV RDATA（`"<priority> <weight> <port> <target>"`）
    fn parseText(rdata: []const u8): SrvRecord? {
        let pos: usize = 0;
        const prio_s = nextToken(rdata, &pos) ?? return null;
        const weight_s = nextToken(rdata, &pos) ?? return null;
        const port_s = nextToken(rdata, &pos) ?? return null;
        const target = nextToken(rdata, &pos) ?? return null;

        if (nextToken(rdata, &pos) != null) {
            return null;
        }

        const priority = parseU16Dec(prio_s) ?? return null;
        const weight = parseU16Dec(weight_s) ?? return null;
        const port = parseU16Dec(port_s) ?? return null;
        if (!isValidDomainTarget(target)) {
            return null;
        }

        return SrvRecord{
            priority: priority,
            weight: weight,
            port: port,
            target: target,
        };
    }

    /// 解析二进制线缆格式 SRV 前 6 字节大端头（`priority`, `weight`, `port`）与剩余目标域名切片
    fn parseWireHeader(rdata: []const u8): SrvWireHeader? {
        if (rdata.len < 6) {
            return null;
        }
        return SrvWireHeader{
            priority: readU16Be(rdata, 0),
            weight: readU16Be(rdata, 2),
            port: readU16Be(rdata, 4),
            target_wire: rdata[6..],
        };
    }
}

/// RFC 2782 SRV 查询名三元组（`_service._proto.domain`）
export struct SrvServiceName {
    service: []const u8;
    proto: []const u8;
    domain: []const u8;

    /// 解析 `_http._tcp.example.com.` 等 RFC 2782 SRV 查询名（`service` 与 `proto` 去除前导 `_`）
    fn parse(qname: []const u8): SrvServiceName? {
        const s = trimAscii(qname);
        if (s.len < 5 || s[0] != '_') {
            return null;
        }

        // 1. _service
        let dot1: usize = 1;
        while (dot1 < s.len && s[dot1] != '.') {
            dot1 += 1;
        }
        if (dot1 >= s.len) {
            return null;
        }
        const service = s[1..dot1];
        if (!isValidSrvLabel(service)) {
            return null;
        }

        // 2. _proto
        const proto_start = dot1 + 1;
        if (proto_start >= s.len || s[proto_start] != '_') {
            return null;
        }
        let dot2: usize = proto_start + 1;
        while (dot2 < s.len && s[dot2] != '.') {
            dot2 += 1;
        }
        if (dot2 >= s.len) {
            return null;
        }
        const proto = s[(proto_start + 1)..dot2];
        if (!isValidSrvLabel(proto)) {
            return null;
        }

        // 3. domain（必须为非空合法域名且非孤立根点 "."）
        const domain = s[(dot2 + 1)..];
        if (domain.len == 0 || (domain.len == 1 && domain[0] == '.')) {
            return null;
        }
        if (!isValidDomainTarget(domain)) {
            return null;
        }

        return SrvServiceName{
            service: service,
            proto: proto,
            domain: domain,
        };
    }
}

/// RFC 2782 排序比较器：`priority` 升序，同优先级 `weight` 降序（返回 -1 / 0 / 1）
export fn compareSrvOrder(a: SrvRecord, b: SrvRecord): i32 {
    if (a.priority < b.priority) {
        return -1;
    }
    if (a.priority > b.priority) {
        return 1;
    }
    if (a.weight > b.weight) {
        return -1;
    }
    if (a.weight < b.weight) {
        return 1;
    }
    return 0;
}

/// 按 RFC 2782 同优先级确定性权重落点选择候选记录索引（跳过 `target == "."` 不可用项）
export fn selectSrvByRoll(records: []const SrvRecord, priority: u16, roll: u32): usize? {
    let count: usize = 0;
    let total_weight: u64 = 0;
    let i: usize = 0;
    while (i < records.len) {
        const rec = records[i];
        if (rec.priority == priority && !rec.isServiceUnavailable()) {
            count += 1;
            total_weight += u64(rec.weight);
        }
        i += 1;
    }
    if (count == 0) {
        return null;
    }

    if (total_weight == 0) {
        const target_idx: usize = (u64(roll) % (count as u64)) as usize;
        let seen: usize = 0;
        let j: usize = 0;
        while (j < records.len) {
            const rec = records[j];
            if (rec.priority == priority && !rec.isServiceUnavailable()) {
                if (seen == target_idx) {
                    return j;
                }
                seen += 1;
            }
            j += 1;
        }
        return null;
    }

    const bucket: u64 = u64(roll) % total_weight;
    let running: u64 = 0;
    let k: usize = 0;
    while (k < records.len) {
        const rec = records[k];
        if (rec.priority == priority && !rec.isServiceUnavailable() && rec.weight > 0) {
            running += u64(rec.weight);
            if (bucket < running) {
                return k;
            }
        }
        k += 1;
    }
    return null;
}
