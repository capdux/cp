// Type definitions for CP

export enum EcdsaAlg {
    ES256,
    ES384,
    ES512
}

export fn paramBytes(alg: EcdsaAlg): usize;
export fn esfParseDerInt(der: &[]u8, pos: usize, limit: usize, param_len: usize, dst: []u8): i64;
export fn esfTrimLeadingZeros(src: &[]u8): usize;
export fn derToJose(der: &[]u8, alg: EcdsaAlg, out: []u8): i64;
export fn joseToDer(jose: &[]u8, alg: EcdsaAlg, out: []u8): i64;
