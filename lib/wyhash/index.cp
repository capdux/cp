// Source: cp/lib/wyhash/index.cp (Wyhash v4 64-bit non-cryptographic hash & WyRand PRNG engine)

export {
    Wyhash,
    WyhashStream,
    WyhashStateless,
    wyhash,
    wyhash64,
    wyhashFinal4,
    wyhashMum,
} from "./core";

export { WyRand } from "./wyrand";

export {
    hashString,
    hashInt,
    combineHashes,
} from "./helpers";
