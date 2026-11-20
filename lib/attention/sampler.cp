// Source: cp/lib/attention/sampler.cp (LLM Logits Sampler: Greedy, Temp, RepPenalty, Top-K, Top-P)
const std = @import("std");
import { AttnError } from "./sdpa.cp";

pub struct SamplingConfig {
    pub temperature: f32 = 1.0,
    pub top_k: usize = 0,
    pub top_p: f32 = 1.0,
    pub repetition_penalty: f32 = 1.0,
    pub seed: u64 = 42,
}

struct Candidate {
    idx: usize,
    val: f32,
}

fn candidateDesc(_: void, a: Candidate, b: Candidate): bool {
    return a.val > b.val;
}

pub fn sampleGreedy(logits: []const f32): AttnError!usize {
    if (logits.len == 0) {
        return AttnError.EmptyInput;
    }
    let best_idx: usize = 0;
    let best_val: f32 = logits[0];
    let i: usize = 1;
    while (i < logits.len) : (i += 1) {
        if (logits[i] > best_val) {
            best_val = logits[i];
            best_idx = i;
        }
    }
    return best_idx;
}

pub fn sampleToken(
    alloc: std.mem.Allocator,
    logits: []const f32,
    recent_tokens: []const usize,
    config: SamplingConfig,
): AttnError!usize {
    if (logits.len == 0) {
        return AttnError.EmptyInput;
    }
    if (config.temperature < 0.0) {
        return AttnError.InvalidTemperature;
    }
    if (config.top_p <= 0.0 or config.top_p > 1.0) {
        return AttnError.InvalidTopP;
    }

    let work = alloc.alloc(f32, logits.len) catch return AttnError.OutOfMemory;
    defer alloc.free(work);
    @memcpy(work, logits);

    if (config.repetition_penalty != 1.0 and recent_tokens.len > 0) {
        let seen = alloc.alloc(bool, logits.len) catch return AttnError.OutOfMemory;
        defer alloc.free(seen);
        @memset(seen, false);

        let r: usize = 0;
        while (r < recent_tokens.len) : (r += 1) {
            const tok = recent_tokens[r];
            if (tok < work.len and !seen[tok]) {
                seen[tok] = true;
                if (work[tok] > 0.0) {
                    work[tok] = work[tok] / config.repetition_penalty;
                } else {
                    work[tok] = work[tok] * config.repetition_penalty;
                }
            }
        }
    }

    if (config.temperature == 0.0) {
        return sampleGreedy(work);
    }

    const inv_temp: f32 = 1.0 / config.temperature;
    let i: usize = 0;
    while (i < work.len) : (i += 1) {
        work[i] *= inv_temp;
    }

    let candidates = std.ArrayList(Candidate).empty;
    defer candidates.deinit(alloc);

    i = 0;
    while (i < work.len) : (i += 1) {
        candidates.append(alloc, Candidate{ .idx = i, .val = work[i] }) catch return AttnError.OutOfMemory;
    }

    std.sort.pdq(Candidate, candidates.items, {}, candidateDesc);

    let active_len: usize = candidates.items.len;
    if (config.top_k > 0 and config.top_k < active_len) {
        active_len = config.top_k;
    }

    const max_logit = candidates.items[0].val;
    let exp_sum: f32 = 0.0;
    i = 0;
    while (i < active_len) : (i += 1) {
        const w = @exp(candidates.items[i].val - max_logit);
        candidates.items[i].val = w;
        exp_sum += w;
    }

    const inv_sum: f32 = if (exp_sum > 0.0) 1.0 / exp_sum else 0.0;
    i = 0;
    while (i < active_len) : (i += 1) {
        candidates.items[i].val *= inv_sum;
    }

    if (config.top_p < 1.0) {
        let cum_prob: f32 = 0.0;
        let cutoff: usize = active_len;
        i = 0;
        while (i < active_len) : (i += 1) {
            cum_prob += candidates.items[i].val;
            if (cum_prob >= config.top_p) {
                cutoff = @as(usize, i);
                cutoff += 1;
                break;
            }
        }
        active_len = cutoff;
    }

    let kept_sum: f32 = 0.0;
    i = 0;
    while (i < active_len) : (i += 1) {
        kept_sum += candidates.items[i].val;
    }

    let z: u64 = config.seed +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    z = z ^ (z >> 31);
    const u: f32 = @as(f32, @floatFromInt(z >> 40)) / 16777216.0;
    const target: f32 = u * kept_sum;

    let acc: f32 = 0.0;
    i = 0;
    while (i < active_len) : (i += 1) {
        acc += candidates.items[i].val;
        if (acc >= target) {
            return candidates.items[i].idx;
        }
    }

    return candidates.items[active_len - 1].idx;
}
