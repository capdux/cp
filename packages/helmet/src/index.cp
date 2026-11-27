// @cap/helmet-headers — Zero-heap HTTP security response headers generator in pure CP
// Clean-room MIT implementation inspired by npm `helmet` v8.

pub struct HelmetOptions {
    pub csp_enabled: bool,
    pub csp_value: string,
    pub hsts_enabled: bool,
    pub hsts_max_age: u64,
    pub hsts_include_subdomains: bool,
    pub hsts_preload: bool,
    pub no_sniff: bool,
    pub frame_options: string,
    pub referrer_policy: string,
    pub coop: string,
    pub corp: string,
    pub dns_prefetch_control: string,
    pub permitted_cross_domain: string,
    pub origin_agent_cluster: bool,

    /// Strict production-grade security defaults (Helmet v8 parity).
    pub static fn defaultStrict(): HelmetOptions {
        return HelmetOptions {
            csp_enabled: true,
            csp_value: "default-src 'self'; base-uri 'self'; font-src 'self' https: data:; form-action 'self'; frame-ancestors 'self'; img-src 'self' data:; object-src 'none'; script-src 'self'; style-src 'self' https: 'unsafe-inline'; upgrade-insecure-requests",
            hsts_enabled: true,
            hsts_max_age: 31536000,
            hsts_include_subdomains: true,
            hsts_preload: false,
            no_sniff: true,
            frame_options: "DENY",
            referrer_policy: "no-referrer",
            coop: "same-origin",
            corp: "same-origin",
            dns_prefetch_control: "off",
            permitted_cross_domain: "none",
            origin_agent_cluster: true,
        };
    }

    /// Minimal baseline for APIs or development environments.
    pub static fn minimal(): HelmetOptions {
        return HelmetOptions {
            csp_enabled: false,
            csp_value: "",
            hsts_enabled: false,
            hsts_max_age: 0,
            hsts_include_subdomains: false,
            hsts_preload: false,
            no_sniff: true,
            frame_options: "SAMEORIGIN",
            referrer_policy: "strict-origin-when-cross-origin",
            coop: "",
            corp: "",
            dns_prefetch_control: "",
            permitted_cross_domain: "",
            origin_agent_cluster: false,
        };
    }
}

// ============================================================================
// Private helpers (all prefixed with `hlm` to avoid module flattening clashes)
// ============================================================================

fn hlmWriteByte(out: []u8, pos: *usize, b: u8) {
    if (pos.* < out.len) {
        out[pos.*] = b;
        pos.* = pos.* + 1;
    }
}

fn hlmWriteStr(out: []u8, pos: *usize, s: string) {
    let i: usize = 0;
    while (i < s.len && pos.* < out.len) {
        out[pos.*] = s[i];
        pos.* = pos.* + 1;
        i = i + 1;
    }
}

fn hlmWriteU64(out: []u8, pos: *usize, val: u64) {
    if (val == 0) {
        hlmWriteByte(out, pos, 48);
        return;
    }
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let v: u64 = val;
    while (v > 0 && n < 24) {
        const digit: u8 = ((v % 10) + 48) as u8;
        tmp[n] = digit;
        n = n + 1;
        v = v / 10;
    }
    while (n > 0) {
        n = n - 1;
        hlmWriteByte(out, pos, tmp[n]);
    }
}

fn hlmWriteHeaderLine(out: []u8, pos: *usize, name: string, val: string) {
    if (val.len == 0 || !isSafeHeaderValue(val)) {
        return;
    }
    hlmWriteStr(out, pos, name);
    hlmWriteStr(out, pos, ": ");
    hlmWriteStr(out, pos, val);
    hlmWriteStr(out, pos, "\r\n");
}

fn hlmAppendCspDirective(out: []u8, pos: *usize, has_any: *bool, dir_name: string, val: string) {
    if (val.len == 0 || !isSafeHeaderValue(val)) {
        return;
    }
    if (has_any.*) {
        hlmWriteStr(out, pos, "; ");
    }
    hlmWriteStr(out, pos, dir_name);
    hlmWriteByte(out, pos, 32);
    hlmWriteStr(out, pos, val);
    has_any.* = true;
}

