// Origin: Pure-CP Stdlib (AI Agent Hierarchical Span Tracer & GenAI JSON Exporter) | Created: 2026-11-22
// Pure CP zero-allocation hierarchical span tracer aligned with OpenTelemetry GenAI semantic
// conventions: fixed-pool parent/child span trees, monotonic duration clamping, aggregate
// token/cost/error rollups, and RFC-8259 escaped zero-allocation JSON trace export.

/// Sentinel parent ID indicating a root span with no parent.
pub const NO_PARENT_SPAN: u32 = 0xFFFFFFFF;

/// Semantic kind of an AI Agent execution span.
pub enum AgentSpanKind {
    AgentRun,
    LlmCall,
    ToolExec,
    Retrieval,
    Guardrail,
    Handoff,
}

/// Lifecycle status of a traced span.
pub enum AgentSpanStatus {
    Running,
    Ok,
    Error,
}

/// A single GenAI telemetry span stored inside a fixed tracer pool.
pub struct AgentSpan {
    span_id: u32 = 0,
    parent_id: u32 = NO_PARENT_SPAN,
    kind: AgentSpanKind = AgentSpanKind.AgentRun,
    name: string = "",
    model_or_tool: string = "",
    start_ms: i64 = 0,
    end_ms: i64 = 0,
    prompt_tokens: u32 = 0,
    completion_tokens: u32 = 0,
    cost_micro_usd: u64 = 0,
    status: AgentSpanStatus = AgentSpanStatus.Running,
}

/// Aggregate telemetry rollup across all spans in a trace.
pub struct TraceRollup {
    total_spans: usize = 0,
    error_spans: usize = 0,
    llm_calls: usize = 0,
    tool_calls: usize = 0,
    total_prompt_tokens: u64 = 0,
    total_completion_tokens: u64 = 0,
    total_cost_micro_usd: u64 = 0,
    total_duration_ms: i64 = 0,
}

// ============================================================================
// Internal Zero-Allocation JSON Writer Helpers
// ============================================================================

struct JsonWriter {
    buf: *[]u8,
    pos: usize = 0,

    pub fn writeByte(*_, b: u8): bool {
        if (_.pos + 1 > _.buf.len) {
            return false;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    pub fn writeBytes(*_, s: string): bool {
        if (_.pos + s.len > _.buf.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            _.buf[_.pos + i] = s[i];
        }
        _.pos += s.len;
        return true;
    }

    pub fn writeU64(*_, val: u64): bool {
        if (val == 0) {
            return _.writeByte('0');
        }
        let tmp: [24]u8 = undefined;
        let len: usize = 0;
        let v = val;
        while (v > 0) {
            tmp[len] = ('0' as u8) + ((v % 10) as u8);
            len += 1;
            v /= 10;
        }
        if (_.pos + len > _.buf.len) {
            return false;
        }
        let i: usize = 0;
        while (i < len) : (i += 1) {
            _.buf[_.pos + i] = tmp[len - 1 - i];
        }
        _.pos += len;
        return true;
    }

    pub fn writeI64(*_, val: i64): bool {
        if (val < 0) {
            if (!_.writeByte('-')) return false;
            // Handle i64::MIN safely via unsigned magnitude
            const mag: u64 = if (val == -9223372036854775807 - 1) {
                ((0x80000000 as u64) << 32)
            } else {
                (-val) as u64
            };
            return _.writeU64(mag);
        }
        return _.writeU64(val as u64);
    }

    pub fn writeJsonString(*_, s: string): bool {
        if (!_.writeByte('"')) return false;
        const hex: string = "0123456789abcdef";
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const b = s[i];
            if (b == '"') {
                if (!_.writeBytes("\\\"")) return false;
            } else if (b == '\\') {
                if (!_.writeBytes("\\\\")) return false;
            } else if (b == '\n') {
                if (!_.writeBytes("\\n")) return false;
            } else if (b == '\r') {
                if (!_.writeBytes("\\r")) return false;
            } else if (b == '\t') {
                if (!_.writeBytes("\\t")) return false;
            } else if (b < 0x20) {
                if (!_.writeBytes("\\u00")) return false;
                if (!_.writeByte(hex[(b >> 4) as usize])) return false;
                if (!_.writeByte(hex[(b & 0x0F) as usize])) return false;
            } else {
                if (!_.writeByte(b)) return false;
            }
        }
        return _.writeByte('"');
    }
}

fn kindToStr(k: AgentSpanKind): string {
    if (k == AgentSpanKind.AgentRun) return "agent_run";
    if (k == AgentSpanKind.LlmCall) return "llm_call";
    if (k == AgentSpanKind.ToolExec) return "tool_exec";
    if (k == AgentSpanKind.Retrieval) return "retrieval";
    if (k == AgentSpanKind.Guardrail) return "guardrail";
    return "handoff";
}

fn statusToStr(s: AgentSpanStatus): string {
    if (s == AgentSpanStatus.Running) return "running";
    if (s == AgentSpanStatus.Ok) return "ok";
    return "error";
}

// ============================================================================
// Public Hierarchical Agent Tracer
// ============================================================================

/// Zero-allocation fixed-pool hierarchical GenAI span tracer.
pub struct AgentTracer {
    trace_id: u64 = 0,
    spans: *[]AgentSpan,
    len: usize = 0,
    next_span_id: u32 = 1,

