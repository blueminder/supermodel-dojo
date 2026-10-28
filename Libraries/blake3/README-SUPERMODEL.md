# Vendored BLAKE3 (portable subset)

Official upstream sources from https://github.com/BLAKE3-team/BLAKE3 (`c/` directory),
commit pinned at fetch time. Only the portable C path is compiled: SIMD sources
(`blake3_sse41.c`, `blake3_avx2.c`, `blake3_avx512.c`, assembly files) are not vendored.

The portable path is enforced by these per-file defines (set in `Makefiles/Rules.inc`
and `MSVC/Supermodel.vcxproj`):

    BLAKE3_NO_SSE2 BLAKE3_NO_SSE41 BLAKE3_NO_AVX2 BLAKE3_NO_AVX512

Fast enough for ROM-sized inputs (~10 MB). See `LICENSE_A2` for licensing.
