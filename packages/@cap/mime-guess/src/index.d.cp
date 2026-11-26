// Type definitions for CP

import { lookup, extension, charset, isCompressible } from "../../mime-types/src/index.cp";
export fn mgsStartsWith(s: string, prefix: string): bool;
export fn mgsHasPathSep(s: string): bool;
export fn mgsIsCompressibleMime(mime_type: string): bool;
export fn extractExt(path: string): string?;
export fn getExtForMime(mime_type: string): string?;
export fn getCharsetForMime(mime_type: string): string?;
export class MimeGuess {
    mime: string,
    found: bool,
    fn fromExt(ext: string): MimeGuess;
    fn fromPath(path: string): MimeGuess;
    fn first(): string?;
    fn firstOrOctetStream(): string;
    fn firstOrTextPlain(): string;
    fn isText(): bool;
    fn isImage(): bool;
    fn isAudio(): bool;
    fn isVideo(): bool;
    fn isCompressible(): bool;
}

