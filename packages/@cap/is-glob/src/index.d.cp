// Type definitions for CP

import { isExtglob } from "../../is-extglob/src/index.cp";
export fn hasClosedBracket(str: string, open_idx: usize): bool;
export fn hasClosedBrace(str: string, open_idx: usize): bool;
export fn hasUnescapedWildcard(str: string): bool;
export fn isGlob(str: string, strict: bool): bool;
export fn isGlobDefault(str: string): bool;
