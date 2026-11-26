pub const ND_MAX_ELEMS: usize = 64;

pub struct Array2D {
    pub rows: usize,
    pub cols: usize,
    pub data: [64]f64,

    pub static fn zeros(r: usize, c: usize): Array2D {
        let buf: [64]f64 = [0.0; 64];
        let rr = if (r * c > ND_MAX_ELEMS) { 0 } else { r };
        let cc = if (r * c > ND_MAX_ELEMS) { 0 } else { c };
        return Array2D { rows: rr, cols: cc, data: buf };
    }

    pub static fn filled(r: usize, c: usize, val: f64): Array2D {
        let arr = Array2D.zeros(r, c);
        let n = arr.rows * arr.cols;
        let i: usize = 0;
        while (i < n) {
            arr.data[i] = val;
            i = i + 1;
        }
        return arr;
    }

    pub static fn eye(n: usize): Array2D {
        let arr = Array2D.zeros(n, n);
        let i: usize = 0;
        while (i < arr.rows) {
            arr.data[i * arr.cols + i] = 1.0;
            i = i + 1;
        }
        return arr;
    }

    pub static fn fromSlice(r: usize, c: usize, vals: []f64): Array2D? {
        if (r * c > ND_MAX_ELEMS || vals.len < r * c) {
            return null;
        }
        let arr = Array2D.zeros(r, c);
        let i: usize = 0;
        let n = r * c;
        while (i < n) {
            arr.data[i] = vals[i];
            i = i + 1;
        }
        return arr;
    }

    pub fn elemCount(): usize {
        return _.rows * _.cols;
    }

    pub fn get(r: usize, c: usize): f64? {
        if (r >= _.rows || c >= _.cols) { return null; }
        return _.data[r * _.cols + c];
    }

    pub fn set(*_, r: usize, c: usize, val: f64): bool {
        if (r >= _.rows || c >= _.cols) { return false; }
        _.data[r * _.cols + c] = val;
        return true;
    }

    pub fn sum(): f64 {
        let acc: f64 = 0.0;
        let n = _.rows * _.cols;
        let i: usize = 0;
        while (i < n) {
            acc = acc + _.data[i];
            i = i + 1;
        }
        return acc;
    }

    pub fn mean(): f64? {
        let n = _.rows * _.cols;
        if (n == 0) { return null; }
        return _.sum() / (n as f64);
    }

    pub fn minVal(): f64? {
        let n = _.rows * _.cols;
        if (n == 0) { return null; }
        let m = _.data[0];
        let i: usize = 1;
        while (i < n) {
            if (_.data[i] < m) { m = _.data[i]; }
            i = i + 1;
        }
        return m;
    }

    pub fn maxVal(): f64? {
        let n = _.rows * _.cols;
        if (n == 0) { return null; }
        let m = _.data[0];
        let i: usize = 1;
        while (i < n) {
            if (_.data[i] > m) { m = _.data[i]; }
            i = i + 1;
        }
        return m;
    }

    pub fn transpose(): Array2D {
        let out = Array2D.zeros(_.cols, _.rows);
        let r: usize = 0;
        while (r < _.rows) {
            let c: usize = 0;
            while (c < _.cols) {
                out.data[c * _.rows + r] = _.data[r * _.cols + c];
                c = c + 1;
            }
            r = r + 1;
        }
        return out;
    }

    pub fn scale(factor: f64): Array2D {
        let out = Array2D.zeros(_.rows, _.cols);
        let n = _.rows * _.cols;
        let i: usize = 0;
        while (i < n) {
            out.data[i] = _.data[i] * factor;
            i = i + 1;
        }
        return out;
    }

    pub fn reshape(new_r: usize, new_c: usize): Array2D? {
        if (new_r * new_c != _.rows * _.cols || new_r * new_c > ND_MAX_ELEMS) {
            return null;
        }
        let out = Array2D { rows: new_r, cols: new_c, data: _.data };
        return out;
    }
}

pub fn addArrays(a: Array2D, b: Array2D): Array2D? {
    if (a.rows != b.rows || a.cols != b.cols) { return null; }
    let out = Array2D.zeros(a.rows, a.cols);
    let n = a.rows * a.cols;
    let i: usize = 0;
    while (i < n) {
        out.data[i] = a.data[i] + b.data[i];
        i = i + 1;
    }
    return out;
}

pub fn matmul(a: Array2D, b: Array2D): Array2D? {
    if (a.cols != b.rows || a.rows * b.cols > ND_MAX_ELEMS) {
        return null;
    }
    let out = Array2D.zeros(a.rows, b.cols);
    let r: usize = 0;
    while (r < a.rows) {
        let c: usize = 0;
        while (c < b.cols) {
            let acc: f64 = 0.0;
            let k: usize = 0;
            while (k < a.cols) {
                acc = acc + a.data[r * a.cols + k] * b.data[k * b.cols + c];
                k = k + 1;
            }
            out.data[r * b.cols + c] = acc;
            c = c + 1;
        }
        r = r + 1;
    }
    return out;
}

pub fn dotProduct(a: []f64, b: []f64): f64? {
    if (a.len != b.len) { return null; }
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) {
        acc = acc + a[i] * b[i];
        i = i + 1;
    }
    return acc;
}
