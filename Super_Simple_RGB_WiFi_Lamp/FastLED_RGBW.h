/* FastLED_RGBW
 * 
 * Hack to enable SK6812 RGBW strips to work with FastLED.
 *
 * Original code by Jim Bumgardner (http://krazydad.com).
 * Modified by David Madison (http://partsnotincluded.com).
 * 
*/

#ifndef FastLED_RGBW_h
#define FastLED_RGBW_h

struct CRGBW;

/// Forward declaration of hsv2rgb_rainbow here,
/// to avoid circular dependencies.
extern void hsv2rgb_rainbow( const CHSV& hsv, CRGBW& rgbw);

/// Clean up the r1 register after a series of *LEAVING_R1_DIRTY calls
// LIB8STATIC_ALWAYS_INLINE void cleanup_R1()
// {
// #if CLEANUP_R1_AVRASM == 1
//     // Restore r1 to "0"; it's expected to always be that
//     asm volatile( "clr __zero_reg__  \n\t" : : : "r1" );
// #endif
// }

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



struct CRGBW  {
	union {
		struct {
			union {
				uint8_t g;
				uint8_t green;
			};
			union {
				uint8_t r;
				uint8_t red;
			};
			union {
				uint8_t b;
				uint8_t blue;
			};
			union {
				uint8_t w;
				uint8_t white;
			};
		};
		uint8_t raw[4];
	};

	CRGBW(){}

	CRGBW(uint8_t rd, uint8_t grn, uint8_t blu, uint8_t wht){
		r = rd;
		g = grn;
		b = blu;
		w = wht;
	}

  /// allow assignment from HSV color
  inline CRGBW& operator = (const CHSV& rhs) __attribute__((always_inline))
  {
      hsv2rgb_rainbow( rhs, *this);
      return *this;
  }

	// inline void operator = (const CRGB c) __attribute__((always_inline)){ 
	// 	this->r = c.r;
	// 	this->g = c.g;
	// 	this->b = c.b;
	// 	this->white = 0;
	// }

  /// allow assignment from one RGB struct to another
	inline CRGBW& operator= (const CRGB& rhs) __attribute__((always_inline))
  {
      r = rhs.r;
      g = rhs.g;
      b = rhs.b;
      w = 0;
      return *this;
  }

  /// allow assignment from one RGB struct to another
	inline CRGBW& operator= (const CRGBW& rhs) __attribute__((always_inline))
  {
      r = rhs.r;
      g = rhs.g;
      b = rhs.b;
      w = rhs.w;
      return *this;
  }

  /// add one RGB to another, saturating at 0xFF for each channel
  inline CRGBW& operator+= (const CRGBW& rhs )
  {
      r = qadd8( r, rhs.r);
      g = qadd8( g, rhs.g);
      b = qadd8( b, rhs.b);
      w = qadd8( w, rhs.w);
      return *this;
  }

  /// add one RGB to another, saturating at 0xFF for each channel
  inline CRGBW& operator+= (const CRGB& rhs )
  {
      r = qadd8( r, rhs.r);
      g = qadd8( g, rhs.g);
      b = qadd8( b, rhs.b);
      w = 0;
      return *this;
  }

  /// %= is a synonym for nscale8_video.  Think of it is scaling down
  /// by "a percentage"
  inline CRGBW& operator%= (uint8_t scaledown )
  {
      nscale8x4_video( r, g, b, w, scaledown);
      return *this;
  }  

  /// scale down a RGB to N 256ths of it's current brightness, using
  /// 'plain math' dimming rules, which means that if the low light levels
  /// may dim all the way to 100% black.
  inline CRGBW& nscale8 (uint8_t scaledown )
  {
      nscale8x4( r, g, b, w, scaledown);
      return *this;
  }

  /// allow assignment from H, S, and V
	inline CRGBW& setHSV (uint8_t hue, uint8_t sat, uint8_t val) __attribute__((always_inline))
  {
      hsv2rgb_rainbow( CHSV(hue, sat, val), *this);
      return *this;
  }
};

