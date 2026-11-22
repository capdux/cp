// Origin: Pure-CP Stdlib (Zero-Heap Agent Prompt Composer for System/Tools/Memory/RAG/User) | Created: 2026-11-22
// Pure CP zero-heap-allocation prompt orchestrator assembling SystemRole, ToolCatalog, MemoryContext,
// RagContext, and UserQuery sections under a hard `max_tokens` budget with priority selection,
// proportional over-budget truncation, canonical LLM section ordering, and detailed build telemetry.

pub const MAX_COMPOSER_BLOCKS: usize = 64;

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

/// Semantic section role inside an `AgentPromptComposer`.
pub enum AgentPromptSection {
    SystemRole,
    MemoryContext,
    RagContext,
    ToolCatalog,
    UserQuery,
}

fn sectionHeader(sec: AgentPromptSection): []const u8 {
    match (sec) {
        AgentPromptSection.SystemRole => return "## System\n",
        AgentPromptSection.ToolCatalog => return "## Tools\n",
        AgentPromptSection.MemoryContext => return "## Memory\n",
        AgentPromptSection.RagContext => return "## Context\n",
        AgentPromptSection.UserQuery => return "## User\n",
    }
}

fn sectionRank(sec: AgentPromptSection): u8 {
    match (sec) {
        AgentPromptSection.SystemRole => return 0,
        AgentPromptSection.ToolCatalog => return 1,
        AgentPromptSection.MemoryContext => return 2,
        AgentPromptSection.RagContext => return 3,
        AgentPromptSection.UserQuery => return 4,
    }
}

/// Single caller-owned section slot stored in `AgentPromptComposer`.
pub struct AgentPromptBlock {
    section: AgentPromptSection = AgentPromptSection.SystemRole;
    priority: u8 = 0;
    token_count: u32 = 0;
    content: string = "";
    occupied: bool = false;
}

/// Detailed telemetry returned by `AgentPromptComposer.build`.
pub struct AgentPromptBuildReport {
    bytes_written: usize = 0;
    used_tokens: u32 = 0;
    max_tokens: u32 = 0;
    included_sections: usize = 0;
    truncated_sections: usize = 0;
    dropped_sections: usize = 0;
    system_blocks: usize = 0;
    memory_blocks: usize = 0;
    rag_blocks: usize = 0;
    tool_blocks: usize = 0;
    user_blocks: usize = 0;
}

/// Zero-heap-allocation priority-budgeted Agent prompt composer.
pub struct AgentPromptComposer {
    blocks: []AgentPromptBlock;
    count: usize = 0;
    max_tokens: u32 = 0;

    /// Initializes a prompt composer over caller-owned `blocks_buf` with a hard `max_tokens` budget.
    /// Returns `null` if `blocks_buf.len == 0`, `blocks_buf.len > MAX_COMPOSER_BLOCKS`, or `max_tokens == 0`.
    static fn init(
        blocks_buf: *[]AgentPromptBlock,
        max_tokens: u32,
    ): AgentPromptComposer? {
        if (blocks_buf.len == 0 || blocks_buf.len > MAX_COMPOSER_BLOCKS || max_tokens == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < blocks_buf.len) : (i += 1) {
            blocks_buf[i] = AgentPromptBlock{};
        }
        return AgentPromptComposer{
            blocks: blocks_buf,
            count: 0,
            max_tokens: max_tokens,
        };
    }

    /// Appends a prompt section block with explicit `priority` (higher = allocated budget first).
    /// If `token_count == 0`, estimates tokens as `ceil(content.len / 4)`.
    fn addSection(
        *_,
        section: AgentPromptSection,
        content: string,
        token_count: u32,
        priority: u8,
    ): bool {
        if (content.len == 0 || _.count >= _.blocks.len) return false;
        const est_tokens: u32 = if (token_count > 0)
            token_count
        else
            ((content.len + 3) / 4) as u32;

        _.blocks[_.count] = AgentPromptBlock{
            section: section,
            priority: priority,
            token_count: if (est_tokens == 0) 1 else est_tokens,
            content: content,
            occupied: true,
        };
        _.count += 1;
        return true;
    }

    /// Convenience helper adding a `SystemRole` block (priority 100).
    fn addSystemRole(*_, content: string, token_count: u32): bool {
        return _.addSection(AgentPromptSection.SystemRole, content, token_count, 100);
    }

    /// Convenience helper adding a `UserQuery` block (priority 95).
    fn addUserQuery(*_, content: string, token_count: u32): bool {
        return _.addSection(AgentPromptSection.UserQuery, content, token_count, 95);
    }

