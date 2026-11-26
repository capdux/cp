// Type definitions for CP

export fn mmulMin(a: usize, b: usize): usize;
export fn sgemm(m: usize, k: usize, n: usize, alpha: f32, a: &[]f32, rsa: usize, csa: usize, b: &[]f32, rsb: usize, csb: usize, beta: f32, c: []f32, rsc: usize, csc: usize);
export fn dgemm(m: usize, k: usize, n: usize, alpha: f64, a: &[]f64, rsa: usize, csa: usize, b: &[]f64, rsb: usize, csb: usize, beta: f64, c: []f64, rsc: usize, csc: usize);
export fn matmulF64(m: usize, k: usize, n: usize, a: &[]f64, b: &[]f64, c: []f64);
