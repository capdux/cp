// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/Channel.cp | Synced: 2026-11-22T10:29:17Z
// Pure-CP zero-allocation bidirectional IPC framing channel state machine for parallel test workers:
// - Incoming partial-read reassembly (`rx_buf`) with 5-byte LE frame header boundary detection
// - Unknown `IpcFrameKind` forward-compatible skip vs `> 64 MiB` corrupt-frame protocol abort
// - POSIX partial-write backpressure queue (`tx_buf` + `advanceTx`)
// - Windows/async double-buffered in-flight write staging (`tx_buf` -> `inflight_buf` swap)
// - Transport-attached vs detached close-reason diagnostics (`ProtocolError` vs `PeerEof` / `CleanClose`).

import {
    IPC_FRAME_HEADER_SIZE,
    IPC_FRAME_MAX_PAYLOAD,
    IpcFrameKind,
} from "./ipcFrame.cp";

/// 零分配 IPC 通道入站/出站/在途缓冲区默认容量（8 KiB）
pub const IPC_CHANNEL_BUF_CAP: usize = 8192;

/// `IpcChannel` 双向帧通道错误集
pub err IpcChannelError {
    ChannelClosed,
    RxBufferOverflow,
    TxBufferOverflow,
    PayloadBufferTooSmall,
    ProtocolCorruptFrame,
}

/// 通道关闭原因枚举（区分正常关闭、对端 EOF、协议错误与写错误）
pub enum IpcChannelCloseReason: u8 {
    Open = 0,
    CleanClose = 1,
    PeerEof = 2,
    ProtocolError = 3,
    WriteError = 4,
}

/// 单步帧轮询结果状态
pub enum IpcChannelPollStatus: u8 {
    NeedMoreData = 0,
    Frame = 1,
    Closed = 2,
}

/// `IpcChannel.pollFrame` 返回的帧轮询结果
pub struct IpcChannelPollResult {
    status: IpcChannelPollStatus = .NeedMoreData,
    kind: IpcFrameKind = .ready,
    payload_len: usize = 0,
}

