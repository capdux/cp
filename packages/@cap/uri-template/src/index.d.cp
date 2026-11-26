// Type definitions for CP

export class TemplateVar {
    name: string,
    value: string,
}

export fn utIsAlpha(c: u8): bool;
export fn utIsDigit(c: u8): bool;
export fn utIsHex(c: u8): bool;
export fn utHexUpper(n: u8): u8;
export fn utIsUnreserved(c: u8): bool;
export fn utIsReserved(c: u8): bool;
export fn utIsOperator(c: u8): bool;
export fn utIsReservedOp(c: u8): bool;
export fn utEqBytes(a: &[]u8, b: &[]u8): bool;
export fn utIsValidVarSpec(spec: &[]u8): bool;
export fn isValid(tmpl: string): bool;
export fn countVars(tmpl: string): usize;
export fn extractVarName(tmpl: string, index: usize, out: []u8): usize;
export fn pctEncode(input: string, allow_reserved: bool, out: []u8): usize;
export fn utTruncateUtf8(val: &[]u8, maxChars: usize): &[]u8;
export fn utWriteSlice(out: []u8, pos: &mut usize, src: &[]u8);
export fn utWriteByte(out: []u8, pos: &mut usize, b: u8);
export fn expand(tmpl: string, vars: &[]TemplateVar, out: []u8): usize;
