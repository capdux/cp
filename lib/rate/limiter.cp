// Source: Absorbed from /data/dev/bun/src/http/HTTPContext.cp | Synced: 2026-11-22T07:42:24Z (Token Bucket & Sliding Window Rate Limiter)

const U64_MAX: u64 = 0xFFFF_FFFF_FFFF_FFFF;

fn satAddU64(a: u64, b: u64): u64 {
    if (a > U64_MAX - b) {
        return U64_MAX;
    }
    return a + b;
}

fn satMulU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > U64_MAX / b) {
        return U64_MAX;
    }
    return a * b;
}

/// 纯整数毫令牌确定性令牌桶限流器（零外部依赖、零堆分配、亚秒级零精度损失）
/// 内部以 1 token = 1000 milli-tokens 表示，因此每经过 1 ms 恰好补充 refill_per_sec 个毫令牌
export struct SimpleTokenBucket {
    capacity_milli: u64;
    tokens_milli: u64;
    refill_per_sec: u64;
    last_refill_ms: u64;

    /// 创建初始满桶的令牌桶限流器
    fn init(capacity: u64, refill_per_sec: u64, now_ms: u64): SimpleTokenBucket {
        const cap_milli = satMulU64(capacity, 1000);
        return SimpleTokenBucket{
            capacity_milli: cap_milli,
            tokens_milli: cap_milli,
            refill_per_sec: refill_per_sec,
            last_refill_ms: now_ms,
        };
    }

    /// 创建指定初始令牌数的令牌桶限流器（initial_tokens 自动钳制至 <= capacity）
    fn initWithTokens(capacity: u64, initial_tokens: u64, refill_per_sec: u64, now_ms: u64): SimpleTokenBucket {
        const cap_milli = satMulU64(capacity, 1000);
        const clamped = if (initial_tokens > capacity) capacity else initial_tokens;
        return SimpleTokenBucket{
            capacity_milli: cap_milli,
            tokens_milli: satMulU64(clamped, 1000),
            refill_per_sec: refill_per_sec,
            last_refill_ms: now_ms,
        };
    }

    /// 按当前时间戳补充令牌（防时钟回拨、防乘法/加法溢出，封顶至 capacity）
    fn refill(*_, now_ms: u64) {
        if (now_ms <= _.last_refill_ms) {
            return;
        }
        const elapsed_ms = now_ms - _.last_refill_ms;
        _.last_refill_ms = now_ms;
        if (_.refill_per_sec == 0 || _.tokens_milli >= _.capacity_milli) {
            return;
        }
        const added_milli = satMulU64(elapsed_ms, _.refill_per_sec);
        const next_milli = satAddU64(_.tokens_milli, added_milli);
        _.tokens_milli = if (next_milli > _.capacity_milli) _.capacity_milli else next_milli;
    }

    /// 先补充令牌再返回当前可用整数令牌数
    fn available(*_, now_ms: u64): u64 {
        _.refill(now_ms);
        return _.tokens_milli / 1000;
    }

    /// 尝试消费指定数量令牌；足够则扣减并返回 true，否则不扣减并返回 false
    fn tryConsume(*_, tokens: u64, now_ms: u64): bool {
        _.refill(now_ms);
        if (tokens == 0) {
            return true;
        }
        if (tokens > U64_MAX / 1000) {
            return false;
        }
        const need_milli = tokens * 1000;
        if (_.tokens_milli < need_milli) {
            return false;
        }
        _.tokens_milli -= need_milli;
        return true;
    }

    /// 计算凑足 tokens 个令牌所需等待的最少毫秒数；若超容量或 refill_per_sec == 0 且不足则返回 null
    fn timeUntilReadyMs(*_, tokens: u64, now_ms: u64): u64? {
        _.refill(now_ms);
        if (tokens == 0) {
            return 0;
        }
        if (tokens > U64_MAX / 1000) {
            return null;
        }
        const need_milli = tokens * 1000;
        if (need_milli > _.capacity_milli) {
            return null;
        }
        if (_.tokens_milli >= need_milli) {
            return 0;
        }
        if (_.refill_per_sec == 0) {
            return null;
        }
        const deficit_milli = need_milli - _.tokens_milli;
        return (deficit_milli - 1) / _.refill_per_sec + 1;
    }
}

/// 固定时间窗口限流器（零外部依赖、零堆分配、防时钟回拨绕过）
export struct FixedWindowLimiter {
    limit: u64;
    window_ms: u64;
    window_start_ms: u64;
    used: u64;

    /// 初始化固定窗口限流器
    fn init(limit: u64, window_ms: u64, now_ms: u64): FixedWindowLimiter {
        return FixedWindowLimiter{
            limit: limit,
            window_ms: window_ms,
            window_start_ms: now_ms,
            used: 0,
        };
    }

    /// 尝试在当前时间窗口放行一次请求；超限返回 false
    fn allow(*_, now_ms: u64): bool {
        if (_.window_ms > 0 && now_ms >= _.window_start_ms) {
            const elapsed = now_ms - _.window_start_ms;
            if (elapsed >= _.window_ms) {
                const steps = elapsed / _.window_ms;
                _.window_start_ms = satAddU64(_.window_start_ms, satMulU64(steps, _.window_ms));
                _.used = 0;
            }
        }
        if (_.used >= _.limit) {
            return false;
        }
        _.used += 1;
        return true;
    }

    /// 查询当前时间窗口剩余可放行配额（只读，不修改内部状态）
    fn remaining(now_ms: u64): u64 {
        if (_.window_ms > 0 && now_ms >= _.window_start_ms) {
            if (now_ms - _.window_start_ms >= _.window_ms) {
                return _.limit;
            }
        }
        if (_.used >= _.limit) {
            return 0;
        }
        return _.limit - _.used;
    }

    /// 查询距离当前时间窗口重置还剩多少毫秒（只读）
    fn resetAfterMs(now_ms: u64): u64 {
        if (_.window_ms == 0) {
            return 0;
        }
        if (now_ms < _.window_start_ms) {
            return satAddU64(_.window_start_ms - now_ms, _.window_ms);
        }
        const elapsed = (now_ms - _.window_start_ms) % _.window_ms;
        return _.window_ms - elapsed;
    }
}
