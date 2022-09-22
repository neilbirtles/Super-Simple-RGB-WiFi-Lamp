#ifndef __INC_LIB8TION_SCALE_RGBW_H
#define __INC_LIB8TION_SCALE_RGBW_H






/// scale three one byte values by a fourth one, which is treated as
///         the numerator of a fraction whose demominator is 256
///         In other words, it computes r,g,b * (scale / 256)
///
///         THIS FUNCTION ALWAYS MODIFIES ITS ARGUMENTS IN PLACE

LIB8STATIC void nscale8x4( uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& w, fract8 scale)
{
#if SCALE8_C == 1
#if (FASTLED_SCALE8_FIXED == 1)
    uint16_t scale_fixed = scale + 1;
    r = (((uint16_t)r) * scale_fixed) >> 8;
    g = (((uint16_t)g) * scale_fixed) >> 8;
    b = (((uint16_t)b) * scale_fixed) >> 8;
    w = (((uint16_t)w) * scale_fixed) >> 8;
#else
    r = ((int)r * (int)(scale) ) >> 8;
    g = ((int)g * (int)(scale) ) >> 8;
    b = ((int)b * (int)(scale) ) >> 8;
    w = ((int)w * (int)(scale) ) >> 8;
#endif
#elif SCALE8_AVRASM == 1
    r = scale8_LEAVING_R1_DIRTY(r, scale);
    g = scale8_LEAVING_R1_DIRTY(g, scale);
    b = scale8_LEAVING_R1_DIRTY(b, scale);
    w = scale8_LEAVING_R1_DIRTY(w, scale);
    cleanup_R1();
#else
#error "No implementation for nscale8x3 available."
#endif
}

/// scale three one byte values by a fourth one, which is treated as
///         the numerator of a fraction whose demominator is 256
///         In other words, it computes r,g,b * (scale / 256), ensuring
/// that non-zero values passed in remain non zero, no matter how low the scale
/// argument.
///
///         THIS FUNCTION ALWAYS MODIFIES ITS ARGUMENTS IN PLACE
LIB8STATIC void nscale8x4_video( uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& w, fract8 scale)
{
#if SCALE8_C == 1
    uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
    r = (r == 0) ? 0 : (((int)r * (int)(scale) ) >> 8) + nonzeroscale;
    g = (g == 0) ? 0 : (((int)g * (int)(scale) ) >> 8) + nonzeroscale;
    b = (b == 0) ? 0 : (((int)b * (int)(scale) ) >> 8) + nonzeroscale;
    w = (w == 0) ? 0 : (((int)w * (int)(scale) ) >> 8) + nonzeroscale;
#elif SCALE8_AVRASM == 1
    nscale8_video_LEAVING_R1_DIRTY( r, scale);
    nscale8_video_LEAVING_R1_DIRTY( g, scale);
    nscale8_video_LEAVING_R1_DIRTY( b, scale);
    nscale8_video_LEAVING_R1_DIRTY( w, scale);
    cleanup_R1();
#else
#error "No implementation for nscale8x3 available."
#endif
}

///  scale two one byte values by a third one, which is treated as
///         the numerator of a fraction whose demominator is 256
///         In other words, it computes i,j * (scale / 256)
///
///         THIS FUNCTION ALWAYS MODIFIES ITS ARGUMENTS IN PLACE

LIB8STATIC void nscale8x2( uint8_t& i, uint8_t& j, fract8 scale)
{
#if SCALE8_C == 1
#if FASTLED_SCALE8_FIXED == 1
    uint16_t scale_fixed = scale + 1;
    i = (((uint16_t)i) * scale_fixed ) >> 8;
    j = (((uint16_t)j) * scale_fixed ) >> 8;
#else
    i = ((uint16_t)i * (uint16_t)(scale) ) >> 8;
    j = ((uint16_t)j * (uint16_t)(scale) ) >> 8;
#endif
#elif SCALE8_AVRASM == 1
    i = scale8_LEAVING_R1_DIRTY(i, scale);
    j = scale8_LEAVING_R1_DIRTY(j, scale);
    cleanup_R1();
#else
#error "No implementation for nscale8x2 available."
#endif
}

///  scale two one byte values by a third one, which is treated as
///         the numerator of a fraction whose demominator is 256
///         In other words, it computes i,j * (scale / 256), ensuring
/// that non-zero values passed in remain non zero, no matter how low the scale
/// argument.
///
///         THIS FUNCTION ALWAYS MODIFIES ITS ARGUMENTS IN PLACE


LIB8STATIC void nscale8x2_video( uint8_t& i, uint8_t& j, fract8 scale)
{
#if SCALE8_C == 1
    uint8_t nonzeroscale = (scale != 0) ? 1 : 0;
    i = (i == 0) ? 0 : (((int)i * (int)(scale) ) >> 8) + nonzeroscale;
    j = (j == 0) ? 0 : (((int)j * (int)(scale) ) >> 8) + nonzeroscale;
#elif SCALE8_AVRASM == 1
    nscale8_video_LEAVING_R1_DIRTY( i, scale);
    nscale8_video_LEAVING_R1_DIRTY( j, scale);
    cleanup_R1();
#else
#error "No implementation for nscale8x2 available."
#endif
}


