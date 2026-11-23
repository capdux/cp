// Source: Absorbed from /data/dev/bun/src/http/AsyncHTTP.cp | Synced: 2026-11-23T01:59:15Z
// Pure-CP zero-allocation asynchronous HTTP task state machine, option validator
// (proxy / unix socket / keepalive / decompression / TLS policy), and simultaneous
// request concurrency limiter (aligned with `BUN_CONFIG_MAX_HTTP_REQUESTS` default 256).

/// 默认最大同时发起 HTTP 请求数（对应上游 `max_simultaneous_requests = 256`）
pub const ASYNC_HTTP_DEFAULT_MAX_SIMULTANEOUS: usize = 256;

/// 允许配置的最大并发请求数上限（防止配置异常导致资源耗尽）
pub const ASYNC_HTTP_MAX_CONFIG_LIMIT: usize = 65536;

/// 异步 HTTP 任务编排与并发限流错误集
pub err AsyncHttpError {
    InvalidStateTransition,
    ConcurrencyLimitReached,
    ActiveCountUnderflow,
    InvalidMaxRequests,
    RetryExhausted,
    InvalidUrl,
    ConflictingTransport,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn startsWithAsciiIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (toLowerAscii(s[i]) != toLowerAscii(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

/// 异步 HTTP 任务生命周期状态（直接对应上游 `AsyncHTTP.State`）
pub enum AsyncHttpState: u8 {
    pending = 0,
    scheduled = 1,
    sending = 2,
    success = 3,
    fail = 4,

    /// 是否处于终态（`success` 或 `fail`）
    fn isTerminal(): bool {
        return _ == .success || _ == .fail;
    }

    /// 是否处于活跃执行期（`scheduled` 或 `sending`）
    fn isActive(): bool {
        return _ == .scheduled || _ == .sending;
    }

    /// 返回可读状态名
    fn asLabel(): []const u8 {
        if (_ == .pending) return "pending";
        if (_ == .scheduled) return "scheduled";
        if (_ == .sending) return "sending";
        if (_ == .success) return "success";
        return "fail";
    }
}

/// 调试详细输出级别（对应上游 `HTTPVerboseLevel`）
pub enum AsyncHttpVerboseLevel: u8 {
    none = 0,
    headers = 1,
    curl = 2,
}

/// 异步 HTTP 请求配置（对应上游 `AsyncHTTP.Options`）
pub struct AsyncHttpOptions {
    http_proxy: []const u8,
    hostname: []const u8,
    unix_socket_path: []const u8,
    disable_timeout: bool,
    disable_keepalive: bool,
    disable_decompression: bool,
    reject_unauthorized: bool,
    verbose: AsyncHttpVerboseLevel,
    max_retries: u8,

    /// 默认配置（开启 TLS 校验、开启 keep-alive、开启自动解压、无代理）
    static fn defaults(): AsyncHttpOptions {
        return AsyncHttpOptions{
            http_proxy: "",
            hostname: "",
            unix_socket_path: "",
            disable_timeout: false,
            disable_keepalive: false,
            disable_decompression: false,
            reject_unauthorized: true,
            verbose: .none,
            max_retries: 0,
        };
    }

    /// 校验配置合法性（拦截同时启用 HTTP 代理与 Unix Domain Socket）
    fn validate(): bool! {
        if (_.http_proxy.len > 0 && _.unix_socket_path.len > 0) {
            return err.ConflictingTransport;
        }
        return true;
    }

    /// 是否通过 HTTP 代理转发
    fn shouldUseProxy(): bool {
        return _.http_proxy.len > 0;
    }

    /// 是否通过本地 Unix Domain Socket 连接
    fn shouldUseUnixSocket(): bool {
        return _.unix_socket_path.len > 0;
    }

    /// 是否自动解压响应体
    fn shouldDecompress(): bool {
        return !_.disable_decompression;
    }

    /// 是否启用 HTTP Keep-Alive 复用
    fn shouldKeepAlive(): bool {
        return !_.disable_keepalive;
    }
}

/// 并发请求节流与配额调度器（对应上游 `active_requests_count` + `max_simultaneous_requests`）
pub struct AsyncHttpConcurrencyLimiter {
    active_count: usize,
    max_simultaneous: usize,
    queued_count: usize,
    peak_active: usize,
    total_dispatched: u64,

    /// 按自定义并发上限初始化（范围 1..65536）
    static fn init(max_simultaneous: usize): AsyncHttpConcurrencyLimiter! {
        if (max_simultaneous == 0 || max_simultaneous > ASYNC_HTTP_MAX_CONFIG_LIMIT) {
            return err.InvalidMaxRequests;
        }
        return AsyncHttpConcurrencyLimiter{
            active_count: 0,
            max_simultaneous: max_simultaneous,
            queued_count: 0,
            peak_active: 0,
            total_dispatched: 0,
        };
    }

    /// 使用官方默认上限 256 初始化
    static fn initDefault(): AsyncHttpConcurrencyLimiter {
        return AsyncHttpConcurrencyLimiter{
            active_count: 0,
            max_simultaneous: ASYNC_HTTP_DEFAULT_MAX_SIMULTANEOUS,
            queued_count: 0,
            peak_active: 0,
            total_dispatched: 0,
        };
    }

    /// 解析 `BUN_CONFIG_MAX_HTTP_REQUESTS` 环境变量字符串并校验 1..65536 合法范围
    static fn parseMaxRequestsEnv(raw: []const u8): usize! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return err.InvalidMaxRequests;
        }
        let acc: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') {
                return err.InvalidMaxRequests;
            }
            acc = acc * 10 + ((c - '0') as usize);
            if (acc > ASYNC_HTTP_MAX_CONFIG_LIMIT) {
                return err.InvalidMaxRequests;
            }
            i += 1;
        }
        if (acc == 0) {
            return err.InvalidMaxRequests;
        }
        return acc;
    }

    /// 当前是否还有并发余量可立即调度新请求
    fn canDispatchNow(): bool {
        return _.active_count < _.max_simultaneous;
    }

    /// 返回剩余可用并发槽位数
    fn remainingHeadroom(): usize {
        if (_.active_count >= _.max_simultaneous) {
            return 0;
        }
        return _.max_simultaneous - _.active_count;
    }

    /// 尝试获取一个并发执行槽位；满载时计入等待队列并返回 `ConcurrencyLimitReached`
    fn tryAcquire(*_): bool! {
        if (_.active_count >= _.max_simultaneous) {
            _.queued_count += 1;
            return err.ConcurrencyLimitReached;
        }
        _.active_count += 1;
        _.total_dispatched += 1;
        if (_.active_count > _.peak_active) {
            _.peak_active = _.active_count;
        }
        return true;
    }

    /// 释放一个活跃请求槽位（防下溢）；返回释放后的活跃请求数
    fn release(*_): usize! {
        if (_.active_count == 0) {
            return err.ActiveCountUnderflow;
        }
        _.active_count -= 1;
        if (_.queued_count > 0) {
            _.queued_count -= 1;
        }
        return _.active_count;
    }
}