    /// Initializes a tracer backed by caller-provided `spans` buffer.
    pub static fn init(trace_id: u64, spans: *[]AgentSpan): AgentTracer {
        return AgentTracer{
            trace_id: trace_id,
            spans: spans,
            len: 0,
            next_span_id: 1,
        };
    }

    fn findIndex(span_id: u32): usize? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.spans[i].span_id == span_id) {
                return i;
            }
        }
        return null;
    }

    /// Starts a new span under `parent_id` (`NO_PARENT_SPAN` for root spans).
    /// Returns the newly assigned `span_id`, or `null` if `parent_id` does not exist
    /// or the fixed span pool is full.
    pub fn startSpan(
        *_,
        parent_id: u32,
        kind: AgentSpanKind,
        name: string,
        model_or_tool: string,
        now_ms: i64,
    ): u32? {
        if (_.len >= _.spans.len) {
            return null;
        }
        if (_.next_span_id == NO_PARENT_SPAN) {
            return null;
        }
        if (parent_id != NO_PARENT_SPAN && _.findIndex(parent_id) == null) {
            return null;
        }

        const id = _.next_span_id;
        _.next_span_id += 1;
        _.spans[_.len] = AgentSpan{
            span_id: id,
            parent_id: parent_id,
            kind: kind,
            name: name,
            model_or_tool: model_or_tool,
            start_ms: now_ms,
            end_ms: now_ms,
            prompt_tokens: 0,
            completion_tokens: 0,
            cost_micro_usd: 0,
            status: AgentSpanStatus.Running,
        };
        _.len += 1;
        return id;
    }

    /// Completes a `Running` span with terminal `status` (`Ok` or `Error`), token usage,
    /// micro-USD cost, and timestamp `now_ms` (clamped to `>= start_ms`).
    /// Returns `false` if `span_id` is unknown, already finished, or `status == Running`.
    pub fn endSpan(
        *_,
        span_id: u32,
        status: AgentSpanStatus,
        prompt_tokens: u32,
        completion_tokens: u32,
        cost_micro_usd: u64,
        now_ms: i64,
    ): bool {
        if (status == AgentSpanStatus.Running) {
            return false;
        }
        if (idx := _.findIndex(span_id)) {
            if (_.spans[idx].status != AgentSpanStatus.Running) {
                return false;
            }
            const eff_end: i64 = if (now_ms < _.spans[idx].start_ms) _.spans[idx].start_ms else now_ms;
            _.spans[idx].status = status;
            _.spans[idx].end_ms = eff_end;
            _.spans[idx].prompt_tokens = prompt_tokens;
            _.spans[idx].completion_tokens = completion_tokens;
            _.spans[idx].cost_micro_usd = cost_micro_usd;
            return true;
        }
        return false;
    }

    /// Computes aggregate token usage, micro-USD cost, span counts, error count, and
    /// end-to-end trace duration across all recorded spans.
    pub fn rollup(): TraceRollup {
        if (_.len == 0) {
            return TraceRollup{};
        }

        let res = TraceRollup{
            total_spans: _.len,
            error_spans: 0,
            llm_calls: 0,
            tool_calls: 0,
            total_prompt_tokens: 0,
            total_completion_tokens: 0,
            total_cost_micro_usd: 0,
            total_duration_ms: 0,
        };

        let min_start: i64 = _.spans[0].start_ms;
        let max_end: i64 = _.spans[0].end_ms;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const sp = _.spans[i];
            if (sp.status == AgentSpanStatus.Error) {
                res.error_spans += 1;
            }
            if (sp.kind == AgentSpanKind.LlmCall) {
                res.llm_calls += 1;
            }
            if (sp.kind == AgentSpanKind.ToolExec) {
                res.tool_calls += 1;
            }
            res.total_prompt_tokens += sp.prompt_tokens as u64;
            res.total_completion_tokens += sp.completion_tokens as u64;
            res.total_cost_micro_usd += sp.cost_micro_usd;

            if (sp.start_ms < min_start) {
                min_start = sp.start_ms;
            }
            if (sp.end_ms > max_end) {
                max_end = sp.end_ms;
            }
        }

        res.total_duration_ms = if (max_end > min_start) max_end - min_start else 0;
        return res;
    }

    /// Exports the complete trace (including `trace_id`, `rollup`, and all `spans`) as
    /// RFC-8259 escaped JSON into `out_buf`.
    /// Returns bytes written, or `null` if `out_buf` is too small.
    pub fn exportJson(out_buf: *[]u8): usize? {
        let w = JsonWriter{ buf: out_buf, pos: 0 };
        const r = _.rollup();

        if (!w.writeBytes("{\"trace_id\":")) return null;
        if (!w.writeU64(_.trace_id)) return null;

        if (!w.writeBytes(",\"rollup\":{\"total_spans\":")) return null;
        if (!w.writeU64(r.total_spans as u64)) return null;
        if (!w.writeBytes(",\"error_spans\":")) return null;
        if (!w.writeU64(r.error_spans as u64)) return null;
        if (!w.writeBytes(",\"llm_calls\":")) return null;
        if (!w.writeU64(r.llm_calls as u64)) return null;
        if (!w.writeBytes(",\"tool_calls\":")) return null;
        if (!w.writeU64(r.tool_calls as u64)) return null;
        if (!w.writeBytes(",\"total_prompt_tokens\":")) return null;
        if (!w.writeU64(r.total_prompt_tokens)) return null;
        if (!w.writeBytes(",\"total_completion_tokens\":")) return null;
        if (!w.writeU64(r.total_completion_tokens)) return null;
        if (!w.writeBytes(",\"total_cost_micro_usd\":")) return null;
        if (!w.writeU64(r.total_cost_micro_usd)) return null;
        if (!w.writeBytes(",\"total_duration_ms\":")) return null;
        if (!w.writeI64(r.total_duration_ms)) return null;
        if (!w.writeBytes("},\"spans\":[")) return null;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (i > 0) {
                if (!w.writeByte(',')) return null;
            }
            const sp = _.spans[i];
            const dur: i64 = if (sp.end_ms > sp.start_ms) sp.end_ms - sp.start_ms else 0;

            if (!w.writeBytes("{\"span_id\":")) return null;
            if (!w.writeU64(sp.span_id as u64)) return null;
            if (!w.writeBytes(",\"parent_id\":")) return null;
            if (sp.parent_id == NO_PARENT_SPAN) {
                if (!w.writeBytes("null")) return null;
            } else {
                if (!w.writeU64(sp.parent_id as u64)) return null;
            }
            if (!w.writeBytes(",\"kind\":")) return null;
            if (!w.writeJsonString(kindToStr(sp.kind))) return null;
            if (!w.writeBytes(",\"name\":")) return null;
            if (!w.writeJsonString(sp.name)) return null;
            if (!w.writeBytes(",\"model_or_tool\":")) return null;
            if (!w.writeJsonString(sp.model_or_tool)) return null;
            if (!w.writeBytes(",\"start_ms\":")) return null;
            if (!w.writeI64(sp.start_ms)) return null;
            if (!w.writeBytes(",\"end_ms\":")) return null;
            if (!w.writeI64(sp.end_ms)) return null;
            if (!w.writeBytes(",\"duration_ms\":")) return null;
            if (!w.writeI64(dur)) return null;
            if (!w.writeBytes(",\"prompt_tokens\":")) return null;
            if (!w.writeU64(sp.prompt_tokens as u64)) return null;
            if (!w.writeBytes(",\"completion_tokens\":")) return null;
            if (!w.writeU64(sp.completion_tokens as u64)) return null;
            if (!w.writeBytes(",\"cost_micro_usd\":")) return null;
            if (!w.writeU64(sp.cost_micro_usd)) return null;
            if (!w.writeBytes(",\"status\":")) return null;
            if (!w.writeJsonString(statusToStr(sp.status))) return null;
            if (!w.writeByte('}')) return null;
        }

        if (!w.writeBytes("]}")) return null;
        return w.pos;
    }
}
