// Type definitions for CP

export fn c25Carry(o: []i64);
export fn c25Sel(p: []i64, q: []i64, b: i64);
export class FieldElement {
    limbs: [16]i64,
    fn fromBytes(b: [32]u8): FieldElement;
    fn toBytes(): [32]u8;
    fn add(other: FieldElement): FieldElement;
    fn sub(other: FieldElement): FieldElement;
    fn mul(other: FieldElement): FieldElement;
    fn square(): FieldElement;
    fn invert(): FieldElement;
}

export fn clampScalar(k: [32]u8): [32]u8;
export fn x25519(scalar: [32]u8, u_coord: [32]u8): [32]u8;
export fn x25519Basepoint(scalar: [32]u8): [32]u8;
