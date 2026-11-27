// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
export fn extractExtension(file_path: string): string;
export fn isBinaryExtension(ext: string): bool;
export fn isBinaryPath(file_path: string): bool;
