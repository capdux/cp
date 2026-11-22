// Source: cp/lib/fft/fft.cp (Cooley-Tukey Radix-2 FFT/IFFT, Windows, PSD & Convolution)
// Pure CP zero-C-dependency Fast Fourier Transform, windowing, spectral magnitude, peak detection, and fast convolution engine.

const std = @import("std");

/// Error set returned by FFT, windowing, spectral analysis, and convolution operations.
pub err FftError {
    NotPowerOfTwo,
    EmptyInput,
    LengthMismatch,
    InvalidSampleRate,
    OutOfMemory,
}

/// Double-precision complex number `(re + i * im)`.
pub struct Complex {
    re: f64 = 0.0;
    im: f64 = 0.0;

    /// Constructs a `Complex` value from real and imaginary components.
    static fn init(re: f64, im: f64): Complex {
        return Complex{ re: re, im: im };
    }

    /// Returns `self + other`.
    fn add(other: Complex): Complex {
        return Complex{
            re: _.re + other.re,
            im: _.im + other.im,
        };
    }

    /// Returns `self - other`.
    fn sub(other: Complex): Complex {
        return Complex{
            re: _.re - other.re,
            im: _.im - other.im,
        };
    }

    /// Returns `self * other`.
    fn mul(other: Complex): Complex {
        return Complex{
            re: _.re * other.re - _.im * other.im,
            im: _.re * other.im + _.im * other.re,
        };
    }

    /// Returns the Euclidean magnitude `sqrt(re^2 + im^2)`.
    fn abs(): f64 {
        return @sqrt(_.re * _.re + _.im * _.im);
    }

    /// Returns the squared magnitude `re^2 + im^2`.
    fn absSq(): f64 {
        return _.re * _.re + _.im * _.im;
    }

    /// Returns the complex conjugate `(re - i * im)`.
    fn conj(): Complex {
        return Complex{
            re: _.re,
            im: 0.0 - _.im,
        };
    }
}

/// Classical tapering window functions for spectral leakage suppression.
pub enum WindowKind {
    Rectangular,
    Hann,
    Hamming,
    Blackman,
}

/// Detected dominant frequency bin, frequency in Hz, and spectral magnitude.
pub struct FrequencyPeak {
    bin: usize = 0;
    freq_hz: f64 = 0.0;
    magnitude: f64 = 0.0;
}

/// Pure CP Cooley-Tukey Radix-2 FFT, IFFT, windowing, spectral analysis, and fast convolution engine.
pub static struct Fft {
    /// Returns `true` if `n > 0` and `n` is an exact power of two.
    static fn isPowerOfTwo(n: usize): bool {
        return n > 0 && (n & (n - 1)) == 0;
    }

    /// Returns the smallest power of two greater than or equal to `n` (minimum `1`).
    static fn nextPowerOfTwo(n: usize): usize {
        if (n <= 1) return 1;
        let p: usize = 1;
        while (p < n) {
            p <<= 1;
        }
        return p;
    }

    /// Performs an in-place iterative Cooley-Tukey Radix-2 forward DFT on `buf`.
    /// Requires `buf.len > 0` and `buf.len` to be a power of two.
    static fn forwardInPlace(buf: []Complex): void ! FftError {
        const n = buf.len;
        if (n == 0) return FftError.EmptyInput;
        if (!Fft.isPowerOfTwo(n)) return FftError.NotPowerOfTwo;

        if (n > 1) {
            // Iterative bit-reversal permutation
            let j: usize = 0;
            for (i_raw of 1..n) {
                const i = usize(i_raw);
                let bit: usize = n >> 1;
                while ((j & bit) != 0) {
                    j ^= bit;
                    bit >>= 1;
                }
                j ^= bit;
                if (i < j) {
                    const tmp = buf[i];
                    buf[i] = buf[j];
                    buf[j] = tmp;
                }
            }

            // Iterative Radix-2 Cooley-Tukey butterfly stages
            let len: usize = 2;
            while (len <= n) {
                const half = len >> 1;
                const base_angle = -2.0 * std.math.pi / (len as f64);
                let i: usize = 0;
                while (i < n) {
                    for (k_raw of 0..half) {
                        const k = usize(k_raw);
                        const theta = base_angle * (k as f64);
                        const w = Complex{ re: @cos(theta), im: @sin(theta) };
                        const u = buf[i + k];
                        const t = w.mul(buf[i + k + half]);
                        buf[i + k] = u.add(t);
                        buf[i + k + half] = u.sub(t);
                    }
                    i += len;
                }
                len <<= 1;
            }
        }
    }