/// scale a 16-bit unsigned value by an 8-bit value,
///         considered as numerator of a fraction whose denominator
///         is 256. In other words, it computes i * (scale / 256)

LIB8STATIC_ALWAYS_INLINE uint16_t scale16by8( uint16_t i, fract8 scale )
{
#if SCALE16BY8_C == 1
    uint16_t result;
#if FASTLED_SCALE8_FIXED == 1
    result = (i * (1+((uint16_t)scale))) >> 8;
#else
    result = (i * scale) / 256;
#endif
    return result;
#elif SCALE16BY8_AVRASM == 1
#if FASTLED_SCALE8_FIXED == 1
    uint16_t result = 0;
    asm volatile(
                 // result.A = HighByte( (i.A x scale) + i.A )
                 "  mul %A[i], %[scale]                 \n\t"
                 "  add r0, %A[i]                       \n\t"
            //   "  adc r1, [zero]                      \n\t"
            //   "  mov %A[result], r1                  \n\t"
                 "  adc %A[result], r1                  \n\t"
                 
                 // result.A-B += i.B x scale
                 "  mul %B[i], %[scale]                 \n\t"
                 "  add %A[result], r0                  \n\t"
                 "  adc %B[result], r1                  \n\t"

                 // cleanup r1
                 "  clr __zero_reg__                    \n\t"
                 
                 // result.A-B += i.B
                 "  add %A[result], %B[i]               \n\t"
                 "  adc %B[result], __zero_reg__        \n\t"

                 : [result] "+r" (result)
                 : [i] "r" (i), [scale] "r" (scale)
                 : "r0", "r1"
                 );
    return result;
#else
    uint16_t result = 0;
    asm volatile(
         // result.A = HighByte(i.A x j )
         "  mul %A[i], %[scale]                 \n\t"
         "  mov %A[result], r1                  \n\t"
         //"  clr %B[result]                      \n\t"

         // result.A-B += i.B x j
         "  mul %B[i], %[scale]                 \n\t"
         "  add %A[result], r0                  \n\t"
         "  adc %B[result], r1                  \n\t"

         // cleanup r1
         "  clr __zero_reg__                    \n\t"

         : [result] "+r" (result)
         : [i] "r" (i), [scale] "r" (scale)
         : "r0", "r1"
         );
    return result;
#endif
#else
    #error "No implementation for scale16by8 available."
#endif
}

/// scale a 16-bit unsigned value by a 16-bit value,
///         considered as numerator of a fraction whose denominator
///         is 65536. In other words, it computes i * (scale / 65536)

