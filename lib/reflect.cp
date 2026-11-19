// Source: Custom (CP native stdlib)
/**
 * CP Reflection Standard Library (cap:reflect)
 * Zero-cost introspection & metadata reflection
 */

pub class FieldInfo {
    name: string;
    typeName: string;
    meta: *!;
}

pub class MethodInfo {
    name: string;
    returnTypeName: string;
    paramNames: Array<string>;
    isAsync: bool;
    isStatic: bool;
    meta: *!;
}

pub class TypeInfo {
    name: string;
    isRef: bool;
    fields: Array<FieldInfo>;
    methods: Array<MethodInfo>;
    meta: *!;
}
