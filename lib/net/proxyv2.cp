// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:49:17Z (HAProxy PROXY Protocol v2 Binary Codec)

/// HAProxy PROXY Protocol v2 固定 12 字节二进制签名：`\r\n\r\n\0\r\nQUIT\n`
export const PROXY_V2_SIGNATURE: [12]u8 = [
    0x0D, 0x0A, 0x0D, 0x0A,
    0x00, 0x0D, 0x0A, 0x51,
    0x55, 0x49, 0x54, 0x0A,
];

/// 判断缓冲区前缀是否匹配 HAProxy PROXY v2 12 字节签名
export fn hasProxyV2Signature(buf: []const u8): bool {
    if (buf.len < 12) {
        return false;
    }
    let i: usize = 0;
    while (i < 12) {
        if (buf[i] != PROXY_V2_SIGNATURE[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn readU16Be(buf: []const u8, off: usize): u16 {
    return (u16(buf[off]) << 8) | u16(buf[off + 1]);
}

fn writeU16Be(out: []u8, off: usize, val: u16) {
    out[off] = ((val >> 8) & 0xFF) as u8;
    out[off + 1] = (val & 0xFF) as u8;
}

/// PROXY v2 命令类型
export enum ProxyV2Command {
    local,
    proxy,
}

/// PROXY v2 地址族
export enum ProxyV2Family {
    unspec,
    inet4,
    inet6,
    unix,
}

/// PROXY v2 传输层协议
export enum ProxyV2Transport {
    unspec,
    stream,
    dgram,
}

/// PROXY v2 TLV 扩展条目（`[type:1][len:2 BE][val:len]`）
export struct ProxyV2Tlv {
    tlv_type: u8;
    value: []const u8;
}

/// PROXY v2 TLV 扩展零分配流式迭代器
export struct ProxyV2TlvIter {
    src: []const u8;
    pos: usize;

    fn init(tlvs_raw: []const u8): ProxyV2TlvIter {
        return ProxyV2TlvIter{
            src: tlvs_raw,
            pos: 0,
        };
    }

    fn next(*_): ProxyV2Tlv? {
        if (_.pos >= _.src.len) {
            return null;
        }
        const rem = _.src.len - _.pos;
        if (rem < 3) {
            _.pos = _.src.len;
            return null;
        }
        const t = _.src[_.pos];
        const vlen: usize = readU16Be(_.src, _.pos + 1) as usize;
        if (rem - 3 < vlen) {
            _.pos = _.src.len;
            return null;
        }
        const val_start = _.pos + 3;
        const val_end = val_start + vlen;
        _.pos = val_end;
        return ProxyV2Tlv{
            tlv_type: t,
            value: _.src[val_start..val_end],
        };
    }
}

/// HAProxy PROXY Protocol v2 解析结果（零堆分配）
export struct ProxyV2Header {
    command: ProxyV2Command;
    family: ProxyV2Family;
    transport: ProxyV2Transport;
    src_ipv4: [4]u8;
    dst_ipv4: [4]u8;
    src_ipv6: [16]u8;
    dst_ipv6: [16]u8;
    src_port: u16;
    dst_port: u16;
    tlvs_raw: []const u8;
    header_len: usize;

    /// 解析 PROXY v2 二进制头部（校验 12 字节签名、ver==2、合法命令/族/传输协议与长度边界）
    fn parse(buf: []const u8): ProxyV2Header? {
        if (buf.len < 16 || !hasProxyV2Signature(buf)) {
            return null;
        }

        const ver_cmd = buf[12];
        const ver = (ver_cmd >> 4) & 0x0F;
        if (ver != 0x02) {
            return null;
        }

        const cmd_bits = ver_cmd & 0x0F;
        let command = ProxyV2Command.local;
        if (cmd_bits == 0x00) {
            command = ProxyV2Command.local;
        } else if (cmd_bits == 0x01) {
            command = ProxyV2Command.proxy;
        } else {
            return null;
        }

        const fam_proto = buf[13];
        const fam_bits = (fam_proto >> 4) & 0x0F;
        let family = ProxyV2Family.unspec;
        if (fam_bits == 0x00) {
            family = ProxyV2Family.unspec;
        } else if (fam_bits == 0x01) {
            family = ProxyV2Family.inet4;
        } else if (fam_bits == 0x02) {
            family = ProxyV2Family.inet6;
        } else if (fam_bits == 0x03) {
            family = ProxyV2Family.unix;
        } else {
            return null;
        }

        const proto_bits = fam_proto & 0x0F;
        let transport = ProxyV2Transport.unspec;
        if (proto_bits == 0x00) {
            transport = ProxyV2Transport.unspec;
        } else if (proto_bits == 0x01) {
            transport = ProxyV2Transport.stream;
        } else if (proto_bits == 0x02) {
            transport = ProxyV2Transport.dgram;
        } else {
            return null;
        }

        const payload_len: usize = readU16Be(buf, 14) as usize;
        const header_len: usize = payload_len + 16;
        if (buf.len < header_len) {
            return null;
        }

        let src_ipv4: [4]u8 = [0; 4];
        let dst_ipv4: [4]u8 = [0; 4];
        let src_ipv6: [16]u8 = [0; 16];
        let dst_ipv6: [16]u8 = [0; 16];
        let src_port: u16 = 0;
        let dst_port: u16 = 0;
        let tlv_offset: usize = 16;

        if (command == ProxyV2Command.proxy) {
            if (family == ProxyV2Family.inet4) {
                if (payload_len < 12) {
                    return null;
                }
                let i: usize = 0;
                while (i < 4) {
                    src_ipv4[i] = buf[16 + i];
                    dst_ipv4[i] = buf[20 + i];
                    i += 1;
                }
                src_port = readU16Be(buf, 24);
                dst_port = readU16Be(buf, 26);
                tlv_offset = 28;
            } else if (family == ProxyV2Family.inet6) {
                if (payload_len < 36) {
                    return null;
                }
                let i: usize = 0;
                while (i < 16) {
                    src_ipv6[i] = buf[16 + i];
                    dst_ipv6[i] = buf[32 + i];
                    i += 1;
                }
                src_port = readU16Be(buf, 48);
                dst_port = readU16Be(buf, 50);
                tlv_offset = 52;
            } else if (family == ProxyV2Family.unix) {
                if (payload_len < 216) {
                    return null;
                }
                tlv_offset = 232;
            }
        }

        return ProxyV2Header{
            command: command,
            family: family,
            transport: transport,
            src_ipv4: src_ipv4,
            dst_ipv4: dst_ipv4,
            src_ipv6: src_ipv6,
            dst_ipv6: dst_ipv6,
            src_port: src_port,
            dst_port: dst_port,
            tlvs_raw: buf[tlv_offset..header_len],
            header_len: header_len,
        };
    }

    /// 编码 PROXY v2 IPv4/TCP（`PROXY` + `AF_INET` + `STREAM`）28 字节头部
    fn encodeIpv4(
        src: [4]u8,
        dst: [4]u8,
        src_port: u16,
        dst_port: u16,
        out: []u8,
    ): ([]const u8)? {
        if (out.len < 28) {
            return null;
        }
        let i: usize = 0;
        while (i < 12) {
            out[i] = PROXY_V2_SIGNATURE[i];
            i += 1;
        }
        out[12] = 0x21; // ver=2, cmd=PROXY
        out[13] = 0x11; // fam=AF_INET, proto=STREAM
        writeU16Be(out, 14, 12);

        let k: usize = 0;
        while (k < 4) {
            out[16 + k] = src[k];
            out[20 + k] = dst[k];
            k += 1;
        }
        writeU16Be(out, 24, src_port);
        writeU16Be(out, 26, dst_port);
        return out[0..28];
    }

    /// 编码 PROXY v2 `LOCAL` 16 字节头部
    fn encodeLocal(out: []u8): ([]const u8)? {
        if (out.len < 16) {
            return null;
        }
        let i: usize = 0;
        while (i < 12) {
            out[i] = PROXY_V2_SIGNATURE[i];
            i += 1;
        }
        out[12] = 0x20; // ver=2, cmd=LOCAL
        out[13] = 0x00; // fam=UNSPEC, proto=UNSPEC
        out[14] = 0x00;
        out[15] = 0x00;
        return out[0..16];
    }
}
