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

export {
    ChatTemplateKind,
    ChatMsgRole,
    ChatMessage,
    sanitizeControlTokens,
    formatChatPrompt,
} from "./chat_format.cp";

export {
    FewShotExample,
    FewShotSelection,
    FewShotConfig,
    tokenJaccardSimilarity,
    selectFewShotExamples,
    renderFewShotBlock,
} from "./fewshot.cp";

export {
    PromptRiskFlags,
    PromptSafetyReport,
    PiiKind,
    PiiSpan,
    scanPiiSpans,
    redactPii,
    inspectPromptSafety,
} from "./safety.cp";

export {
    JsonSchemaType,
    SchemaProp,
    ToolSchemaSpec,
    emitJsonSchemaObject,
    emitOpenAiToolJson,
    emitAnthropicToolJson,
} from "./json_schema.cp";

export {
    StreamChannel,
    DemuxEvent,
    DemuxStats,
    ReasoningStreamDemuxer,
} from "./reasoning_stream.cp";

export {
    JsonCandidateSpan,
    JsonRepairReport,
    JsonValueKind,
    RequiredFieldSpec,
    FieldCheckIssueKind,
    FieldCheckIssue,
    RequiredFieldsReport,
    extractJsonCandidate,
    repairLlmJson,
    checkRequiredJsonFields,
    formatRepairRetryPrompt,
} from "./structured_output.cp";

export {
    MAX_COMPOSER_BLOCKS,
    AgentPromptSection,
    AgentPromptBlock,
    AgentPromptBuildReport,
    AgentPromptComposer,
} from "./agent_composer.cp";

export {
    FIREWALL_FLAG_DELIMITER_OVERRIDE,
    FIREWALL_FLAG_SECRET_LEAK,
    FIREWALL_FLAG_PATH_TRAVERSAL,
    FIREWALL_FLAG_UNSAFE_URL,
    FirewallAction,
    FirewallPolicy,
    FirewallFinding,
    FirewallVerdict,
    evaluatePromptFirewall,
    redactFirewallSecrets,
} from "./firewall.cp";

export {
    MediaPartKind,
    ImageDetailMode,
    MultimodalRejectReason,
    MultimodalPart,
    MultimodalLimits,
    MultimodalAuditResult,
    estimateImageTokens,
    estimateAudioTokens,
    isAllowedMultimodalMime,
    auditMultimodalPayload,
} from "./multimodal.cp";

export {
    MAX_PROMPT_VERSIONS,
    MAX_VERSION_ID_LEN,
    MAX_MODEL_HINT_LEN,
    MAX_PROMPT_SUMMARY_LEN,
    BPS_SCALE,
    PromptVariantStatus,
    PromptVersionEntry,
    RouteDecision,
    RolloutGuardConfig,
    RegistrySummaryStats,
    PromptVersionRegistry,
} from "./version_registry.cp";

export {
    JSON_STREAM_MAX_DEPTH,
    JSON_STREAM_MAX_KEY_LEN,
    JSON_STREAM_MAX_VAL_LEN,
    JSON_STREAM_MAX_EVENTS,
    JsonStreamEventKind,
    JsonStreamValueKind,
    JsonStreamEvent,
    JsonStreamStats,
    JsonStreamScanner,
} from "./json_stream_scanner.cp";

export {
    CITATION_MAX_REFS,
    CITATION_MAX_SENTENCES,
    CitationRefSpan,
    CitationSentenceAudit,
    CitationGateConfig,
    CitationAuditReport,
    extractInlineCitations,
    computeClaimSupportBps,
    auditRagCitations,
} from "./citation_grounder.cp";
