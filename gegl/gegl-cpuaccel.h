/* LIBGEGL - The GEGL Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GEGL_CPU_ACCEL_H__
#define __GEGL_CPU_ACCEL_H__

G_BEGIN_DECLS


typedef enum
{
  GEGL_CPU_ACCEL_NONE        = 0x0,

  /* x86 accelerations */
  GEGL_CPU_ACCEL_X86_MMX     = 0x80000000,
  GEGL_CPU_ACCEL_X86_3DNOW   = 0x40000000,
  GEGL_CPU_ACCEL_X86_MMXEXT  = 0x20000000,
  GEGL_CPU_ACCEL_X86_SSE     = 0x10000000,
  GEGL_CPU_ACCEL_X86_SSE2    = 0x08000000,
  GEGL_CPU_ACCEL_X86_SSE3    = 0x04000000,
  GEGL_CPU_ACCEL_X86_SSSE3   = 0x02000000,
  GEGL_CPU_ACCEL_X86_SSE4_1  = 0x01000000,
  GEGL_CPU_ACCEL_X86_SSE4_2  = 0x00800000,
  GEGL_CPU_ACCEL_X86_AVX     = 0x00400000,
  GEGL_CPU_ACCEL_X86_POPCNT  = 0x00200000,
  GEGL_CPU_ACCEL_X86_FMA     = 0x00100000,
  GEGL_CPU_ACCEL_X86_MOVBE   = 0x00080000,
  GEGL_CPU_ACCEL_X86_F16C    = 0x00040000,
  GEGL_CPU_ACCEL_X86_XSAVE   = 0x00020000,
  GEGL_CPU_ACCEL_X86_OSXSAVE = 0x00010000,
  GEGL_CPU_ACCEL_X86_BMI1    = 0x00008000,
  GEGL_CPU_ACCEL_X86_BMI2    = 0x00004000,
  GEGL_CPU_ACCEL_X86_AVX2    = 0x00002000,

  GEGL_CPU_ACCEL_X86_64_V2 =
    (GEGL_CPU_ACCEL_X86_POPCNT|
     GEGL_CPU_ACCEL_X86_SSE4_1|
     GEGL_CPU_ACCEL_X86_SSE4_2|
     GEGL_CPU_ACCEL_X86_SSSE3),

  GEGL_CPU_ACCEL_X86_64_V3 =
    (GEGL_CPU_ACCEL_X86_64_V2|
     GEGL_CPU_ACCEL_X86_BMI1|
     GEGL_CPU_ACCEL_X86_BMI2|
     GEGL_CPU_ACCEL_X86_AVX|
     GEGL_CPU_ACCEL_X86_FMA|
     GEGL_CPU_ACCEL_X86_F16C|
     GEGL_CPU_ACCEL_X86_AVX2|
     GEGL_CPU_ACCEL_X86_OSXSAVE|
     GEGL_CPU_ACCEL_X86_MOVBE),

  /* powerpc accelerations */
  GEGL_CPU_ACCEL_PPC_ALTIVEC = 0x00000010,

  /* arm accelerations */
  GEGL_CPU_ACCEL_ARM_NEON    = 0x00000020,
} GeglCpuAccelFlags;


GeglCpuAccelFlags  gegl_cpu_accel_get_support (void);


G_END_DECLS

#endif  /* __GEGL_CPU_ACCEL_H__ */
