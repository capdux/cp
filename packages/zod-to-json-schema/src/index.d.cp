// Type definitions for CP

import { BufWriter } from "cap:fmt";
export enum SchemaType {
    String,
    Number,
    Integer,
    Boolean,
    Array,
    Object,
    Null
}

export class SchemaProp {
    name: string,
    kind: SchemaType,
    description: string,
    required: bool,
    format: string,
    item_kind: SchemaType,
    has_min: bool,
    min_val: i64,
    has_max: bool,
    max_val: i64,
}

export fn schemaTypeName(t: SchemaType): string;
export fn prop(name: string, kind: SchemaType, description: string, required: bool): SchemaProp;
export fn stringProp(name: string, description: string, format: string, required: bool): SchemaProp;
export fn intProp(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp;
export fn arrayProp(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp;
export fn toJsonSchema(title: string, props: &[]SchemaProp, additional_properties: bool, out: []u8): usize;
