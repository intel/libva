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

/**
 * Function visibility
 */
#if defined(__GNUC__)
#  define DLL_HIDDEN __attribute__((visibility("hidden")))
#  define DLL_EXPORT __attribute__((visibility("default")))
#else
#  define DLL_HIDDEN
#  define DLL_EXPORT
#endif

#endif /* _INTEL_COMPILER_H_ */
