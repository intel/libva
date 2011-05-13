#ifndef _INTEL_COMPILER_H_
#define _INTEL_COMPILER_H_

/**
 * Function inlining
 */
#if defined(__GNUC__)
#  define INLINE __inline__
#elif (__STDC_VERSION__ >= 199901L) /* C99 */
#  define INLINE inline
#else
#  define INLINE
#endif

#endif /* _INTEL_COMPILER_H_ */