LIB8STATIC uint16_t scale16( uint16_t i, fract16 scale )
{
  #if SCALE16_C == 1
    uint16_t result;
#if FASTLED_SCALE8_FIXED == 1
    result = ((uint32_t)(i) * (1+(uint32_t)(scale))) / 65536;
#else
    result = ((uint32_t)(i) * (uint32_t)(scale)) / 65536;
#endif
    return result;
#elif SCALE16_AVRASM == 1
#if FASTLED_SCALE8_FIXED == 1
    // implemented sort of like
    //   result = ((i * scale) + i ) / 65536
    //
    // why not like this, you may ask?
    //   result = (i * (scale+1)) / 65536
    // the answer is that if scale is 65535, then scale+1
    // will be zero, which is not what we want.
    uint32_t result;
    asm volatile(
                 // result.A-B  = i.A x scale.A
                 "  mul %A[i], %A[scale]                 \n\t"
                 //  save results...
                 // basic idea:
                 //"  mov %A[result], r0                 \n\t"
                 //"  mov %B[result], r1                 \n\t"
                 // which can be written as...
                 "  movw %A[result], r0                   \n\t"
                 // Because we're going to add i.A-B to
                 // result.A-D, we DO need to keep both
                 // the r0 and r1 portions of the product
                 // UNlike in the 'unfixed scale8' version.
                 // So the movw here is needed.
                 : [result] "=r" (result)
                 : [i] "r" (i),
                 [scale] "r" (scale)
                 : "r0", "r1"
                 );
    
    asm volatile(
                 // result.C-D  = i.B x scale.B
                 "  mul %B[i], %B[scale]                 \n\t"
                 //"  mov %C[result], r0                 \n\t"
                 //"  mov %D[result], r1                 \n\t"
                 "  movw %C[result], r0                   \n\t"
                 : [result] "+r" (result)
                 : [i] "r" (i),
                 [scale] "r" (scale)
                 : "r0", "r1"
                 );
    
    const uint8_t  zero = 0;
    asm volatile(
                 // result.B-D += i.B x scale.A
                 "  mul %B[i], %A[scale]                 \n\t"
                 
                 "  add %B[result], r0                   \n\t"
                 "  adc %C[result], r1                   \n\t"
                 "  adc %D[result], %[zero]              \n\t"
                 
                 // result.B-D += i.A x scale.B
                 "  mul %A[i], %B[scale]                 \n\t"
                 
                 "  add %B[result], r0                   \n\t"
                 "  adc %C[result], r1                   \n\t"
                 "  adc %D[result], %[zero]              \n\t"
                 
                 // cleanup r1
                 "  clr r1                               \n\t"
                 
                 : [result] "+r" (result)
                 : [i] "r" (i),
                 [scale] "r" (scale),
                 [zero] "r" (zero)
                 : "r0", "r1"
                 );

    asm volatile(
                 // result.A-D += i.A-B
                 "  add %A[result], %A[i]                \n\t"
                 "  adc %B[result], %B[i]                \n\t"
                 "  adc %C[result], %[zero]              \n\t"
                 "  adc %D[result], %[zero]              \n\t"
                 : [result] "+r" (result)
                 : [i] "r" (i),
                 [zero] "r" (zero)
                 );
    
    result = result >> 16;
    return result;
#else
    uint32_t result;
    asm volatile(
                 // result.A-B  = i.A x scale.A
                 "  mul %A[i], %A[scale]                 \n\t"
                 //  save results...
                 // basic idea:
                 //"  mov %A[result], r0                 \n\t"
                 //"  mov %B[result], r1                 \n\t"
                 // which can be written as...
                 "  movw %A[result], r0                   \n\t"
                 // We actually don't need to do anything with r0,
                 // as result.A is never used again here, so we
                 // could just move the high byte, but movw is
                 // one clock cycle, just like mov, so might as
                 // well, in case we want to use this code for
                 // a generic 16x16 multiply somewhere.

                 : [result] "=r" (result)
                 : [i] "r" (i),
                   [scale] "r" (scale)
                 : "r0", "r1"
                 );

    asm volatile(
                 // result.C-D  = i.B x scale.B
                 "  mul %B[i], %B[scale]                 \n\t"
                 //"  mov %C[result], r0                 \n\t"
                 //"  mov %D[result], r1                 \n\t"
                 "  movw %C[result], r0                   \n\t"
                 : [result] "+r" (result)
                 : [i] "r" (i),
                   [scale] "r" (scale)
                 : "r0", "r1"
                 );

    const uint8_t  zero = 0;
    asm volatile(
                 // result.B-D += i.B x scale.A
                 "  mul %B[i], %A[scale]                 \n\t"

                 "  add %B[result], r0                   \n\t"
                 "  adc %C[result], r1                   \n\t"
                 "  adc %D[result], %[zero]              \n\t"

                 // result.B-D += i.A x scale.B
                 "  mul %A[i], %B[scale]                 \n\t"

                 "  add %B[result], r0                   \n\t"
                 "  adc %C[result], r1                   \n\t"
                 "  adc %D[result], %[zero]              \n\t"

                 // cleanup r1
                 "  clr r1                               \n\t"

                 : [result] "+r" (result)
                 : [i] "r" (i),
                   [scale] "r" (scale),
                   [zero] "r" (zero)
                 : "r0", "r1"
                 );

    result = result >> 16;
    return result;
#endif
#else
    #error "No implementation for scale16 available."
#endif
}
///@}

///@defgroup Dimming Dimming and brightening functions
///
/// Dimming and brightening functions
///
/// The eye does not respond in a linear way to light.
/// High speed PWM'd LEDs at 50% duty cycle appear far
/// brighter then the 'half as bright' you might expect.
///
/// If you want your midpoint brightness leve (128) to
/// appear half as bright as 'full' brightness (255), you
/// have to apply a 'dimming function'.
///@{

/// Adjust a scaling value for dimming
LIB8STATIC uint8_t dim8_raw( uint8_t x)
{
    return scale8( x, x);
}

/// Adjust a scaling value for dimming for video (value will never go below 1)
LIB8STATIC uint8_t dim8_video( uint8_t x)
{
    return scale8_video( x, x);
}

/// Linear version of the dimming function that halves for values < 128
LIB8STATIC uint8_t dim8_lin( uint8_t x )
{
    if( x & 0x80 ) {
        x = scale8( x, x);
    } else {
        x += 1;
        x /= 2;
    }
    return x;
}

/// inverse of the dimming function, brighten a value
LIB8STATIC uint8_t brighten8_raw( uint8_t x)
{
    uint8_t ix = 255 - x;
    return 255 - scale8( ix, ix);
}

/// inverse of the dimming function, brighten a value
LIB8STATIC uint8_t brighten8_video( uint8_t x)
{
    uint8_t ix = 255 - x;
    return 255 - scale8_video( ix, ix);
}

/// inverse of the dimming function, brighten a value
LIB8STATIC uint8_t brighten8_lin( uint8_t x )
{
    uint8_t ix = 255 - x;
    if( ix & 0x80 ) {
        ix = scale8( ix, ix);
    } else {
        ix += 1;
        ix /= 2;
    }
    return 255 - ix;
}

///@}
#endif
