#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#ifdef __AVX__
  #include <immintrin.h>
#else
  #warning No AVX support - will not compile
#endif

void sqrtSerial(int N,
                float initialGuess,
                float values[],
                float output[])
{

    static const float kThreshold = 0.00001f;

    for (int i=0; i<N; i++) {

        float x = values[i];
        float guess = initialGuess;

        float error = fabs(guess * guess * x - 1.f);

        while (error > kThreshold) {
            guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
            error = fabs(guess * guess * x - 1.f);
        }

        output[i] = x * guess;
    }
}

void sqrtSimd256(int N,
                float initialGuess,
                float values[],
                float output[])
{
    static const float kThreshold = 0.00001f;
    __mmask8 mask;
    __m256 guess, x, tmp, left, right;
    __m256 const_one = _mm256_set1_ps(1.0f);
    __m256 const_two = _mm256_set1_ps(2.0f);
    __m256 const_three = _mm256_set1_ps(3.0f);
    __m256 threshold = _mm256_set1_ps(kThreshold);
    __m256 ABS = _mm256_set1_ps(-0.0f);

    for (int i = 0; i < N; i += 8) {
        x = _mm256_load_ps(values + i);                           // float x = values[i];
        guess = _mm256_set1_ps(initialGuess);                     // float guess = initialGuess;
        mask = 0xFF;
        tmp = _mm256_mask_mul_ps(tmp, mask, guess, guess);        // 
        tmp = _mm256_mask_mul_ps(tmp, mask, tmp, x);              //
        tmp = _mm256_mask_sub_ps(tmp, mask, tmp, const_one);      //
        tmp = _mm256_mask_andnot_ps(tmp, mask, ABS, tmp);         // float error = fabs(guess * guess * x - 1.f);
        // 比较的操作条件 greater than 30: OP := _CMP_GT_OQ
        mask = mask & (_mm256_cmp_ps_mask(tmp, threshold, (unsigned char)30));

        while (mask) {
            left = _mm256_mask_mul_ps(guess, mask, const_three, guess); // left = 3.f * guess
            right = _mm256_mask_mul_ps(guess, mask, guess, guess);      //
            right = _mm256_mask_mul_ps(right, mask, right, guess);      // 
            right = _mm256_mask_mul_ps(right, mask, right, x);          // right = x * guess * guess * guess

            left = _mm256_mask_sub_ps(left, mask, left, right);         //
            guess = _mm256_mask_div_ps(left, mask, left, const_two);    // guess = (3.f * guess - x * guess * guess * guess) * 0.5f;

            tmp = _mm256_mask_mul_ps(guess, mask, guess, guess);        // 
            tmp = _mm256_mask_mul_ps(tmp, mask, tmp, x);                //
            tmp = _mm256_mask_sub_ps(tmp, mask, tmp, const_one);        // float error = guess * guess * x - 1.f
            tmp = _mm256_mask_andnot_ps(tmp, mask, ABS, tmp);           // error = fabs(error);
            mask = mask & (_mm256_cmp_ps_mask(tmp, threshold, (unsigned char)30));
        }

        guess = _mm256_mask_mul_ps(guess, 0xFF, x, guess);
        _mm256_store_ps(output + i, guess);
    }
}