    /// Allocates and returns the forward DFT of `input`.
    static fn forward(alloc: std.mem.Allocator, input: []const Complex): []Complex ! FftError {
        if (input.len == 0) return FftError.EmptyInput;
        if (!Fft.isPowerOfTwo(input.len)) return FftError.NotPowerOfTwo;

        let out = alloc.alloc(Complex, input.len) catch return FftError.OutOfMemory;
        errdefer alloc.free(out);
        for (i of 0..input.len) {
            out[i] = input[i];
        }
        Fft.forwardInPlace(out)?;
        return out;
    }

    /// Performs an in-place inverse DFT via conjugation, forward FFT, and `1 / N` scaling.
    static fn inverseInPlace(buf: []Complex): void ! FftError {
        const n = buf.len;
        if (n == 0) return FftError.EmptyInput;
        if (!Fft.isPowerOfTwo(n)) return FftError.NotPowerOfTwo;

        for (i of 0..n) {
            buf[i] = buf[i].conj();
        }
        Fft.forwardInPlace(buf)?;

        const inv_n = 1.0 / (n as f64);
        for (i of 0..n) {
            buf[i] = Complex{
                re: buf[i].re * inv_n,
                im: 0.0 - (buf[i].im * inv_n),
            };
        }
    }

    /// Allocates and returns the inverse DFT of `spectrum`.
    static fn inverse(alloc: std.mem.Allocator, spectrum: []const Complex): []Complex ! FftError {
        if (spectrum.len == 0) return FftError.EmptyInput;
        if (!Fft.isPowerOfTwo(spectrum.len)) return FftError.NotPowerOfTwo;

        let out = alloc.alloc(Complex, spectrum.len) catch return FftError.OutOfMemory;
        errdefer alloc.free(out);
        for (i of 0..spectrum.len) {
            out[i] = spectrum[i];
        }
        Fft.inverseInPlace(out)?;
        return out;
    }

    /// Applies the specified tapering window function to `signal` and returns a newly allocated slice.
    static fn applyWindow(alloc: std.mem.Allocator, signal: []const f64, kind: WindowKind): []f64 ! FftError {
        if (signal.len == 0) return FftError.EmptyInput;

        let out = alloc.alloc(f64, signal.len) catch return FftError.OutOfMemory;
        if (signal.len == 1) {
            out[0] = signal[0];
            return out;
        }

        const denom = (signal.len - 1) as f64;
        const two_pi = 2.0 * std.math.pi;
        const four_pi = 4.0 * std.math.pi;

        for (i of 0..signal.len) {
            const ratio = (i as f64) / denom;
            let w: f64 = 1.0;
            switch (kind) {
                .Rectangular => {
                    w = 1.0;
                },
                .Hann => {
                    w = 0.5 * (1.0 - @cos(two_pi * ratio));
                },
                .Hamming => {
                    w = 0.54 - 0.46 * @cos(two_pi * ratio);
                },
                .Blackman => {
                    w = 0.42 - 0.5 * @cos(two_pi * ratio) + 0.08 * @cos(four_pi * ratio);
                },
            }
            out[i] = signal[i] * w;
        }
        return out;
    }

