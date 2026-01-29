#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <cstdint>

static inline void hann_window(std::vector<float> &x)
{
  const size_t N = x.size();
  if (N < 2) return;
  for (size_t n = 0; n < N; n++) {
    float w = 0.5f - 0.5f * std::cos(2.0f * float(M_PI) * float(n) / float(N - 1));
    x[n] *= w;
  }
}

static inline void fft_inplace(std::vector<std::complex<float>> &a)
{
  const size_t N = a.size();
  for (size_t i = 1, j = 0; i < N; i++) {
    size_t bit = N >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }

  for (size_t len = 2; len <= N; len <<= 1) {
    float ang = -2.0f * float(M_PI) / float(len);
    std::complex<float> wlen(std::cos(ang), std::sin(ang));
    for (size_t i = 0; i < N; i += len) {
      std::complex<float> w(1.0f, 0.0f);
      for (size_t j = 0; j < len/2; j++) {
        auto u = a[i + j];
        auto v = a[i + j + len/2] * w;
        a[i + j] = u + v;
        a[i + j + len/2] = u - v;
        w *= wlen;
      }
    }
  }
}

static inline void real_fft_mag_0_to_nyquist(
  const std::vector<float> &input,
  std::vector<float> &out_mag)
{
  const size_t N = input.size();
  std::vector<std::complex<float>> a(N);
  for (size_t i = 0; i < N; i++) a[i] = {input[i], 0.0f};

  fft_inplace(a);

  const size_t nyq = N/2;
  out_mag.resize(nyq);
  for (size_t k = 0; k < nyq; k++) {
    float re = a[k].real();
    float im = a[k].imag();
    out_mag[k] = std::sqrt(re*re + im*im);
  }
}