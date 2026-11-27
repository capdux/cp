// Type definitions for CP

import { trim, eqIgnoreCase } from "cap:ascii";
export fn yn(input: string): bool?;
export fn ynLenient(input: string): bool?;
export fn ynDefault(input: string, default_val: bool): bool;