    /// Computes the single-sided real FFT magnitude spectrum over bins `0..N/2` (length `N / 2 + 1`).
    /// Interior bins `1..N/2-1` are scaled by `sqrt(2)` so `sum(mag^2) / N == sum(signal^2)` (Parseval).
    static fn rfftMagnitude(alloc: std.mem.Allocator, signal: []const f64): []f64 ! FftError {
        const n = signal.len;
        if (n == 0) return FftError.EmptyInput;
        if (!Fft.isPowerOfTwo(n)) return FftError.NotPowerOfTwo;

        let c_buf = alloc.alloc(Complex, n) catch return FftError.OutOfMemory;
        defer alloc.free(c_buf);
        for (i of 0..n) {
            c_buf[i] = Complex{ re: signal[i], im: 0.0 };
        }
        Fft.forwardInPlace(c_buf)?;

        const half = n >> 1;
        const out_len = half + 1;
        let mag = alloc.alloc(f64, out_len) catch return FftError.OutOfMemory;

        const sqrt2 = @sqrt(2.0);
        mag[0] = c_buf[0].abs();
        if (n >= 2) {
            for (k of 1..half) {
                mag[k] = sqrt2 * c_buf[k].abs();
            }
            mag[half] = c_buf[half].abs();
        }
        return mag;
    }

    /// Identifies the strongest non-DC frequency bin (`1..N/2`) and returns its bin index, frequency in Hz, and magnitude.
    static fn dominantFrequency(alloc: std.mem.Allocator, signal: []const f64, sample_rate_hz: f64): FrequencyPeak ! FftError {
        if (sample_rate_hz <= 0.0 || std.math.isNan(sample_rate_hz) || std.math.isInf(sample_rate_hz)) {
            return FftError.InvalidSampleRate;
        }
        const n = signal.len;
        if (n == 0) return FftError.EmptyInput;
        if (!Fft.isPowerOfTwo(n)) return FftError.NotPowerOfTwo;

        if (n < 2) {
            return FrequencyPeak{
                bin: 0,
                freq_hz: 0.0,
                magnitude: @abs(signal[0]),
            };
        }

        let c_buf = alloc.alloc(Complex, n) catch return FftError.OutOfMemory;
        defer alloc.free(c_buf);
        for (i of 0..n) {
            c_buf[i] = Complex{ re: signal[i], im: 0.0 };
        }
        Fft.forwardInPlace(c_buf)?;

        const half = n >> 1;
        let best_bin: usize = 1;
        let best_mag: f64 = c_buf[1].abs();
        for (k of 2..(half + 1)) {
            const m = c_buf[k].abs();
            if (m > best_mag) {
                best_mag = m;
                best_bin = usize(k);
            }
        }

        const freq_hz = (best_bin as f64) * sample_rate_hz / (n as f64);
        return FrequencyPeak{
            bin: best_bin,
            freq_hz: freq_hz,
            magnitude: best_mag,
        };
    }

    /// Computes linear convolution of `a` and `b` via zero-padded Radix-2 FFT.
    /// Returns a newly allocated slice of length `a.len + b.len - 1`.
    static fn convolve(alloc: std.mem.Allocator, a: []const f64, b: []const f64): []f64 ! FftError {
        if (a.len == 0 || b.len == 0) return FftError.EmptyInput;

        const out_len = a.len + b.len - 1;
        const fft_len = Fft.nextPowerOfTwo(out_len);

        let fa = alloc.alloc(Complex, fft_len) catch return FftError.OutOfMemory;
        defer alloc.free(fa);
        let fb = alloc.alloc(Complex, fft_len) catch return FftError.OutOfMemory;
        defer alloc.free(fb);

        for (i_raw of 0..fft_len) {
            const i = usize(i_raw);
            fa[i] = if (i < a.len) Complex{ re: a[i], im: 0.0 } else Complex{ re: 0.0, im: 0.0 };
            fb[i] = if (i < b.len) Complex{ re: b[i], im: 0.0 } else Complex{ re: 0.0, im: 0.0 };
        }

        Fft.forwardInPlace(fa)?;
        Fft.forwardInPlace(fb)?;

        for (i of 0..fft_len) {
            fa[i] = fa[i].mul(fb[i]);
        }

        Fft.inverseInPlace(fa)?;

        let out = alloc.alloc(f64, out_len) catch return FftError.OutOfMemory;
        for (i of 0..out_len) {
            out[i] = fa[i].re;
        }
        return out;
    }
}
