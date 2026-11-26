// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export class EnumVariant {
    id: i32,
    name: string,
    alias: string,
    message: string,
}

export fn stmStrEq(a: string, b: string): bool;
export class EnumRegistry {
    variants: [32]EnumVariant,
    count: usize,
    fn new(): EnumRegistry;
    fn variantCount(): usize;
    fn register(_: auto, id: i32, name: string, alias: string, message: string): bool;
    fn fromStr(s: string): i32?;
    fn fromStrIgnoreCase(s: string): i32?;
    fn asStr(id: i32): string?;
    fn getMessage(id: i32): string?;
    fn variantAt(idx: usize): EnumVariant?;
    fn formatVariantList(out: []u8): usize;
}