/// 单个异步 HTTP 任务状态机（对应上游 `AsyncHTTP` 编排生命周期）
pub struct AsyncHttpTask {
    id: u64,
    url: []const u8,
    state: AsyncHttpState,
    options: AsyncHttpOptions,
    attempt: u8,
    status_code: u16,

    /// 构造任务并校验 URL 协议（`http://` 或 `https://`）与传输选项
    static fn init(id: u64, url: []const u8, opts: AsyncHttpOptions): AsyncHttpTask! {
        const trimmed = trimWs(url);
        const is_http = startsWithAsciiIgnoreCase(trimmed, "http://") && trimmed.len > 7;
        const is_https = startsWithAsciiIgnoreCase(trimmed, "https://") && trimmed.len > 8;
        if (!is_http && !is_https) {
            return err.InvalidUrl;
        }
        * = opts.validate()?;
        return AsyncHttpTask{
            id: id,
            url: trimmed,
            state: .pending,
            options: opts,
            attempt: 0,
            status_code: 0,
        };
    }

    /// 判断目标 URL 是否使用 HTTPS
    fn isHttps(): bool {
        return startsWithAsciiIgnoreCase(_.url, "https://");
    }

    /// `pending -> scheduled`
    fn schedule(*_): AsyncHttpState! {
        if (_.state != .pending) {
            return err.InvalidStateTransition;
        }
        _.state = .scheduled;
        return _.state;
    }

    /// `scheduled -> sending`
    fn beginSend(*_): AsyncHttpState! {
        if (_.state != .scheduled) {
            return err.InvalidStateTransition;
        }
        _.state = .sending;
        return _.state;
    }

    /// `sending -> success`
    fn completeSuccess(*_, status_code: u16): AsyncHttpState! {
        if (_.state != .sending) {
            return err.InvalidStateTransition;
        }
        _.status_code = status_code;
        _.state = .success;
        return _.state;
    }

    /// `scheduled | sending -> fail`
    fn completeFail(*_): AsyncHttpState! {
        if (_.state != .scheduled && _.state != .sending) {
            return err.InvalidStateTransition;
        }
        _.state = .fail;
        return _.state;
    }

    /// 失败后按 `max_retries` 重置回 `pending` 准备重试
    fn resetForRetry(*_): AsyncHttpState! {
        if (_.state != .fail) {
            return err.InvalidStateTransition;
        }
        if (_.attempt >= _.options.max_retries) {
            return err.RetryExhausted;
        }
        _.attempt += 1;
        _.status_code = 0;
        _.state = .pending;
        return _.state;
    }
}