    /// Convenience helper adding a `ToolCatalog` block (priority 80).
    fn addToolCatalog(*_, content: string, token_count: u32): bool {
        return _.addSection(AgentPromptSection.ToolCatalog, content, token_count, 80);
    }

    /// Convenience helper adding a `RagContext` block (priority 70).
    fn addRagContext(*_, content: string, token_count: u32): bool {
        return _.addSection(AgentPromptSection.RagContext, content, token_count, 70);
    }

    /// Convenience helper adding a `MemoryContext` block (priority 60).
    fn addMemoryContext(*_, content: string, token_count: u32): bool {
        return _.addSection(AgentPromptSection.MemoryContext, content, token_count, 60);
    }

    /// Selects sections by descending priority under `max_tokens` (proportionally truncating
    /// partial fits when remaining tokens > 0) and renders them in canonical LLM order
    /// (`SystemRole -> ToolCatalog -> MemoryContext -> RagContext -> UserQuery`) into `out_buf`.
    /// Returns `null` if `out_buf` is too small.
    fn build(out_buf: *[]u8): AgentPromptBuildReport? {
        let report = AgentPromptBuildReport{
            max_tokens: _.max_tokens,
        };
        if (_.count == 0) return report;

        // 1. Sort block indices by (priority desc, insertion index asc) in stack buffer.
        let order: [64]usize = [0; 64];
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            order[i] = i;
        }
        let a: usize = 1;
        while (a < _.count) : (a += 1) {
            const key = order[a];
            let b: usize = a;
            while (b > 0) {
                const prev = order[b - 1];
                const better = (_.blocks[key].priority > _.blocks[prev].priority) ||
                    (_.blocks[key].priority == _.blocks[prev].priority && key < prev);
                if (!better) break;
                order[b] = prev;
                b -= 1;
            }
            order[b] = key;
        }

        // 2. Allocate token budget in priority order.
        let keep_bytes: [64]usize = [0; 64];
        let is_included: [64]bool = [false; 64];
        let is_truncated: [64]bool = [false; 64];
        let used_tokens: u32 = 0;

        let p: usize = 0;
        while (p < _.count) : (p += 1) {
            const idx = order[p];
            const blk = _.blocks[idx];
            const rem: u32 = if (_.max_tokens > used_tokens) _.max_tokens - used_tokens else 0;
            if (rem == 0) {
                report.dropped_sections += 1;
                continue;
            }

            if (blk.token_count <= rem) {
                is_included[idx] = true;
                keep_bytes[idx] = blk.content.len;
                used_tokens += blk.token_count;
                report.included_sections += 1;
            } else {
                // Proportionally truncate content to fit remaining tokens.
                const scaled: usize = ((blk.content.len as u64) * (rem as u64) / (blk.token_count as u64)) as usize;
                const cut: usize = if (scaled == 0) 1 else if (scaled > blk.content.len) blk.content.len else scaled;
                is_included[idx] = true;
                is_truncated[idx] = true;
                keep_bytes[idx] = cut;
                used_tokens += rem;
                report.included_sections += 1;
                report.truncated_sections += 1;
            }
        }
        report.used_tokens = used_tokens;

        // 3. Render included sections in canonical LLM order (rank 0..4).
        let pos: usize = 0;
        let rank: u8 = 0;
        while (rank < 5) : (rank += 1) {
            let j: usize = 0;
            while (j < _.count) : (j += 1) {
                if (!is_included[j]) continue;
                const blk = _.blocks[j];
                if (sectionRank(blk.section) != rank) continue;

                if (!appendBytes(out_buf, &pos, sectionHeader(blk.section))) return null;
                const slice_len = keep_bytes[j];
                if (!appendBytes(out_buf, &pos, blk.content[0..slice_len])) return null;
                if (is_truncated[j]) {
                    if (!appendBytes(out_buf, &pos, "...[truncated]")) return null;
                }
                if (!appendBytes(out_buf, &pos, "\n\n")) return null;

                match (blk.section) {
                    AgentPromptSection.SystemRole => report.system_blocks += 1,
                    AgentPromptSection.MemoryContext => report.memory_blocks += 1,
                    AgentPromptSection.RagContext => report.rag_blocks += 1,
                    AgentPromptSection.ToolCatalog => report.tool_blocks += 1,
                    AgentPromptSection.UserQuery => report.user_blocks += 1,
                }
            }
        }

        report.bytes_written = pos;
        return report;
    }

    /// Returns number of staged sections.
    fn sectionCount(): usize {
        return _.count;
    }

    /// Clears staged sections while preserving `blocks` and `max_tokens`.
    fn reset(*_) {
        _.count = 0;
    }
}
