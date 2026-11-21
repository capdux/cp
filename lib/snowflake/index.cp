// Source: cp/lib/snowflake/index.cp (Pure CP Distributed 64-bit Snowflake ID)
export {
    DEFAULT_EPOCH_MS,
    MAX_DATACENTER_ID,
    MAX_WORKER_ID,
    MAX_SEQUENCE,
    SnowflakeError,
    SnowflakeParts,
    SnowflakeGenerator,
    decomposeId,
    encodeBase62,
    decodeBase62,
} from "./snowflake.cp";
