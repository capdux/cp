// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/AuthSwitchRequest.cp | Synced: 2026-11-22T06:05:31Z
// ============================================================================
// Pure-CP Zero-Allocation MySQL 4.1/8.0 AuthSwitch, SSLRequest & LocalInfileRequest Frame Codec
// Absorbed from Bun: src/sql/mysql/protocol/{AuthSwitchRequest,AuthSwitchResponse,SSLRequest,LocalInfileRequest}.cp
// ============================================================================

import {
    MYSQL_PACKET_HEADER_SIZE,
    MySqlWireWriter,
} from "./mysqlWriter.cp";
import { MySqlWireReader } from "./mysqlReader.cp";
import { MySqlWirePacketHeader } from "./mysqlStmt.cp";

/// MySQL `AuthSwitchRequest` 包标识字节 (`0xFE`)
pub const MYSQL_AUTH_SWITCH_HEADER: u8 = 0xfe;

/// MySQL `LOCAL_INFILE_Request` 包标识字节 (`0xFB`)
pub const MYSQL_LOCAL_INFILE_HEADER: u8 = 0xfb;

/// MySQL `SSLRequest` 固定载荷字节数 (`4B cap + 4B max_pkt + 1B charset + 23B reserved = 32B`)
pub const MYSQL_SSL_REQUEST_PAYLOAD_LEN: usize = 32;

/// MySQL `CLIENT_SSL` 能力标志位 (`1 << 11 = 0x0800`)
pub const MYSQL_CAP_CLIENT_SSL: u32 = 0x00000800;

/// MySQL 认证切换 / SSL / LocalInfile 帧编解码错误集
pub err MySqlAuthFrameError {
    BufferTooSmall,
    TruncatedPayload,
    InvalidHeader,
    MissingPluginTerminator,
    InvalidSslRequestLength,
    MissingSslCapability,
    EmptyFilename,
    TrailingBytes,
}

/// 零拷贝 `AuthSwitchRequest` (`0xFE`) 视图（对齐 Bun `AuthSwitchRequest.cp`）
pub struct MySqlAuthSwitchReqView {
    sequence_id: u8 = 0,
    plugin_name: []const u8 = "",
    plugin_data: []const u8 = "",
}

/// 零拷贝 `AuthSwitchResponse` 视图（对齐 Bun `AuthSwitchResponse.cp`）
pub struct MySqlAuthSwitchRespView {
    sequence_id: u8 = 0,
    auth_response: []const u8 = "",
}

/// MySQL `SSLRequest` 32 字节固定载荷视图（对齐 Bun `SSLRequest.cp`）
pub struct MySqlSslRequestView {
    sequence_id: u8 = 1,
    capabilities: u32 = MYSQL_CAP_CLIENT_SSL,
    max_packet_size: u32 = 0x01000000,
    character_set: u8 = 45, // utf8mb4_general_ci

    fn hasSslFlag(): bool {
        return (_.capabilities & MYSQL_CAP_CLIENT_SSL) != 0;
    }
}

/// 零拷贝 `LOCAL_INFILE_Request` (`0xFB`) 视图（对齐 Bun `LocalInfileRequest.cp`）
pub struct MySqlLocalInfileReqView {
    sequence_id: u8 = 0,
    filename: []const u8 = "",

    /// 安全校验：拦截绝对路径、`..` 目录穿越、反斜杠与控制字符（防止恶意服务端诱导读取任意本地敏感文件）
    fn isSafeRelativeFilename(): bool {
        if (_.filename.len == 0) {
            return false;
        }
        if (_.filename[0] == '/' || _.filename[0] == '\\') {
            return false;
        }
        let i: usize = 0;
        while (i < _.filename.len) {
            const ch = _.filename[i];
            if (ch < 0x20 || ch == 0x7f || ch == '\\') {
                return false;
            }
            if (ch == '.' && i + 1 < _.filename.len && _.filename[i + 1] == '.') {
                return false;
            }
            i += 1;
        }
        return true;
    }
}

/// 编码带 4 字节包头的 `AuthSwitchRequest` (`0xFE`) 帧（零堆分配）
pub fn encodeMySqlAuthSwitchRequest(
    buf: []u8,
    sequence_id: u8,
    plugin_name: []const u8,
    plugin_data: []const u8,
): ([]const u8)! {
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(MYSQL_AUTH_SWITCH_HEADER) catch {
        return err.BufferTooSmall;
    };
    * = w.writeCString(plugin_name) catch {
        return err.BufferTooSmall;
    };
    if (plugin_data.len > 0) {
        * = w.writeBytes(plugin_data) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码带 4 字节包头的 `AuthSwitchRequest` (`0xFE`) 帧（零拷贝）
pub fn decodeMySqlAuthSwitchRequest(wire: []const u8): MySqlAuthSwitchReqView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    if (payload_len < 2) {
        return err.TruncatedPayload;
    }
    if (wire[MYSQL_PACKET_HEADER_SIZE] != MYSQL_AUTH_SWITCH_HEADER) {
        return err.InvalidHeader;
    }
    const rest = wire[MYSQL_PACKET_HEADER_SIZE + 1 .. total];
    let nul_pos: usize = rest.len;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == 0) {
            nul_pos = i;
            break;
        }
        i += 1;
    }
    if (nul_pos == rest.len) {
        return err.MissingPluginTerminator;
    }
    return MySqlAuthSwitchReqView{
        sequence_id: hdr.sequence_id,
        plugin_name: rest[0..nul_pos],
        plugin_data: rest[nul_pos + 1 ..],
    };
}

