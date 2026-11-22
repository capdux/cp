// Origin: Pure-CP Stdlib (Zero-Heap Multimodal AI Agent Attachment Validator & Tile Token Estimator) | Created: 2026-11-22
// Pure CP zero-heap-allocation multimodal input governance & token budget estimation engine:
//   1. `MediaPartKind` (`Text | Image | Audio | Document`), `ImageDetailMode` (`Low | High | Auto`),
//      `MultimodalRejectReason`, `MultimodalPart`, `MultimodalLimits`, `MultimodalAuditResult`
//   2. `estimateImageTokens`: exact Vision 512x512 tile scaling (Low=85; High=2048->768 short-side->85+170*tiles)
//   3. `estimateAudioTokens`: ceiling audio duration token estimation (default 32 tok/s)
//   4. `isAllowedMultimodalMime`: case-insensitive MIME whitelist (ignoring `;params`)
//   5. `auditMultimodalPayload`: unified count, byte-size, dimension/duration/page, MIME & token-budget audit

const std = @import("std");

/// Modality category for an AI Agent multimodal input attachment.
pub enum MediaPartKind {
    Text,
    Image,
    Audio,
    Document,
}

/// Vision detail resolution mode for image token estimation.
pub enum ImageDetailMode {
    Low,
    High,
    Auto,
}

/// Specific policy violation reason returned by `auditMultimodalPayload`.
pub enum MultimodalRejectReason {
    None,
    InvalidLimits,
    TooManyParts,
    DisallowedMime,
    InvalidDimensions,
    TotalBytesExceeded,
    ImageDimensionExceeded,
    AudioDurationExceeded,
    DocumentPagesExceeded,
    TokenBudgetExceeded,
}

/// Metadata descriptor for a single multimodal attachment part.
pub struct MultimodalPart {
    kind: MediaPartKind = MediaPartKind.Text;
    mime_type: string = "";
    byte_size: u64 = 0;
    width: u32 = 0;
    height: u32 = 0;
    duration_ms: u64 = 0;
    page_count: u32 = 0;
    image_detail: ImageDetailMode = ImageDetailMode.Auto;
}

/// Governance policy limits and token estimation parameters for multimodal payloads.
pub struct MultimodalLimits {
    max_parts: usize = 16;
    max_total_bytes: u64 = 20971520; // 20 MiB
    max_image_dim: u32 = 8192;
    max_audio_duration_ms: u64 = 300000; // 5 minutes
    max_document_pages: u32 = 100;
    max_total_tokens: u32 = 32768;
    audio_tokens_per_sec: u32 = 32;
    tokens_per_doc_page: u32 = 256;
    text_bytes_per_token: u32 = 4;

    static fn default(): MultimodalLimits {
        return MultimodalLimits{};
    }

    fn isValid(): bool {
        return _.max_parts > 0 &&
            _.max_total_bytes > 0 &&
            _.text_bytes_per_token > 0;
    }
}

