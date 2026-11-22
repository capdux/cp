// Origin: Pure-CP Stdlib (Zero-Alloc 64-bit Charikar SimHash, Hamming Similarity & 4x16-bit LSH Bands) | Created: 2026-11-22

export {
    SIMHASH_BITS,
    SIMHASH_DEFAULT_THRESHOLD,
    fnv1a64,
    SimHasher,
    simhashText,
    simhashShingles,
    hammingDistance,
    simhashSimilarity,
    isNearDuplicate,
    simhashBands4,
    sharesAnyBand4,
} from "./simhash.cp";