/// 编码带 4 字节包头的 `AuthSwitchResponse` 帧（零堆分配）
pub fn encodeMySqlAuthSwitchResponse(
    buf: []u8,
    sequence_id: u8,
    auth_response: []const u8,
): ([]const u8)! {
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(sequence_id) catch {
        return err.BufferTooSmall;
    };
    if (auth_response.len > 0) {
        * = w.writeBytes(auth_response) catch {
            return err.BufferTooSmall;
        };
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码带 4 字节包头的 `AuthSwitchResponse` 帧（零拷贝）
pub fn decodeMySqlAuthSwitchResponse(wire: []const u8): MySqlAuthSwitchRespView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    return MySqlAuthSwitchRespView{
        sequence_id: hdr.sequence_id,
        auth_response: wire[MYSQL_PACKET_HEADER_SIZE..total],
    };
}

/// 编码带 4 字节包头的 `SSLRequest` 36 字节定长帧（自动确保 `CLIENT_SSL` 位或严格校验）
pub fn encodeMySqlSslRequestFrame(buf: []u8, req: MySqlSslRequestView): ([]const u8)! {
    if (!req.hasSslFlag()) {
        return err.MissingSslCapability;
    }
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(req.sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(req.capabilities) catch {
        return err.BufferTooSmall;
    };
    * = w.writeU32(req.max_packet_size) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(req.character_set) catch {
        return err.BufferTooSmall;
    };
    let z: usize = 0;
    while (z < 23) {
        * = w.writeByte(0) catch {
            return err.BufferTooSmall;
        };
        z += 1;
    }
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码带 4 字节包头的 `SSLRequest` 36 字节定长帧
pub fn decodeMySqlSslRequestFrame(wire: []const u8): MySqlSslRequestView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    if (payload_len != MYSQL_SSL_REQUEST_PAYLOAD_LEN) {
        return err.InvalidSslRequestLength;
    }
    let r = MySqlWireReader.init(wire[MYSQL_PACKET_HEADER_SIZE..total]);
    const cap = r.readU32() catch {
        return err.TruncatedPayload;
    };
    if ((cap & MYSQL_CAP_CLIENT_SSL) == 0) {
        return err.MissingSslCapability;
    }
    const max_pkt = r.readU32() catch {
        return err.TruncatedPayload;
    };
    const charset = r.readByte() catch {
        return err.TruncatedPayload;
    };
    return MySqlSslRequestView{
        sequence_id: hdr.sequence_id,
        capabilities: cap,
        max_packet_size: max_pkt,
        character_set: charset,
    };
}

/// 编码带 4 字节包头的 `LOCAL_INFILE_Request` (`0xFB`) 帧
pub fn encodeMySqlLocalInfileRequest(
    buf: []u8,
    sequence_id: u8,
    filename: []const u8,
): ([]const u8)! {
    if (filename.len == 0) {
        return err.EmptyFilename;
    }
    let w = MySqlWireWriter.init(buf);
    const patch = w.beginPacket(sequence_id) catch {
        return err.BufferTooSmall;
    };
    * = w.writeByte(MYSQL_LOCAL_INFILE_HEADER) catch {
        return err.BufferTooSmall;
    };
    * = w.writeBytes(filename) catch {
        return err.BufferTooSmall;
    };
    * = w.finishPacket(patch) catch {
        return err.BufferTooSmall;
    };
    return w.written();
}

/// 解码带 4 字节包头的 `LOCAL_INFILE_Request` (`0xFB`) 帧（零拷贝）
pub fn decodeMySqlLocalInfileRequest(wire: []const u8): MySqlLocalInfileReqView! {
    const hdr = MySqlWirePacketHeader.decode(wire) ?? {
        return err.TruncatedPayload;
    };
    const payload_len: usize = @as(usize, hdr.length);
    const total = MYSQL_PACKET_HEADER_SIZE + payload_len;
    if (wire.len < total) {
        return err.TruncatedPayload;
    }
    if (wire.len > total) {
        return err.TrailingBytes;
    }
    if (payload_len < 2) {
        return err.EmptyFilename;
    }
    if (wire[MYSQL_PACKET_HEADER_SIZE] != MYSQL_LOCAL_INFILE_HEADER) {
        return err.InvalidHeader;
    }
    return MySqlLocalInfileReqView{
        sequence_id: hdr.sequence_id,
        filename: wire[MYSQL_PACKET_HEADER_SIZE + 1 .. total],
    };
}
