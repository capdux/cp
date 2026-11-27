// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";

export enum BinaryCategory {
    None,
    Image,
    AudioVideo,
    Archive,
    Executable,
    Font,
    DocumentData,
}

export fn extractExtension(file_path: string): string;
export fn classifyBinaryExtension(ext: string): BinaryCategory;
export fn classifyBinaryPath(file_path: string): BinaryCategory;
export fn isBinaryExtension(ext: string): bool;
export fn isBinaryPath(file_path: string): bool;
export fn isTextPath(file_path: string): bool;
export fn isImagePath(file_path: string): bool;
export fn isMediaPath(file_path: string): bool;
export fn isArchivePath(file_path: string): bool;
export fn isExecutablePath(file_path: string): bool;
export fn isFontPath(file_path: string): bool;