// ============================================================================
// Public API
// ============================================================================

/// Validate an HTTP header value against CRLF / null-byte / control-char injection.
/// Allows printable ASCII/UTF-8 bytes and HTAB (`\t`); rejects `\r`, `\n`, `\0`,
/// other control bytes `< 32`, and DEL (`0x7F`).
pub fn isSafeHeaderValue(val: string): bool {
    let i: usize = 0;
    while (i < val.len) {
        const c: u8 = val[i];
        if (c == 0 || c == 10 || c == 13 || c == 127) {
            return false;
        }
        if (c < 32 && c != 9) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Format a `Strict-Transport-Security` header value (without header name or CRLF)
/// into caller-provided `out`. Returns the number of bytes written.
pub fn formatHstsHeader(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    hlmWriteStr(out, &pos, "max-age=");
    hlmWriteU64(out, &pos, max_age);
    if (include_subdomains) {
        hlmWriteStr(out, &pos, "; includeSubDomains");
    }
    if (preload) {
        hlmWriteStr(out, &pos, "; preload");
    }
    return pos;
}

/// Format a `Content-Security-Policy` header value (without header name or CRLF)
/// from the 5 core fetch directives into caller-provided `out`.
/// Empty or unsafe directive values are omitted. Returns the number of bytes written.
pub fn formatCspHeader(
    default_src: string,
    script_src: string,
    style_src: string,
    img_src: string,
    connect_src: string,
    out: []u8,
): usize {
    if (out.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let has_any: bool = false;
    hlmAppendCspDirective(out, &pos, &has_any, "default-src", default_src);
    hlmAppendCspDirective(out, &pos, &has_any, "script-src", script_src);
    hlmAppendCspDirective(out, &pos, &has_any, "style-src", style_src);
    hlmAppendCspDirective(out, &pos, &has_any, "img-src", img_src);
    hlmAppendCspDirective(out, &pos, &has_any, "connect-src", connect_src);
    return pos;
}

/// Format all enabled security headers in `opts` as a CRLF-delimited HTTP header block
/// into caller-provided `out`. Automatically drops any header whose value fails
/// `isSafeHeaderValue`. Returns the number of bytes written.
pub fn formatHelmetHeaders(opts: HelmetOptions, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    let pos: usize = 0;

    if (opts.csp_enabled) {
        hlmWriteHeaderLine(out, &pos, "Content-Security-Policy", opts.csp_value);
    }

    if (opts.hsts_enabled) {
        let hsts_buf: [96]u8 = undefined;
        const hsts_len: usize = formatHstsHeader(
            opts.hsts_max_age,
            opts.hsts_include_subdomains,
            opts.hsts_preload,
            &hsts_buf,
        );
        if (hsts_len > 0) {
            hlmWriteHeaderLine(out, &pos, "Strict-Transport-Security", hsts_buf[0..hsts_len]);
        }
    }

    if (opts.no_sniff) {
        hlmWriteHeaderLine(out, &pos, "X-Content-Type-Options", "nosniff");
    }

    hlmWriteHeaderLine(out, &pos, "X-Frame-Options", opts.frame_options);
    hlmWriteHeaderLine(out, &pos, "Referrer-Policy", opts.referrer_policy);
    hlmWriteHeaderLine(out, &pos, "Cross-Origin-Opener-Policy", opts.coop);
    hlmWriteHeaderLine(out, &pos, "Cross-Origin-Resource-Policy", opts.corp);
    hlmWriteHeaderLine(out, &pos, "X-DNS-Prefetch-Control", opts.dns_prefetch_control);
    hlmWriteHeaderLine(out, &pos, "X-Permitted-Cross-Domain-Policies", opts.permitted_cross_domain);

    if (opts.origin_agent_cluster) {
        hlmWriteHeaderLine(out, &pos, "Origin-Agent-Cluster", "?1");
    }

    return pos;
}
