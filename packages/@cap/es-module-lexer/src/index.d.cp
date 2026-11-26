// Type definitions for CP

export enum ImportKind {
    Static,
    Dynamic,
    ImportMeta
}

export class ImportSpecifier {
    kind: ImportKind,
    spec_start: usize,
    spec_end: usize,
    stmt_start: usize,
    stmt_end: usize,
    fn specifier(src: string): string;
}

export class ExportSpecifier {
    name_start: usize,
    name_end: usize,
    local_start: usize,
    local_end: usize,
    fn name(src: string): string;
    fn localName(src: string): string;
}

export class EsmParseResult {
    import_count: usize,
    export_count: usize,
    facade: bool,
    has_module_syntax: bool,
}

export class EsmlSpan {
    start: usize,
    end: usize,
    ok: bool,
}

export fn esmlIsWs(c: u8): bool;
export fn esmlIsIdentStart(c: u8): bool;
export fn esmlIsIdentPart(c: u8): bool;
export fn esmlStrEq(a: string, b: string): bool;
export fn esmlSkipWsAndComments(src: string, start: usize): usize;
export fn esmlScanString(src: string, start: usize): EsmlSpan;
export fn esmlSkipTemplate(src: string, start: usize): usize;
export fn esmlScanIdent(src: string, start: usize): EsmlSpan;
export fn esmlPrevNonWsChar(src: string, pos: usize): u8;
export fn esmlPushImport(out: []ImportSpecifier, count: usize, kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize): usize;
export fn esmlPushExport(out: []ExportSpecifier, count: usize, name_start: usize, name_end: usize, local_start: usize, local_end: usize): usize;
export fn parseEsm(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult;
export fn isFacade(src: string): bool;
