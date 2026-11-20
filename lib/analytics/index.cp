// Source: cp/lib/analytics/index.cp (Unified Exports for cap:analytics & cap.analytics)
// Industrial-grade pure CP runtime telemetry collector, bounded ring buffer, PII redaction & JSON/NDJSON batch serializer.

export {
    REDACTED_VALUE,
    MAX_EVENT_NAME_LEN,
    MAX_PROP_KEY_LEN,
    MAX_PROPS_PER_EVENT,
    AnalyticsError,
    DropPolicy,
    PropKind,
    PropValue,
    EventProp,
    AnalyticsEvent,
    isSensitiveKey,
    validateEventName,
    validatePropKey,
    sanitizeAndCloneProp,
    freeProp,
    cloneEventWithSanitization,
    freeEvent,
    freeEventBatch,
} from "./types.cp";

export {
    CollectorConfig,
    AnalyticsCollector,
} from "./collector.cp";

export {
    encodeBatchJson,
    encodeBatchNdjson,
} from "./json.cp";