inline uint16_t getRGBWsize(uint16_t nleds){
	uint16_t nbytes = nleds * 4;
	if(nbytes % 3 > 0) return nbytes / 3 + 1;
	else return nbytes / 3;
}

void nscale8( CRGBW* leds, uint16_t num_leds, uint8_t scale)
{
    for( uint16_t i = 0; i < num_leds; i++) {
        leds[i].nscale8( scale);
    }
}

void fadeToBlackBy( CRGBW* leds, uint16_t num_leds, uint8_t fadeBy)
{
    nscale8( leds, num_leds, 255 - fadeBy);
}

void fill_solid( struct CRGBW * leds, int numToFill,
                 const struct CRGB& color)
{
    for( int i = 0; i < numToFill; i++) {
        leds[i] = color;
    }
}

void fill_solid( struct CRGBW * leds, int numToFill,
                 const struct CRGBW& color)
{
    for( int i = 0; i < numToFill; i++) {
        leds[i] = color;
    }
}

// Sometimes the compiler will do clever things to reduce
// code size that result in a net slowdown, if it thinks that
// a variable is not used in a certain location.
// This macro does its best to convince the compiler that
// the variable is used in this location, to help control
// code motion and de-duplication that would result in a slowdown.
#define FORCE_REFERENCE(var)  asm volatile( "" : : "r" (var) )


#define K255 255
#define K171 171
#define K170 170
#define K85  85

