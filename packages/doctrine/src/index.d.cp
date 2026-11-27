// Type definitions for CP

export class JsDocTag {
    title: string,
    type_expr: string,
    name: string,
    description: string,
    optional: bool,
}

export fn docEmptyTag(): JsDocTag;
export fn docIsWs(ch: u8): bool;
export fn docIsSpace(ch: u8): bool;
export fn docStrEq(a: string, b: string): bool;
export fn docTrim(s: string): string;
export fn docStripLinePrefix(line: string): string;
export fn docTagHasName(title: string): bool;
export fn docParseTagLine(line: string): JsDocTag;
export class JsDocComment {
    description: string,
    tags: [16]JsDocTag,
    tag_count: usize,
    fn tagCount(): usize;
    fn getTag(idx: usize): JsDocTag?;
    fn findTag(title: string): JsDocTag?;
    fn isDeprecated(): bool;
}

export fn parseJsDoc(comment: string): JsDocComment;
