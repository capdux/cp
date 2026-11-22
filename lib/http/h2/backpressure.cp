// Source: Absorbed from /data/dev/bun/src/http/H2Client.cp | Synced: 2026-11-22T21:22:51Z
// Pure-CP zero-allocation HTTP/2 client write-buffer backpressure budget,
// hysteresis watermarks, and live session/stream coordinator metrics.

/// 本地初始流控窗口大小（16 MiB，对齐 Bun `H2Client.local_initial_window_size`）
pub const H2_LOCAL_INITIAL_WINDOW_SIZE: u32 = 16 * 1024 * 1024;

/// 本地最大响应头列表字节上限（256 KiB，对齐 Bun `H2Client.local_max_header_list_size`）
pub const H2_LOCAL_MAX_HEADER_LIST_SIZE: u32 = 256 * 1024;

/// DATA 帧写缓冲背压高水位（256 KiB，对齐 Bun `H2Client.write_buffer_high_water`）
pub const H2_WRITE_BUFFER_HIGH_WATER: usize = 256 * 1024;

/// 控制帧（SETTINGS/PING/RST_STREAM/GOAWAY）绝对硬上限（1 MiB，对齐 Bun `H2Client.write_buffer_control_limit`）
pub const H2_WRITE_BUFFER_CONTROL_LIMIT: usize = 1024 * 1024;

/// HTTP/2 背压与指标账本错误集
pub err H2BackpressureError {
    InvalidThresholds,
    ControlBufferExceeded,
    CounterUnderflow,
}

/// 写缓冲准入判定结果
pub enum H2WriteAdmission {
    Allow,
    ApplyBackpressure,
    DropConnection,
}

/// 零堆分配 HTTP/2 写缓冲双水位背压预算控制器（含 50% 滞回解除防抖动）
pub struct H2WriteBufferBudget {
    queued_bytes: usize = 0,
    high_water: usize = H2_WRITE_BUFFER_HIGH_WATER,
    control_limit: usize = H2_WRITE_BUFFER_CONTROL_LIMIT,
    backpressure_active: bool = false,
    peak_queued_bytes: usize = 0,
    backpressure_events: u64 = 0,

    /// 使用 Bun 生产默认水位（256 KiB 高水位 / 1 MiB 控制上限）初始化
    static fn initDefault(): H2WriteBufferBudget {
        return H2WriteBufferBudget{};
    }

    /// 使用自定义双水位初始化（要求 `0 < high_water <= control_limit`）
    static fn init(high_water: usize, control_limit: usize): H2BackpressureError!H2WriteBufferBudget {
        if (high_water == 0 || high_water > control_limit) {
            return H2BackpressureError.InvalidThresholds;
        }
        return H2WriteBufferBudget{
            queued_bytes: 0,
            high_water: high_water,
            control_limit: control_limit,
            backpressure_active: false,
            peak_queued_bytes: 0,
            backpressure_events: 0,
        };
    }

    /// 判断当前是否允许继续提交普通 DATA 帧
    fn canAcceptData(): bool {
        return !_.backpressure_active && _.queued_bytes < _.high_water;
    }

    /// 返回距离 DATA 高水位剩余的可用字节预算
    fn remainingDataBudget(): usize {
        if (_.queued_bytes >= _.high_water) {
            return 0;
        }
        return _.high_water - _.queued_bytes;
    }

    /// 返回距离控制帧绝对硬上限的剩余字节预算
    fn remainingControlBudget(): usize {
        if (_.queued_bytes >= _.control_limit) {
            return 0;
        }
        return _.control_limit - _.queued_bytes;
    }