void hsv2rgb_rainbow( const CHSV& hsv, CRGBW& rgbw)
{
    // Yellow has a higher inherent brightness than
    // any other color; 'pure' yellow is perceived to
    // be 93% as bright as white.  In order to make
    // yellow appear the correct relative brightness,
    // it has to be rendered brighter than all other
    // colors.
    // Level Y1 is a moderate boost, the default.
    // Level Y2 is a strong boost.
    const uint8_t Y1 = 1;
    const uint8_t Y2 = 0;
    
    // G2: Whether to divide all greens by two.
    // Depends GREATLY on your particular LEDs
    const uint8_t G2 = 0;
    
    // Gscale: what to scale green down by.
    // Depends GREATLY on your particular LEDs
    const uint8_t Gscale = 0;
    
    
    uint8_t hue = hsv.hue;
    uint8_t sat = hsv.sat;
    uint8_t val = hsv.val;
    
    uint8_t offset = hue & 0x1F; // 0..31
    
    // offset8 = offset * 8
    uint8_t offset8 = offset;
    {
#if defined(__AVR__)
        // Left to its own devices, gcc turns "x <<= 3" into a loop
        // It's much faster and smaller to just do three single-bit shifts
        // So this business is to force that.
        offset8 <<= 1;
        asm volatile("");
        offset8 <<= 1;
        asm volatile("");
        offset8 <<= 1;
#else
        // On ARM and other non-AVR platforms, we just shift 3.
        offset8 <<= 3;
#endif
    }
    
    uint8_t third = scale8( offset8, (256 / 3)); // max = 85
    
    uint8_t r, g, b;
    
    if( ! (hue & 0x80) ) {
        // 0XX
        if( ! (hue & 0x40) ) {
            // 00X
            //section 0-1
            if( ! (hue & 0x20) ) {
                // 000
                //case 0: // R -> O
                r = K255 - third;
                g = third;
                b = 0;
                FORCE_REFERENCE(b);
            } else {
                // 001
                //case 1: // O -> Y
                if( Y1 ) {
                    r = K171;
                    g = K85 + third ;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if( Y2 ) {
                    r = K170 + third;
                    //uint8_t twothirds = (third << 1);
                    uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                    g = K85 + twothirds;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            }
        } else {
            //01X
            // section 2-3
            if( !  (hue & 0x20) ) {
                // 010
                //case 2: // Y -> G
                if( Y1 ) {
                    //uint8_t twothirds = (third << 1);
                    uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                    r = K171 - twothirds;
                    g = K170 + third;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
                if( Y2 ) {
                    r = K255 - offset8;
                    g = K255;
                    b = 0;
                    FORCE_REFERENCE(b);
                }
            } else {
                // 011
                // case 3: // G -> A
                r = 0;
                FORCE_REFERENCE(r);
                g = K255 - third;
                b = third;
            }
        }
    } else {
        // section 4-7
        // 1XX
        if( ! (hue & 0x40) ) {
            // 10X
            if( ! ( hue & 0x20) ) {
                // 100
                //case 4: // A -> B
                r = 0;
                FORCE_REFERENCE(r);
                //uint8_t twothirds = (third << 1);
                uint8_t twothirds = scale8( offset8, ((256 * 2) / 3)); // max=170
                g = K171 - twothirds; //K170?
                b = K85  + twothirds;
                
            } else {
                // 101
                //case 5: // B -> P
                r = third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K255 - third;
                
            }
        } else {
            if( !  (hue & 0x20)  ) {
                // 110
                //case 6: // P -- K
                r = K85 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K171 - third;
                
            } else {
                // 111
                //case 7: // K -> R
                r = K170 + third;
                g = 0;
                FORCE_REFERENCE(g);
                b = K85 - third;
                
            }
        }
    }
    
    // This is one of the good places to scale the green down,
    // although the client can scale green down as well.
    if( G2 ) g = g >> 1;
    if( Gscale ) g = scale8_video_LEAVING_R1_DIRTY( g, Gscale);
    
    // Scale down colors if we're desaturated at all
    // and add the brightness_floor to r, g, and b.
    if( sat != 255 ) {
        if( sat == 0) {
            r = 255; b = 255; g = 255;
        } else {
            //nscale8x3_video( r, g, b, sat);
#if (FASTLED_SCALE8_FIXED==1)
            if( r ) r = scale8_LEAVING_R1_DIRTY( r, sat);
            if( g ) g = scale8_LEAVING_R1_DIRTY( g, sat);
            if( b ) b = scale8_LEAVING_R1_DIRTY( b, sat);
#else
            if( r ) r = scale8_LEAVING_R1_DIRTY( r, sat) + 1;
            if( g ) g = scale8_LEAVING_R1_DIRTY( g, sat) + 1;
            if( b ) b = scale8_LEAVING_R1_DIRTY( b, sat) + 1;
#endif
            cleanup_R1();
            
            uint8_t desat = 255 - sat;
            desat = scale8( desat, desat);
            
            uint8_t brightness_floor = desat;
            r += brightness_floor;
            g += brightness_floor;
            b += brightness_floor;
        }
    }
    
    // Now scale everything down if we're at value < 255.
    if( val != 255 ) {
        
        val = scale8_video_LEAVING_R1_DIRTY( val, val);
        if( val == 0 ) {
            r=0; g=0; b=0;
        } else {
            // nscale8x3_video( r, g, b, val);
#if (FASTLED_SCALE8_FIXED==1)
            if( r ) r = scale8_LEAVING_R1_DIRTY( r, val);
            if( g ) g = scale8_LEAVING_R1_DIRTY( g, val);
            if( b ) b = scale8_LEAVING_R1_DIRTY( b, val);
#else
            if( r ) r = scale8_LEAVING_R1_DIRTY( r, val) + 1;
            if( g ) g = scale8_LEAVING_R1_DIRTY( g, val) + 1;
            if( b ) b = scale8_LEAVING_R1_DIRTY( b, val) + 1;
#endif
            cleanup_R1();
        }
    }
    
    // Here we have the old AVR "missing std X+n" problem again
    // It turns out that fixing it winds up costing more than
    // not fixing it.
    // To paraphrase Dr Bronner, profile! profile! profile!
    //asm volatile(  ""  :  :  : "r26", "r27" );
    //asm volatile (" movw r30, r26 \n" : : : "r30", "r31");
    rgbw.r = r;
    rgbw.g = g;
    rgbw.b = b;
    rgbw.w = 0;
}

#endif