/// Aggregated audit verdict and per-modality token breakdown.
pub struct MultimodalAuditResult {
    allowed: bool = true;
    reject_reason: MultimodalRejectReason = MultimodalRejectReason.None;
    offending_index: usize? = null;
    total_parts: usize = 0;
    total_bytes: u64 = 0;
    text_tokens: u32 = 0;
    image_tokens: u32 = 0;
    audio_tokens: u32 = 0;
    document_tokens: u32 = 0;
    total_tokens: u32 = 0;
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn asciiEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn trimMimeBase(mime_type: string): string {
    // Strip optional ";charset=..." parameters and surrounding ASCII spaces
    let end: usize = 0;
    while (end < mime_type.len && mime_type[end] != ';') : (end += 1) {}
    let start: usize = 0;
    while (start < end && (mime_type[start] == ' ' || mime_type[start] == '\t')) : (start += 1) {}
    while (end > start && (mime_type[end - 1] == ' ' || mime_type[end - 1] == '\t')) : (end -= 1) {}
    return mime_type[start..end];
}

fn ceilDivU64(num: u64, den: u64): u64 {
    if (num == 0 || den == 0) return 0;
    return ((num - 1) / den) + 1;
}

fn satAddU32(a: u32, b: u32): u32 {
    const max_u32: u32 = std.math.maxInt(u32);
    if (b > max_u32 - a) return max_u32;
    return a + b;
}

fn clampU64ToU32(v: u64): u32 {
    const max_u32: u64 = std.math.maxInt(u32) as u64;
    if (v > max_u32) return std.math.maxInt(u32);
    return v as u32;
}

/// Estimates Vision LLM token cost for an image of size `width x height`:
/// - Returns `0` if `width == 0 || height == 0`.
/// - `Low` (or `Auto` with `width <= 512 && height <= 512`): fixed `85` tokens.
/// - `High` (or larger `Auto`): fits inside `2048x2048`, scales shortest side to `768px`,
///   then counts `512x512` tiles as `85 + 170 * tiles_x * tiles_y`.
pub fn estimateImageTokens(width: u32, height: u32, detail: ImageDetailMode): u32 {
    if (width == 0 || height == 0) return 0;
    if (detail == ImageDetailMode.Low) return 85;
    if (detail == ImageDetailMode.Auto && width <= 512 && height <= 512) return 85;

    let w: f64 = width as f64;
    let h: f64 = height as f64;

    // Step 1: Fit within 2048x2048 preserving aspect ratio
    if (w > 2048.0 || h > 2048.0) {
        const long_side: f64 = if (w > h) w else h;
        const scale1: f64 = 2048.0 / long_side;
        w = w * scale1;
        h = h * scale1;
    }

    // Step 2: Scale shortest side to 768px if larger than 768px
    const short_side: f64 = if (w < h) w else h;
    if (short_side > 768.0) {
        const scale2: f64 = 768.0 / short_side;
        w = w * scale2;
        h = h * scale2;
    }

    // Step 3: Count 512x512 tiles (at least 1x1)
    let tx: u64 = @ceil(w / 512.0) as u64;
    let ty: u64 = @ceil(h / 512.0) as u64;
    if (tx == 0) tx = 1;
    if (ty == 0) ty = 1;

    return clampU64ToU32(85 + 170 * tx * ty);
}

/// Estimates audio token cost by rounding `duration_ms * rate / 1000` upward.
/// Defaults to `32` tokens/sec when `tokens_per_sec == 0`.
pub fn estimateAudioTokens(duration_ms: u64, tokens_per_sec: u32): u32 {
    if (duration_ms == 0) return 0;
    const rate: u64 = if (tokens_per_sec > 0) (tokens_per_sec as u64) else 32;
    const whole_secs: u64 = duration_ms / 1000;
    const rem_ms: u64 = duration_ms % 1000;
    const rem_toks: u64 = ceilDivU64(rem_ms * rate, 1000);
    const max_u64: u64 = std.math.maxInt(u64);
    if (whole_secs > (max_u64 - rem_toks) / rate) {
        return std.math.maxInt(u32);
    }
    return clampU64ToU32(whole_secs * rate + rem_toks);
}

/// Validates whether `mime_type` is permitted for the declared `kind` (case-insensitive,
/// ignoring optional `;params` such as `; charset=utf-8`).
pub fn isAllowedMultimodalMime(kind: MediaPartKind, mime_type: string): bool {
    const base: string = trimMimeBase(mime_type);
    if (base.len == 0) return false;

    if (kind == MediaPartKind.Image) {
        return asciiEqIgnoreCase(base, "image/png") ||
            asciiEqIgnoreCase(base, "image/jpeg") ||
            asciiEqIgnoreCase(base, "image/webp") ||
            asciiEqIgnoreCase(base, "image/gif");
    }
    if (kind == MediaPartKind.Audio) {
        return asciiEqIgnoreCase(base, "audio/wav") ||
            asciiEqIgnoreCase(base, "audio/mp3") ||
            asciiEqIgnoreCase(base, "audio/mpeg") ||
            asciiEqIgnoreCase(base, "audio/ogg") ||
            asciiEqIgnoreCase(base, "audio/flac") ||
            asciiEqIgnoreCase(base, "audio/webm");
    }
    if (kind == MediaPartKind.Document) {
        return asciiEqIgnoreCase(base, "application/pdf") ||
            asciiEqIgnoreCase(base, "text/plain") ||
            asciiEqIgnoreCase(base, "text/markdown") ||
            asciiEqIgnoreCase(base, "text/csv");
    }
    // Text
    return asciiEqIgnoreCase(base, "text/plain") ||
        asciiEqIgnoreCase(base, "text/markdown") ||
        asciiEqIgnoreCase(base, "text/csv");
}

/// Audits a batch of multimodal attachment descriptors against `limits`, checking part count,
/// MIME whitelist, total bytes, per-modality dimension/duration/page caps, and total token budget.
pub fn auditMultimodalPayload(
    parts: []const MultimodalPart,
    limits: MultimodalLimits,
): MultimodalAuditResult {
    let res = MultimodalAuditResult{
        allowed: true,
        reject_reason: MultimodalRejectReason.None,
        offending_index: null,
        total_parts: parts.len,
        total_bytes: 0,
        text_tokens: 0,
        image_tokens: 0,
        audio_tokens: 0,
        document_tokens: 0,
        total_tokens: 0,
    };

    if (!limits.isValid()) {
        res.allowed = false;
        res.reject_reason = MultimodalRejectReason.InvalidLimits;
        return res;
    }

    if (parts.len > limits.max_parts) {
        res.allowed = false;
        res.reject_reason = MultimodalRejectReason.TooManyParts;
        res.offending_index = limits.max_parts;
        return res;
    }

    let i: usize = 0;
    while (i < parts.len) : (i += 1) {
        const p: MultimodalPart = parts[i];

        if (!isAllowedMultimodalMime(p.kind, p.mime_type)) {
            res.allowed = false;
            res.reject_reason = MultimodalRejectReason.DisallowedMime;
            res.offending_index = i;
            return res;
        }

        if (p.byte_size > std.math.maxInt(u64) - res.total_bytes ||
            res.total_bytes + p.byte_size > limits.max_total_bytes) {
            res.allowed = false;
            res.reject_reason = MultimodalRejectReason.TotalBytesExceeded;
            res.offending_index = i;
            return res;
        }
        res.total_bytes += p.byte_size;

        let part_tokens: u32 = 0;
        if (p.kind == MediaPartKind.Text) {
            part_tokens = clampU64ToU32(ceilDivU64(p.byte_size, limits.text_bytes_per_token as u64));
            res.text_tokens = satAddU32(res.text_tokens, part_tokens);
        } else if (p.kind == MediaPartKind.Image) {
            if (p.width == 0 || p.height == 0) {
                res.allowed = false;
                res.reject_reason = MultimodalRejectReason.InvalidDimensions;
                res.offending_index = i;
                return res;
            }
            if (limits.max_image_dim > 0 && (p.width > limits.max_image_dim || p.height > limits.max_image_dim)) {
                res.allowed = false;
                res.reject_reason = MultimodalRejectReason.ImageDimensionExceeded;
                res.offending_index = i;
                return res;
            }
            part_tokens = estimateImageTokens(p.width, p.height, p.image_detail);
            res.image_tokens = satAddU32(res.image_tokens, part_tokens);
        } else if (p.kind == MediaPartKind.Audio) {
            if (limits.max_audio_duration_ms > 0 && p.duration_ms > limits.max_audio_duration_ms) {
                res.allowed = false;
                res.reject_reason = MultimodalRejectReason.AudioDurationExceeded;
                res.offending_index = i;
                return res;
            }
            part_tokens = estimateAudioTokens(p.duration_ms, limits.audio_tokens_per_sec);
            res.audio_tokens = satAddU32(res.audio_tokens, part_tokens);
        } else {
            // Document
            if (limits.max_document_pages > 0 && p.page_count > limits.max_document_pages) {
                res.allowed = false;
                res.reject_reason = MultimodalRejectReason.DocumentPagesExceeded;
                res.offending_index = i;
                return res;
            }
            if (p.page_count > 0) {
                part_tokens = clampU64ToU32((p.page_count as u64) * (limits.tokens_per_doc_page as u64));
            } else {
                part_tokens = clampU64ToU32(ceilDivU64(p.byte_size, limits.text_bytes_per_token as u64));
            }
            res.document_tokens = satAddU32(res.document_tokens, part_tokens);
        }

        res.total_tokens = satAddU32(res.total_tokens, part_tokens);
        if (limits.max_total_tokens > 0 && res.total_tokens > limits.max_total_tokens) {
            res.allowed = false;
            res.reject_reason = MultimodalRejectReason.TokenBudgetExceeded;
            res.offending_index = i;
            return res;
        }
    }

    return res;
}