fn readLeU32(s: []const u8): u32 {
    return @as(u32, s[0]) |
        (@as(u32, s[1]) << 8) |
        (@as(u32, s[2]) << 16) |
        (@as(u32, s[3]) << 24);
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn shiftLeft(buf: []u8, total_len: usize, shift: usize): usize {
    if (shift == 0) {
        return total_len;
    }
    if (shift >= total_len) {
        return 0;
    }
    const rest = total_len - shift;
    let i: usize = 0;
    while (i < rest) {
        buf[i] = buf[i + shift];
        i += 1;
    }
    return rest;
}

/// 零分配双向背压 IPC 帧通道状态机（吸收自 Bun `cli/test/parallel/Channel.cp`）
pub struct IpcChannel {
    attached: bool,
    done: bool,
    close_reason: IpcChannelCloseReason,
    rx_buf: [IPC_CHANNEL_BUF_CAP]u8,
    rx_len: usize,
    tx_buf: [IPC_CHANNEL_BUF_CAP]u8,
    tx_len: usize,
    inflight_buf: [IPC_CHANNEL_BUF_CAP]u8,
    inflight_len: usize,
    frames_decoded: usize,
    unknown_frames_skipped: usize,
    bytes_sent: usize,

    static fn init(attached: bool): IpcChannel {
        return IpcChannel{
            attached: attached,
            done: false,
            close_reason: .Open,
            rx_buf: [0; IPC_CHANNEL_BUF_CAP],
            rx_len: 0,
            tx_buf: [0; IPC_CHANNEL_BUF_CAP],
            tx_len: 0,
            inflight_buf: [0; IPC_CHANNEL_BUF_CAP],
            inflight_len: 0,
            frames_decoded: 0,
            unknown_frames_skipped: 0,
            bytes_sent: 0,
        };
    }

    /// 底层 socket/pipe 是否仍处于挂载状态
    fn isAttached(): bool {
        return _.attached;
    }

    /// 通道是否已结束
    fn isDone(): bool {
        return _.done;
    }

    /// 对齐 Bun `Channel.isAttached` 诊断契约：
    /// 若 `done == true` 且传输仍挂载（`attached == true`），说明是帧协议错误而非对端干净 EOF。
    fn isProtocolError(): bool {
        return _.done && _.attached && _.close_reason == .ProtocolError;
    }

    /// 是否有待发送或正在异步写入中的字节（对齐 Bun `Channel.hasPendingWrites`）
    fn hasPendingWrites(): bool {
        return _.tx_len > 0 || _.inflight_len > 0;
    }

    /// 当前入站缓冲中尚未凑齐完整帧的残留字节数
    fn rxBufferedLen(): usize {
        return _.rx_len;
    }

    /// 当前出站积压队列字节数
    fn txQueuedLen(): usize {
        return _.tx_len;
    }

    /// 当前异步在途写缓冲字节数
    fn inflightLen(): usize {
        return _.inflight_len;
    }

    /// 入队一段已编码的 IPC 帧字节（背压安全：缓冲区不足返回 `TxBufferOverflow`）
    fn queueSend(*_, frame_bytes: []const u8): usize! {
        if (_.done) {
            return err.ChannelClosed;
        }
        if (frame_bytes.len > IPC_CHANNEL_BUF_CAP - _.tx_len) {
            return err.TxBufferOverflow;
        }
        copyBytes(_.tx_buf[_.tx_len.._.tx_len + frame_bytes.len], frame_bytes);
        _.tx_len += frame_bytes.len;
        return _.tx_len;
    }

    /// POSIX 部分写模型：取出最多 `max_bytes` 字节准备写入内核 socket
    fn peekTxChunk(max_bytes: usize): []const u8 {
        if (_.tx_len == 0 || max_bytes == 0) {
            return "";
        }
        const n = if (max_bytes < _.tx_len) max_bytes else _.tx_len;
        return _.tx_buf[0..n];
    }

    /// POSIX 部分写回调：推进已由内核接受的 `written` 字节并前移剩余积压数据
    fn advanceTx(*_, written: usize) {
        if (written == 0) {
            return;
        }
        const actual = if (written > _.tx_len) _.tx_len else written;
        _.tx_len = shiftLeft(_.tx_buf[0..], _.tx_len, actual);
        _.bytes_sent += actual;
    }

    /// Windows / 异步双缓冲模型：若当前无在途写且 `tx_buf` 非空，将 `tx_buf` 切入 `inflight_buf`
    /// 返回稳定切片供异步写入提交；写入期间新帧可继续通过 `queueSend` 写入 `tx_buf`。
    fn submitInflight(*_): []const u8 {
        if (_.done || _.inflight_len > 0 || _.tx_len == 0) {
            return "";
        }
        copyBytes(_.inflight_buf[0.._.tx_len], _.tx_buf[0.._.tx_len]);
        _.inflight_len = _.tx_len;
        _.tx_len = 0;
        return _.inflight_buf[0.._.inflight_len];
    }

    /// Windows / 异步写完成回调：清空 `inflight_buf`；若写入失败则关闭通道并标记 `WriteError`
    fn completeInflight(*_, write_ok: bool) {
        const sent = _.inflight_len;
        _.inflight_len = 0;
        if (_.done) {
            return;
        }
        if (!write_ok) {
            _.done = true;
            _.close_reason = .WriteError;
            return;
        }
        _.bytes_sent += sent;
    }

    /// 注入从底层 socket/pipe 读到的原始字节块到入站重组缓冲
    fn ingest(*_, chunk: []const u8): usize! {
        if (_.done) {
            return err.ChannelClosed;
        }
        if (chunk.len > IPC_CHANNEL_BUF_CAP - _.rx_len) {
            return err.RxBufferOverflow;
        }
        copyBytes(_.rx_buf[_.rx_len.._.rx_len + chunk.len], chunk);
        _.rx_len += chunk.len;
        return _.rx_len;
    }

    /// 从入站重组缓冲中解码下一帧：
    /// - 不足 5 字节头或不足完整载荷时返回 `NeedMoreData`；
    /// - 载荷长度超过 `IPC_FRAME_MAX_PAYLOAD` (64 MiB) 时保持 `attached=true` 并标记 `ProtocolError`；
    /// - 未知 `IpcFrameKind` 按前向兼容契约跳过 `5 + len` 字节并继续解码下一帧。
    fn pollFrame(*_, payload_out: []u8): IpcChannelPollResult! {
        if (_.done) {
            return IpcChannelPollResult{
                status: .Closed,
                kind: .ready,
                payload_len: 0,
            };
        }

        while (_.rx_len >= IPC_FRAME_HEADER_SIZE) {
            const payload_len_u32 = readLeU32(_.rx_buf[0..4]);
            if (payload_len_u32 > IPC_FRAME_MAX_PAYLOAD) {
                // Keep `attached` untouched so caller can distinguish protocol error from clean EOF.
                _.done = true;
                _.close_reason = .ProtocolError;
                return err.ProtocolCorruptFrame;
            }
            const payload_len: usize = @intCast(payload_len_u32);
            const total_len = IPC_FRAME_HEADER_SIZE + payload_len;
            if (_.rx_len < total_len) {
                return IpcChannelPollResult{
                    status: .NeedMoreData,
                    kind: .ready,
                    payload_len: 0,
                };
            }

            const maybe_kind = IpcFrameKind.fromU8(_.rx_buf[4]);
            if (kind := maybe_kind) {
                if (payload_len > payload_out.len) {
                    return err.PayloadBufferTooSmall;
                }
                copyBytes(payload_out[0..payload_len], _.rx_buf[IPC_FRAME_HEADER_SIZE..total_len]);
                _.rx_len = shiftLeft(_.rx_buf[0..], _.rx_len, total_len);
                _.frames_decoded += 1;
                return IpcChannelPollResult{
                    status: .Frame,
                    kind: kind,
                    payload_len: payload_len,
                };
            } else {
                // Unknown frame kind: skip `5 + len` bytes for forward compatibility.
                _.rx_len = shiftLeft(_.rx_buf[0..], _.rx_len, total_len);
                _.unknown_frames_skipped += 1;
            }
        }

        return IpcChannelPollResult{
            status: .NeedMoreData,
            kind: .ready,
            payload_len: 0,
        };
    }

    /// 对端关闭/错误回调：先卸载传输句柄再标记 `done`（对齐 Bun `PosixHandlers.onClose` / `WindowsHandlers.onError`）
    fn onPeerEof(*_) {
        _.attached = false;
        if (!_.done) {
            _.done = true;
            _.close_reason = .PeerEof;
        }
    }

    /// 主动优雅关闭通道
    fn close(*_) {
        _.attached = false;
        if (!_.done) {
            _.done = true;
            _.close_reason = .CleanClose;
        }
    }
}
