// Source: cp/lib/prompt/index.cp (Unified Exports for cap:prompt)
// Unified Public Exports for LLM Context Window Management & Structured Output Extraction (cap:prompt & cap.prompt).

export {
    PromptError,
    PromptRole,
    TrimStrategy,
    PromptMessage,
    WindowStats,
    ContextWindow,
} from "./messages.cp";

export {
    CodeBlock,
    XmlTagMatch,
    PromptExtractor,
} from "./extractor.cp";

export {
    ThoughtSplit,
    ExtractedBlock,
    RedactStats,
    splitThoughtBlock,
    extractCodeFence,
    extractFirstJsonSlice,
    jsonHasTopLevelKeys,
    redactSensitiveText,
} from "./structured.cp";

export {
    TemplateVar,
    TemplateRenderStatus,
    TemplateRenderResult,
    PackRole,
    PackableMessage,
    PackSummary,
    renderPromptTemplate,
    extractTemplateKeys,
    packContextWindow,
} from "./template.cp";
