// Source: bun/src/dotenv/env_loader.cp
// CP Standard Library: cap:dotenv

export {
    MAX_ENV_ENTRIES,
    ENV_BUFFER_CAPACITY,
    EnvEntry,
    EnvMap,
    EnvParser,
    parse,
    parseWithOptions,
    expandString,
} from "./parser.cp";

export {
    DotenvOptions,
    DotenvLoader,
    config,
    load,
} from "./loader.cp";
