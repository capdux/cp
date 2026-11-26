// Type definitions for CP

import { CAT_NONE, CAT_IMAGE, CAT_AUDIO, CAT_VIDEO, CAT_ARCHIVE, CAT_FONT, CAT_EXECUTABLE, CAT_DOCUMENT, CAT_DATA_ML, isBinaryPath, isBinaryExtension, extensionCategory } from "../../binary-extensions/src/index.cp";
export fn ibpExtractExt(path: string): string;
export fn isTextPath(path: string): bool;
export fn filterBinaryPaths(paths: &[]string, out_indices: []usize): usize;
export fn filterTextPaths(paths: &[]string, out_indices: []usize): usize;
export fn countBinaryPaths(paths: &[]string): usize;
export fn detectPathKind(path: string): u8;
