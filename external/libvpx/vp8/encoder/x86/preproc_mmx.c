


#include "memory.h"
#include "preproc.h"
#include "pragmas.h"

#define FRAMECOUNT 7
#define ROUNDUP32(X) ( ( ( (unsigned long) X ) + 31 )&( 0xFFFFFFE0 ) )

extern void vpx_get_processor_flags(int *mmx_enabled, int *xmm_enabled, int *wmt_enabled);

void (*temp_filter)(pre_proc_instance *ppi, unsigned char *s, unsigned char *d, int bytes, int strength);

void temp_filter_wmt
(
    pre_proc_instance *ppi,
    unsigned char *s,
    unsigned char *d,
    int bytes,
    int strength
)
{
    int byte = 0;
    unsigned char *frameptr = ppi->frame_buffer;

    __declspec(align(16)) unsigned short threes[]  = { 3, 3, 3, 3, 3, 3, 3, 3};
    __declspec(align(16)) unsigned short sixteens[] = {16, 16, 16, 16, 16, 16, 16, 16};

    if (ppi->frame == 0)
    {
        do
        {
            int i;
            int frame = 0;

            do
            {
                for (i = 0; i < 8; i++)
                {
                    *frameptr = s[byte+i];
                    ++frameptr;
                }

                ++frame;
            }
            while (frame < FRAMECOUNT);

            for (i = 0; i < 8; i++)
                d[byte+i] = s[byte+i];

            byte += 8;

        }
        while (byte < bytes);
    }
    else
    {
        int i;
        int offset2 = (ppi->frame % FRAMECOUNT);

        do
        {
            __declspec(align(16)) unsigned short counts[8];
            __declspec(align(16)) unsigned short sums[8];
            __asm
            {
                mov         eax, offset2
                mov         edi, s                  // source pixels
                pxor        xmm1, xmm1              // accumulator

                pxor        xmm7, xmm7

                mov         esi, frameptr           // accumulator
                pxor        xmm2, xmm2              // count

                movq        xmm3, QWORD PTR [edi]

                movq        QWORD PTR [esi+8*eax], xmm3

                punpcklbw   xmm3, xmm2              // xmm3 source pixels
                mov         ecx,  FRAMECOUNT

                next_frame:
                movq        xmm4, QWORD PTR [esi]   // get frame buffer values
                punpcklbw   xmm4, xmm7              // xmm4 frame buffer pixels
                movdqa      xmm6, xmm4              // save the pixel values
                psubsw      xmm4, xmm3              // subtracted pixel values
                pmullw      xmm4, xmm4              // square xmm4
                movd        xmm5, strength
                psrlw       xmm4, xmm5              // should be strength
                pmullw      xmm4, threes            // 3 * modifier
                movdqa      xmm5, sixteens          // 16s
                psubusw     xmm5, xmm4              // 16 - modifiers
                movdqa      xmm4, xmm5              // save the modifiers
                pmullw      xmm4, xmm6              // multiplier values
                paddusw     xmm1, xmm4              // accumulator
                paddusw     xmm2, xmm5              // count
                add         esi, 8                  // next frame
                dec         ecx                     // next set of eight pixels
                jnz         next_frame

                movdqa      counts, xmm2
                psrlw       xmm2, 1                 // divide count by 2 for rounding
                paddusw     xmm1, xmm2              // rounding added in

                mov         frameptr, esi

                movdqa      sums, xmm1
            }

            for (i = 0; i < 8; i++)
            {
                int blurvalue = sums[i] * ppi->fixed_divide[counts[i]];
                blurvalue >>= 16;
                d[i] = blurvalue;
            }

            s += 8;
            d += 8;
            byte += 8;
        }
        while (byte < bytes);
    }

    ++ppi->frame;
    __asm emms
}

void temp_filter_mmx
(
    pre_proc_instance *ppi,
    unsigned char *s,
    unsigned char *d,
    int bytes,
    int strength
)
{
    int byte = 0;
    unsigned char *frameptr = ppi->frame_buffer;

    __declspec(align(16)) unsigned short threes[]  = { 3, 3, 3, 3};
    __declspec(align(16)) unsigned short sixteens[] = {16, 16, 16, 16};

    if (ppi->frame == 0)
    {
        do
        {
            int i;
            int frame = 0;

            do
            {
                for (i = 0; i < 4; i++)
                {
                    *frameptr = s[byte+i];
                    ++frameptr;
                }

                ++frame;
            }
            while (frame < FRAMECOUNT);

            for (i = 0; i < 4; i++)
                d[byte+i] = s[byte+i];

            byte += 4;

        }
        while (byte < bytes);
    }
    else
    {
        int i;
        int offset2 = (ppi->frame % FRAMECOUNT);

        do
        {
            __declspec(align(16)) unsigned short counts[8];
            __declspec(align(16)) unsigned short sums[8];
            __asm
            {

                mov         eax, offset2
                mov         edi, s                  // source pixels
                pxor        mm1, mm1                // accumulator
                pxor        mm7, mm7

                mov         esi, frameptr           // accumulator
                pxor        mm2, mm2                // count

                movd        mm3, DWORD PTR [edi]
                movd        DWORD PTR [esi+4*eax], mm3

                punpcklbw   mm3, mm2                // mm3 source pixels
                mov         ecx,  FRAMECOUNT

                next_frame:
                movd        mm4, DWORD PTR [esi]    // get frame buffer values
                punpcklbw   mm4, mm7                // mm4 frame buffer pixels
                movq        mm6, mm4                // save the pixel values
                psubsw      mm4, mm3                // subtracted pixel values
                pmullw      mm4, mm4                // square mm4
                movd        mm5, strength
                psrlw       mm4, mm5                // should be strength
                pmullw      mm4, threes             // 3 * modifier
                movq        mm5, sixteens           // 16s
                psubusw     mm5, mm4                // 16 - modifiers
                movq        mm4, mm5                // save the modifiers
                pmullw      mm4, mm6                // multiplier values
                paddusw     mm1, mm4                // accumulator
                paddusw     mm2, mm5                // count
                add         esi, 4                  // next frame
                dec         ecx                     // next set of eight pixels
                jnz         next_frame

                movq        counts, mm2
                psrlw       mm2, 1                  // divide count by 2 for rounding
                paddusw     mm1, mm2                // rounding added in

                mov         frameptr, esi

                movq        sums, mm1

            }

            for (i = 0; i < 4; i++)
            {
                int blurvalue = sums[i] * ppi->fixed_divide[counts[i]];
                blurvalue >>= 16;
                d[i] = blurvalue;
            }

            s += 4;
            d += 4;
            byte += 4;
        }
        while (byte < bytes);
    }

    ++ppi->frame;
    __asm emms
}
