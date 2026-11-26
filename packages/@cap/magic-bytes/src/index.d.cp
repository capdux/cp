// Type definitions for CP

export class FileType {
    ext: string,
    mime: string,
    kind: u8,
    matched: bool,
}

export fn mbMatch(ext: string, mime: string, kind: u8): FileType;
export fn mbEqAt(data: &[]u8, offset: usize, lit: &[]u8): bool;
export fn detect(data: &[]u8): FileType;
export fn isImage(data: &[]u8): bool;
export fn isArchive(data: &[]u8): bool;
export fn isExecutable(data: &[]u8): bool;
