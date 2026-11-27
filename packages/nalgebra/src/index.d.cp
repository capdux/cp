// Type definitions for CP

import { Vector2, Vector3, Vector4, RowMatrix2, RowMatrix3, RowMatrix4 } from "../../mint/src/index.cp";
export fn naSqrt(x: f64): f64;
export fn vec2Add(a: Vector2, b: Vector2): Vector2;
export fn vec2Sub(a: Vector2, b: Vector2): Vector2;
export fn vec2Scale(v: Vector2, s: f64): Vector2;
export fn vec2Norm(v: Vector2): f64;
export fn vec3Add(a: Vector3, b: Vector3): Vector3;
export fn vec3Sub(a: Vector3, b: Vector3): Vector3;
export fn vec3Scale(v: Vector3, s: f64): Vector3;
export fn vec3Norm(v: Vector3): f64;
export fn vec3Normalize(v: Vector3): Vector3?;
export fn vec3Lerp(a: Vector3, b: Vector3, t: f64): Vector3;
export fn vec3Distance(a: Vector3, b: Vector3): f64;
export fn mat2Mul(a: RowMatrix2, b: RowMatrix2): RowMatrix2;
export fn mat2Inverse(m: RowMatrix2): RowMatrix2?;
export fn mat3Det(m: RowMatrix3): f64;
export fn mat3Mul(a: RowMatrix3, b: RowMatrix3): RowMatrix3;
export fn mat3Inverse(m: RowMatrix3): RowMatrix3?;
export fn mat4Mul(a: RowMatrix4, b: RowMatrix4): RowMatrix4;
export fn mat4Translation(tx: f64, ty: f64, tz: f64): RowMatrix4;
export fn mat4Scaling(sx: f64, sy: f64, sz: f64): RowMatrix4;
export fn mat4TransformPoint(m: RowMatrix4, p: Vector3): Vector3;