    /// 记录普通 DATA 帧入队字节数，返回准入策略（达到高水位触发背压，超过控制硬上限建议断连）
    fn queueData(*_, bytes: usize): H2WriteAdmission {
        if (bytes > _.control_limit || _.queued_bytes > _.control_limit - bytes) {
            return H2WriteAdmission.DropConnection;
        }
        _.queued_bytes += bytes;
        if (_.queued_bytes > _.peak_queued_bytes) {
            _.peak_queued_bytes = _.queued_bytes;
        }
        if (_.queued_bytes >= _.high_water) {
            if (!_.backpressure_active) {
                _.backpressure_active = true;
                _.backpressure_events += 1;
            }
            return H2WriteAdmission.ApplyBackpressure;
        }
        return H2WriteAdmission.Allow;
    }

    /// 记录高优先级控制帧（SETTINGS/PING/WINDOW_UPDATE/RST_STREAM）入队；允许越过 DATA 高水位但严禁越过控制硬上限
    fn queueControl(*_, bytes: usize): H2BackpressureError!H2WriteAdmission {
        if (bytes > _.control_limit || _.queued_bytes > _.control_limit - bytes) {
            return H2BackpressureError.ControlBufferExceeded;
        }
        _.queued_bytes += bytes;
        if (_.queued_bytes > _.peak_queued_bytes) {
            _.peak_queued_bytes = _.queued_bytes;
        }
        if (_.queued_bytes >= _.high_water) {
            if (!_.backpressure_active) {
                _.backpressure_active = true;
                _.backpressure_events += 1;
            }
            return H2WriteAdmission.ApplyBackpressure;
        }
        return H2WriteAdmission.Allow;
    }

    /// 记录底层 socket 成功刷出 `bytes` 字节；若背压状态由此解除（回落至 `high_water / 2` 及以下）返回 `true`
    fn onFlushed(*_, bytes: usize): bool {
        if (bytes >= _.queued_bytes) {
            _.queued_bytes = 0;
        } else {
            _.queued_bytes -= bytes;
        }
        const low_water = _.high_water / 2;
        if (_.backpressure_active && _.queued_bytes <= low_water) {
            _.backpressure_active = false;
            return true;
        }
        return false;
    }

    /// 返回当前写缓冲相对 `control_limit` 的占用百分比（0..100）
    fn utilizationPercent(): u8 {
        if (_.control_limit == 0) {
            return 100;
        }
        const pct = (_.queued_bytes * 100) / _.control_limit;
        if (pct > 100) {
            return 100;
        }
        return @as(u8, @intCast(pct));
    }
}

/// 零堆分配 HTTP/2 客户端活跃会话与流协调器账本（对齐 Bun `H2Client.live_sessions` / `live_streams`）
pub struct H2ClientMetrics {
    live_sessions: u32 = 0,
    live_streams: u32 = 0,
    peak_sessions: u32 = 0,
    peak_streams: u32 = 0,
    total_sessions_opened: u64 = 0,
    total_streams_opened: u64 = 0,

    static fn init(): H2ClientMetrics {
        return H2ClientMetrics{};
    }

    fn onSessionOpened(*_): u32 {
        _.live_sessions += 1;
        _.total_sessions_opened += 1;
        if (_.live_sessions > _.peak_sessions) {
            _.peak_sessions = _.live_sessions;
        }
        return _.live_sessions;
    }

    fn onSessionClosed(*_): H2BackpressureError!u32 {
        if (_.live_sessions == 0) {
            return H2BackpressureError.CounterUnderflow;
        }
        _.live_sessions -= 1;
        return _.live_sessions;
    }

    fn onStreamOpened(*_): u32 {
        _.live_streams += 1;
        _.total_streams_opened += 1;
        if (_.live_streams > _.peak_streams) {
            _.peak_streams = _.live_streams;
        }
        return _.live_streams;
    }

    fn onStreamClosed(*_): H2BackpressureError!u32 {
        if (_.live_streams == 0) {
            return H2BackpressureError.CounterUnderflow;
        }
        _.live_streams -= 1;
        return _.live_streams;
    }

    /// 判断是否全部会话与流均已归零（安全退出/资源泄漏断言用）
    fn isIdle(): bool {
        return _.live_sessions == 0 && _.live_streams == 0;
    }
}
