// Type definitions for CP

import { HmacSha256, HMAC_SHA256_LEN } from "../../hmac/src/index.cp";

export const HKDF_HASH_LEN: usize;
export const HKDF_MAX_OKM_LEN: usize;
export const HKDF_SHA256_HASH_LEN: usize;
export const HKDF_SHA256_MAX_OKM_LEN: usize;
export const HKDF_SHA384_HASH_LEN: usize;
export const HKDF_SHA384_MAX_OKM_LEN: usize;
export const HKDF_SHA512_HASH_LEN: usize;
export const HKDF_SHA512_MAX_OKM_LEN: usize;

export enum HkdfHashAlg {
    Sha256,
    Sha384,
    Sha512,
    fn hashLen(): usize;
    fn maxOkmLen(): usize;
}

export class HkdfSha256 {
    prk: [32]u8,
    fn extract(salt: []u8, ikm: []u8): HkdfSha256;
    fn fromPrk(prk_in: []u8): HkdfSha256?;
    fn expand(info: []u8, okm: []u8): bool;
    fn expandMultiInfo(infos: &[]string, okm: []u8): bool;
    fn expandLabel(label: &[]u8, context: &[]u8, okm: []u8): bool;
    fn verifyPrk(expected: &[]u8): bool;
}

export class HkdfSha384 {
    prk: [48]u8,
    fn extract(salt: &[]u8, ikm: &[]u8): HkdfSha384;
    fn fromPrk(prk_in: &[]u8): HkdfSha384?;
    fn expand(info: &[]u8, okm: []u8): bool;
    fn verifyPrk(expected: &[]u8): bool;
}

export class HkdfSha512 {
    prk: [64]u8,
    fn extract(salt: &[]u8, ikm: &[]u8): HkdfSha512;
    fn fromPrk(prk_in: &[]u8): HkdfSha512?;
    fn expand(info: &[]u8, okm: []u8): bool;
    fn verifyPrk(expected: &[]u8): bool;
}

export fn hkdfSha256(salt: []u8, ikm: []u8, info: []u8, okm: []u8): bool;
export fn hkdfSha384(salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool;
export fn hkdfSha512(salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool;
export fn hkdfExtract(alg: HkdfHashAlg, salt: &[]u8, ikm: &[]u8, prk_out: []u8): usize;
export fn hkdfExpand(alg: HkdfHashAlg, prk: &[]u8, info: &[]u8, okm: []u8): bool;
export fn hkdfDerive(alg: HkdfHashAlg, salt: &[]u8, ikm: &[]u8, info: &[]u8, okm: []u8): bool;
