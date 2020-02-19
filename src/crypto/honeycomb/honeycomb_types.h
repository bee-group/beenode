#ifndef HONEYCOMB_TYPESH
#define HONEYCOMB_TYPESH

#include <limits.h>

//
// All our I/O functions are defined over octet streams. We do not know
// how to handle input data if bytes are not octets.
//
#if CHAR_BIT != 8
	#error This code requires 8-bit bytes
#endif

//
// We want to define the types "bee_u32" and "bee_u64" which hold
// unsigned values of at least, respectively, 32 and 64 bits. These
// tests should select appropriate types for most platforms. The
// macro "BEE_64" is defined if the 64-bit is supported.
//

#undef BEE_64
#undef BEE_64_TRUE

#if defined __STDC__ && __STDC_VERSION__ >= 199901L

	//
	// On C99 implementations, we can use <stdint.h> to get an exact 64-bit
	// type, if any, or otherwise use a wider type (which must exist, for
	// C99 conformance).
	//

	#include <stdint.h>

	#ifdef UINT32_MAX
		typedef uint32_t 		bee_u32;
		typedef int32_t 		bee_s32;
	#else
		typedef uint_fast32_t 	bee_u32;
		typedef int_fast32_t 	bee_s32;
	#endif

	#if !BEE_NO_64
		#ifdef UINT64_MAX
			typedef uint64_t 		bee_u64;
			typedef int64_t 		bee_s64;
		#else
			typedef uint_fast64_t 	bee_u64;
			typedef int_fast64_t 	bee_s64;
		#endif
	#endif

	#define BEE_C32(x)    ((bee_u32)(x))

	#if !BEE_NO_64
		#define BEE_C64(x)    ((bee_u64)(x))
		#define BEE_64  1
	#endif

#else

	//
	// On non-C99 systems, we use "unsigned int" if it is wide enough,
	// "unsigned long" otherwise. This supports all "reasonable" architectures.
	// We have to be cautious: pre-C99 preprocessors handle constants
	// differently in '#if' expressions. Hence the shifts to test UINT_MAX.
	//

	#if ((UINT_MAX >> 11) >> 11) >= 0x3FF

		typedef unsigned int bee_u32;
		typedef int bee_s32;

		#define BEE_C32(x)    ((bee_u32)(x ## U))

	#else

		typedef unsigned long bee_u32;
		typedef long bee_s32;

		#define BEE_C32(x)    ((bee_u32)(x ## UL))

	#endif

	#if !BEE_NO_64

		//
		// We want a 64-bit type. We use "unsigned long" if it is wide enough (as
		// is common on 64-bit architectures such as AMD64, Alpha or Sparcv9),
		// "unsigned long long" otherwise, if available. We use ULLONG_MAX to
		// test whether "unsigned long long" is available; we also know that
		// gcc features this type, even if the libc header do not know it.
		//

		#if ((ULONG_MAX >> 31) >> 31) >= 3

			typedef unsigned long bee_u64;
			typedef long bee_s64;

			#define BEE_C64(x)    ((bee_u64)(x ## UL))

			#define BEE_64  1

		#elif ((ULLONG_MAX >> 31) >> 31) >= 3 || defined __GNUC__

			typedef unsigned long long bee_u64;
			typedef long long bee_s64;

			#define BEE_C64(x)    ((bee_u64)(x ## ULL))

			#define BEE_64  1

		#else

			//
			// No 64-bit type...
			//

		#endif

	#endif

#endif


//
// If the "unsigned long" type has length 64 bits or more, then this is
// a "true" 64-bit architectures. This is also true with Visual C on
// amd64, even though the "long" type is limited to 32 bits.
//
#if BEE_64 && (((ULONG_MAX >> 31) >> 31) >= 3 || defined _M_X64)
	#define BEE_64_TRUE   1
#endif

//
// Implementation note: some processors have specific opcodes to perform
// a rotation. Recent versions of gcc recognize the expression above and
// use the relevant opcodes, when appropriate.
//

#define BEE_T32(x)    ((x) & BEE_C32(0xFFFFFFFF))
#define BEE_ROTL32(x, n)   BEE_T32(((x) << (n)) | ((x) >> (32 - (n))))
#define BEE_ROTR32(x, n)   BEE_ROTL32(x, (32 - (n)))

#if BEE_64
	#define BEE_T64(x)    ((x) & BEE_C64(0xFFFFFFFFFFFFFFFF))
	#define BEE_ROTL64(x, n)   BEE_T64(((x) << (n)) | ((x) >> (64 - (n))))
	#define BEE_ROTR64(x, n)   BEE_ROTL64(x, (64 - (n)))
#endif

#ifndef DOXYGEN_IGNORE
	//
	// Define BEE_INLINE to be an "inline" qualifier, if available. We define
	// some small macro-like functions which benefit greatly from being inlined.
	//
	#if (defined __STDC__ && __STDC_VERSION__ >= 199901L) || defined __GNUC__
		#define BEE_INLINE inline
	#elif defined _MSC_VER
		#define BEE_INLINE __inline
	#else
		#define BEE_INLINE
	#endif
#endif

//
// We define some macros which qualify the architecture. These macros
// may be explicit set externally (e.g. as compiler parameters). The
// code below sets those macros if they are not already defined.
//
// Most macros are boolean, thus evaluate to either zero or non-zero.
// The BEE_UPTR macro is special, in that it evaluates to a C type,
// or is not defined.
//
// BEE_UPTR             if defined: unsigned type to cast pointers into
//
// BEE_UNALIGNED        non-zero if unaligned accesses are efficient
// BEE_LITTLE_ENDIAN    non-zero if architecture is known to be little-endian
// BEE_BIG_ENDIAN       non-zero if architecture is known to be big-endian
// BEE_LITTLE_FAST      non-zero if little-endian decoding is fast
// BEE_BIG_FAST         non-zero if big-endian decoding is fast
//
// If BEE_UPTR is defined, then encoding and decoding of 32-bit and 64-bit
// values will try to be "smart". Either BEE_LITTLE_ENDIAN or BEE_BIG_ENDIAN
// _must_ be non-zero in those situations. The 32-bit and 64-bit types
// _must_ also have an exact width.
//
// BEE_SPARCV9_GCC_32   UltraSPARC-compatible with gcc, 32-bit mode
// BEE_SPARCV9_GCC_64   UltraSPARC-compatible with gcc, 64-bit mode
// BEE_SPARCV9_GCC      UltraSPARC-compatible with gcc
// BEE_I386_GCC         x86-compatible (32-bit) with gcc
// BEE_I386_MSVC        x86-compatible (32-bit) with Microsoft Visual C
// BEE_AMD64_GCC        x86-compatible (64-bit) with gcc
// BEE_AMD64_MSVC       x86-compatible (64-bit) with Microsoft Visual C
// BEE_PPC32_GCC        PowerPC, 32-bit, with gcc
// BEE_PPC64_GCC        PowerPC, 64-bit, with gcc
//
// TODO: enhance automatic detection, for more architectures and compilers.
// Endianness is the most important. BEE_UNALIGNED and BEE_UPTR help with
// some very fast functions (e.g. MD4) when using unaligned input data.
// The CPU-specific-with-GCC macros are useful only for inline assembly,
// normally restrained to this header file.
//

//
// 32-bit x86, aka "i386 compatible".
//
#if defined __i386__ || defined _M_IX86
	#define BEE_DETECT_UNALIGNED         1
	#define BEE_DETECT_LITTLE_ENDIAN     1
	#define BEE_DETECT_UPTR              bee_u32
	#ifdef __GNUC__
		#define BEE_DETECT_I386_GCC      1
	#endif
	#ifdef _MSC_VER
		#define BEE_DETECT_I386_MSVC     1
	#endif
//
// 64-bit x86, hereafter known as "amd64".
//
#elif defined __x86_64 || defined _M_X64
	#define BEE_DETECT_UNALIGNED         1
	#define BEE_DETECT_LITTLE_ENDIAN     1
	#define BEE_DETECT_UPTR              bee_u64
	#ifdef __GNUC__
		#define BEE_DETECT_AMD64_GCC         1
	#endif
	#ifdef _MSC_VER
		#define BEE_DETECT_AMD64_MSVC        1
	#endif
//
// 64-bit Sparc architecture (implies v9).
//
#elif ((defined __sparc__ || defined __sparc) && defined __arch64__) \
	|| defined __sparcv9
	#define BEE_DETECT_BIG_ENDIAN        1
	#define BEE_DETECT_UPTR              bee_u64
	#ifdef __GNUC__
		#define BEE_DETECT_SPARCV9_GCC_64    1
		#define BEE_DETECT_LITTLE_FAST       1
	#endif
//
// 32-bit Sparc.
//
#elif (defined __sparc__ || defined __sparc) \
	&& !(defined __sparcv9 || defined __arch64__)
	#define BEE_DETECT_BIG_ENDIAN        1
	#define BEE_DETECT_UPTR              bee_u32
	#if defined __GNUC__ && defined __sparc_v9__
		#define BEE_DETECT_SPARCV9_GCC_32    1
		#define BEE_DETECT_LITTLE_FAST       1
	#endif
//
// ARM, little-endian.
///
#elif defined __arm__ && __ARMEL__
	#define BEE_DETECT_LITTLE_ENDIAN     1
//
// MIPS, little-endian.
//
#elif MIPSEL || _MIPSEL || __MIPSEL || __MIPSEL__
	#define BEE_DETECT_LITTLE_ENDIAN     1
//
// MIPS, big-endian.
//
#elif MIPSEB || _MIPSEB || __MIPSEB || __MIPSEB__
	#define BEE_DETECT_BIG_ENDIAN        1
//
// PowerPC.
//
#elif defined __powerpc__ || defined __POWERPC__ || defined __ppc__ \
	|| defined _ARCH_PPC

	//
	// Note: we do not declare cross-endian access to be "fast": even if
	// using inline assembly, implementation should still assume that
	// keeping the decoded word in a temporary is faster than decoding
	// it again.
	///
	#if defined __GNUC__
		#if BEE_64_TRUE
			#define BEE_DETECT_PPC64_GCC         1
		#else
			#define BEE_DETECT_PPC32_GCC         1
		#endif
	#endif

	#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
		#define BEE_DETECT_BIG_ENDIAN        1
	#elif defined __LITTLE_ENDIAN__ || defined _LITTLE_ENDIAN
		#define BEE_DETECT_LITTLE_ENDIAN     1
	#endif
//
// Itanium, 64-bit.
///
#elif defined __ia64 || defined __ia64__ \
	|| defined __itanium__ || defined _M_IA64

	#if defined __BIG_ENDIAN__ || defined _BIG_ENDIAN
		#define BEE_DETECT_BIG_ENDIAN        1
	#else
		#define BEE_DETECT_LITTLE_ENDIAN     1
	#endif
	#if defined __LP64__ || defined _LP64
		#define BEE_DETECT_UPTR              bee_u64
	#else
		#define BEE_DETECT_UPTR              bee_u32
	#endif
#endif

#if defined BEE_DETECT_SPARCV9_GCC_32 || defined BEE_DETECT_SPARCV9_GCC_64
	#define BEE_DETECT_SPARCV9_GCC       1
#endif

#if defined BEE_DETECT_UNALIGNED && !defined BEE_UNALIGNED
	#define BEE_UNALIGNED         BEE_DETECT_UNALIGNED
#endif
#if defined BEE_DETECT_UPTR && !defined BEE_UPTR
	#define BEE_UPTR              BEE_DETECT_UPTR
#endif
#if defined BEE_DETECT_LITTLE_ENDIAN && !defined BEE_LITTLE_ENDIAN
	#define BEE_LITTLE_ENDIAN     BEE_DETECT_LITTLE_ENDIAN
#endif
#if defined BEE_DETECT_BIG_ENDIAN && !defined BEE_BIG_ENDIAN
	#define BEE_BIG_ENDIAN        BEE_DETECT_BIG_ENDIAN
#endif
#if defined BEE_DETECT_LITTLE_FAST && !defined BEE_LITTLE_FAST
	#define BEE_LITTLE_FAST       BEE_DETECT_LITTLE_FAST
#endif
#if defined BEE_DETECT_BIG_FAST && !defined BEE_BIG_FAST
	#define BEE_BIG_FAST    BEE_DETECT_BIG_FAST
#endif
#if defined BEE_DETECT_SPARCV9_GCC_32 && !defined BEE_SPARCV9_GCC_32
	#define BEE_SPARCV9_GCC_32    BEE_DETECT_SPARCV9_GCC_32
#endif
#if defined BEE_DETECT_SPARCV9_GCC_64 && !defined BEE_SPARCV9_GCC_64
	#define BEE_SPARCV9_GCC_64    BEE_DETECT_SPARCV9_GCC_64
#endif
#if defined BEE_DETECT_SPARCV9_GCC && !defined BEE_SPARCV9_GCC
	#define BEE_SPARCV9_GCC       BEE_DETECT_SPARCV9_GCC
#endif
#if defined BEE_DETECT_I386_GCC && !defined BEE_I386_GCC
	#define BEE_I386_GCC          BEE_DETECT_I386_GCC
#endif
#if defined BEE_DETECT_I386_MSVC && !defined BEE_I386_MSVC
	#define BEE_I386_MSVC         BEE_DETECT_I386_MSVC
#endif
#if defined BEE_DETECT_AMD64_GCC && !defined BEE_AMD64_GCC
	#define BEE_AMD64_GCC         BEE_DETECT_AMD64_GCC
#endif
#if defined BEE_DETECT_AMD64_MSVC && !defined BEE_AMD64_MSVC
	#define BEE_AMD64_MSVC        BEE_DETECT_AMD64_MSVC
#endif
#if defined BEE_DETECT_PPC32_GCC && !defined BEE_PPC32_GCC
	#define BEE_PPC32_GCC         BEE_DETECT_PPC32_GCC
#endif
#if defined BEE_DETECT_PPC64_GCC && !defined BEE_PPC64_GCC
	#define BEE_PPC64_GCC         BEE_DETECT_PPC64_GCC
#endif

#if BEE_LITTLE_ENDIAN && !defined BEE_LITTLE_FAST
	#define BEE_LITTLE_FAST              1
#endif
#if BEE_BIG_ENDIAN && !defined BEE_BIG_FAST
	#define BEE_BIG_FAST                 1
#endif

#if defined BEE_UPTR && !(BEE_LITTLE_ENDIAN || BEE_BIG_ENDIAN)
	#error BEE_UPTR defined, but endianness is not known.
#endif


#if BEE_I386_GCC && !BEE_NO_ASM
	//
	// On x86 32-bit, with gcc, we use the bswapl opcode to byte-swap 32-bit
	// values.
	//
	static BEE_INLINE bee_u32 bee_bswap32(bee_u32 x)
	{
		__asm__ __volatile__ ("bswapl %0" : "=r" (x) : "0" (x));
		return x;
	}

	#if BEE_64
		static BEE_INLINE bee_u64 bee_bswap64(bee_u64 x)
		{
			return ((bee_u64)bee_bswap32((bee_u32)x) << 32)
				| (bee_u64)bee_bswap32((bee_u32)(x >> 32));
		}
	#endif

#elif BEE_AMD64_GCC && !BEE_NO_ASM
	//
	// On x86 64-bit, with gcc, we use the bswapl opcode to byte-swap 32-bit
	// and 64-bit values.
	//
	static BEE_INLINE bee_u32 bee_bswap32(bee_u32 x)
	{
		__asm__ __volatile__ ("bswapl %0" : "=r" (x) : "0" (x));
		return x;
	}

	#if BEE_64
		static BEE_INLINE bee_u64 bee_bswap64(bee_u64 x)
		{
			__asm__ __volatile__ ("bswapq %0" : "=r" (x) : "0" (x));
			return x;
		}
	#endif

	//
	// Disabled code. Apparently, Microsoft Visual C 2005 is smart enough
	// to generate proper opcodes for endianness swapping with the pure C
	// implementation below.
	//
	//
	//#elif BEE_I386_MSVC && !BEE_NO_ASM
	//
	//static __inline bee_u32 __declspec(naked) __fastcall
	//bee_bswap32(bee_u32 x)
	//{
	//	__asm {
	//		bswap  ecx
	//		mov    eax,ecx
	//		ret
	//	}
	//}
	//
	//#if BEE_64
	//
	//static BEE_INLINE bee_u64
	//bee_bswap64(bee_u64 x)
	//{
	//	return ((bee_u64)bee_bswap32((bee_u32)x) << 32)
	//		| (bee_u64)bee_bswap32((bee_u32)(x >> 32));
	//}
	//
	//#endif
	//
	//
	// [end of disabled code]
	//
#else
	static BEE_INLINE bee_u32 bee_bswap32(bee_u32 x)
	{
		x = BEE_T32((x << 16) | (x >> 16));
		x = ((x & BEE_C32(0xFF00FF00)) >> 8)
			| ((x & BEE_C32(0x00FF00FF)) << 8);
		return x;
	}

	#if BEE_64
		//
		// Byte-swap a 64-bit value.
		//
		// @param x   the input value
		// @return  the byte-swapped value
		///
		static BEE_INLINE bee_u64 bee_bswap64(bee_u64 x)
		{
			x = BEE_T64((x << 32) | (x >> 32));
			x = ((x & BEE_C64(0xFFFF0000FFFF0000)) >> 16)
				| ((x & BEE_C64(0x0000FFFF0000FFFF)) << 16);
			x = ((x & BEE_C64(0xFF00FF00FF00FF00)) >> 8)
				| ((x & BEE_C64(0x00FF00FF00FF00FF)) << 8);
			return x;
		}
	#endif
#endif

#if BEE_SPARCV9_GCC && !BEE_NO_ASM
	//
	// On UltraSPARC systems, native ordering is big-endian, but it is
	// possible to perform little-endian read accesses by specifying the
	// address space 0x88 (ASI_PRIMARY_LITTLE). Basically, either we use
	// the opcode "lda [%reg]0x88,%dst", where %reg is the register which
	// contains the source address and %dst is the destination register,
	// or we use "lda [%reg+imm]%asi,%dst", which uses the %asi register
	// to get the address space name. The latter format is better since it
	// combines an addition and the actual access in a single opcode; but
	// it requires the setting (and subsequent resetting) of %asi, which is
	// slow. Some operations (i.e. MD5 compression function) combine many
	// successive little-endian read accesses, which may share the same
	// %asi setting. The macros below contain the appropriate inline
	// assembly.
	//

	#define BEE_SPARCV9_SET_ASI   \
		bee_u32 bee_sparcv9_asi; \
		__asm__ __volatile__ ( \
			"rd %%asi,%0\n\twr %%g0,0x88,%%asi" : "=r" (bee_sparcv9_asi));

	#define BEE_SPARCV9_RESET_ASI  \
		__asm__ __volatile__ ("wr %%g0,%0,%%asi" : : "r" (bee_sparcv9_asi));

	#define BEE_SPARCV9_DEC32LE(base, idx)   ({ \
			bee_u32 bee_sparcv9_tmp; \
			__asm__ __volatile__ ("lda [%1+" #idx "*4]%%asi,%0" \
				: "=r" (bee_sparcv9_tmp) : "r" (base)); \
			bee_sparcv9_tmp; \
		})
#endif

//-----------------------------------------------------------------------------------------
//--.
static BEE_INLINE void bee_enc16be(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = (val >> 8);
	((unsigned char *)dst)[1] = val;
}

//-----------------------------------------------------------------------------------------
//--.
static BEE_INLINE unsigned bee_dec16be(const void *src)
{
	return ((unsigned)(((const unsigned char *)src)[0]) << 8)
		| (unsigned)(((const unsigned char *)src)[1]);
}

//-----------------------------------------------------------------------------------------
//--.
static BEE_INLINE void bee_enc16le(void *dst, unsigned val)
{
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = val >> 8;
}

//-----------------------------------------------------------------------------------------
//--.
static BEE_INLINE unsigned bee_dec16le(const void *src)
{
	return (unsigned)(((const unsigned char *)src)[0])
		| ((unsigned)(((const unsigned char *)src)[1]) << 8);
}

//-----------------------------------------------------------------------------------------
//--.
///
// Encode a 32-bit value into the provided buffer (big endian convention).
//
// @param dst   the destination buffer
// @param val   the 32-bit value to encode
//
static BEE_INLINE void bee_enc32be(void *dst, bee_u32 val)
{
	#if defined BEE_UPTR
	#if BEE_UNALIGNED
	#if BEE_LITTLE_ENDIAN
		val = bee_bswap32(val);
	#endif
		*(bee_u32 *)dst = val;
	#else
		if (((BEE_UPTR)dst & 3) == 0) {
	#if BEE_LITTLE_ENDIAN
			val = bee_bswap32(val);
	#endif
			*(bee_u32 *)dst = val;
		} else {
			((unsigned char *)dst)[0] = (val >> 24);
			((unsigned char *)dst)[1] = (val >> 16);
			((unsigned char *)dst)[2] = (val >> 8);
			((unsigned char *)dst)[3] = val;
		}
	#endif
	#else
		((unsigned char *)dst)[0] = (val >> 24);
		((unsigned char *)dst)[1] = (val >> 16);
		((unsigned char *)dst)[2] = (val >> 8);
		((unsigned char *)dst)[3] = val;
	#endif
}

//-----------------------------------------------------------------------------------------
//--
//
// Encode a 32-bit value into the provided buffer (big endian convention).
// The destination buffer must be properly aligned.
//
// @param dst   the destination buffer (32-bit aligned)
// @param val   the value to encode
//
static BEE_INLINE void bee_enc32be_aligned(void *dst, bee_u32 val)
{
	#if BEE_LITTLE_ENDIAN
		*(bee_u32 *)dst = bee_bswap32(val);
	#elif BEE_BIG_ENDIAN
		*(bee_u32 *)dst = val;
	#else
		((unsigned char *)dst)[0] = (val >> 24);
		((unsigned char *)dst)[1] = (val >> 16);
		((unsigned char *)dst)[2] = (val >> 8);
		((unsigned char *)dst)[3] = val;
	#endif
}

//-----------------------------------------------------------------------------------------
//--.
//
// Decode a 32-bit value from the provided buffer (big endian convention).
//
// @param src   the source buffer
// @return  the decoded value
//
static BEE_INLINE bee_u32 bee_dec32be( const void *src )
{
	#if defined BEE_UPTR
	#if BEE_UNALIGNED
	#if BEE_LITTLE_ENDIAN
		return bee_bswap32(*(const bee_u32 *)src);
	#else
		return *(const bee_u32 *)src;
	#endif
	#else
		if (((BEE_UPTR)src & 3) == 0) {
	#if BEE_LITTLE_ENDIAN
			return bee_bswap32(*(const bee_u32 *)src);
	#else
			return *(const bee_u32 *)src;
	#endif
		} else {
			return ((bee_u32)(((const unsigned char *)src)[0]) << 24)
				| ((bee_u32)(((const unsigned char *)src)[1]) << 16)
				| ((bee_u32)(((const unsigned char *)src)[2]) << 8)
				| (bee_u32)(((const unsigned char *)src)[3]);
		}
	#endif
	#else
		return ((bee_u32)(((const unsigned char *)src)[0]) << 24)
			| ((bee_u32)(((const unsigned char *)src)[1]) << 16)
			| ((bee_u32)(((const unsigned char *)src)[2]) << 8)
			| (bee_u32)(((const unsigned char *)src)[3]);
	#endif
}

//-----------------------------------------------------------------------------------------
//--.
//
// Decode a 32-bit value from the provided buffer (big endian convention).
// The source buffer must be properly aligned.
//
// @param src   the source buffer (32-bit aligned)
// @return  the decoded value
//
static BEE_INLINE bee_u32 bee_dec32be_aligned(const void *src)
{
	#if BEE_LITTLE_ENDIAN
		return bee_bswap32(*(const bee_u32 *)src);
	#elif BEE_BIG_ENDIAN
		return *(const bee_u32 *)src;
	#else
		return ((bee_u32)(((const unsigned char *)src)[0]) << 24)
			| ((bee_u32)(((const unsigned char *)src)[1]) << 16)
			| ((bee_u32)(((const unsigned char *)src)[2]) << 8)
			| (bee_u32)(((const unsigned char *)src)[3]);
	#endif
}

//-----------------------------------------------------------------------------------------
//--.
//
// Encode a 32-bit value into the provided buffer (little endian convention).
//
// @param dst   the destination buffer
// @param val   the 32-bit value to encode
//
static BEE_INLINE void bee_enc32le(void *dst, bee_u32 val)
{
	#if defined BEE_UPTR
		#if BEE_UNALIGNED
			#if BEE_BIG_ENDIAN
				val = bee_bswap32(val);
			#endif
			*(bee_u32 *)dst = val;
		#else
			if (((BEE_UPTR)dst & 3) == 0) {
			#if BEE_BIG_ENDIAN
					val = bee_bswap32(val);
			#endif
				*(bee_u32 *)dst = val;
			} else {
				((unsigned char *)dst)[0] = val;
				((unsigned char *)dst)[1] = (val >> 8);
				((unsigned char *)dst)[2] = (val >> 16);
				((unsigned char *)dst)[3] = (val >> 24);
			}
		#endif
	#else
		((unsigned char *)dst)[0] = val;
		((unsigned char *)dst)[1] = (val >> 8);
		((unsigned char *)dst)[2] = (val >> 16);
		((unsigned char *)dst)[3] = (val >> 24);
	#endif
}

//-----------------------------------------------------------------------------------------
//--.
//
// Encode a 32-bit value into the provided buffer (little endian convention).
// The destination buffer must be properly aligned.
//
// @param dst   the destination buffer (32-bit aligned)
// @param val   the value to encode
//
static BEE_INLINE void bee_enc32le_aligned(void *dst, bee_u32 val)
{
#if BEE_LITTLE_ENDIAN
	*(bee_u32 *)dst = val;
#elif BEE_BIG_ENDIAN
	*(bee_u32 *)dst = bee_bswap32(val);
#else
	((unsigned char *)dst)[0] = val;
	((unsigned char *)dst)[1] = (val >> 8);
	((unsigned char *)dst)[2] = (val >> 16);
	((unsigned char *)dst)[3] = (val >> 24);
#endif
}

//-----------------------------------------------------------------------------------------
//--.
//
// Decode a 32-bit value from the provided buffer (little endian convention).
//
// @param src   the source buffer
// @return  the decoded value
//
static BEE_INLINE bee_u32 bee_dec32le(const void *src)
{
#if defined BEE_UPTR
#if BEE_UNALIGNED
#if BEE_BIG_ENDIAN
	return bee_bswap32(*(const bee_u32 *)src);
#else
	return *(const bee_u32 *)src;
#endif
#else
	if (((BEE_UPTR)src & 3) == 0) {
#if BEE_BIG_ENDIAN
#if BEE_SPARCV9_GCC && !BEE_NO_ASM
		bee_u32 tmp;

		//
		// "__volatile__" is needed here because without it,
		// gcc-3.4.3 miscompiles the code and performs the
		// access before the test on the address, thus triggering
		// a bus error...
		//
		__asm__ __volatile__ (
			"lda [%1]0x88,%0" : "=r" (tmp) : "r" (src));
		return tmp;
//
// On PowerPC, this turns out not to be worth the effort: the inline
// assembly makes GCC optimizer uncomfortable, which tends to nullify
// the decoding gains.
//
// For most hash functions, using this inline assembly trick changes
// hashing speed by less than 5% and often _reduces_ it. The biggest
// gains are for MD4 (+11%) and CubeHash (+30%). For all others, it is
// less then 10%. The speed gain on CubeHash is probably due to the
// chronic shortage of registers that CubeHash endures; for the other
// functions, the generic code appears to be efficient enough already.
//
//#elif (BEE_PPC32_GCC || BEE_PPC64_GCC) && !BEE_NO_ASM
//		bee_u32 tmp;
//
//		__asm__ __volatile__ (
//			"lwbrx %0,0,%1" : "=r" (tmp) : "r" (src));
//		return tmp;
//
#else
		return bee_bswap32(*(const bee_u32 *)src);
#endif
#else
		return *(const bee_u32 *)src;
#endif
	} else {
		return (bee_u32)(((const unsigned char *)src)[0])
			| ((bee_u32)(((const unsigned char *)src)[1]) << 8)
			| ((bee_u32)(((const unsigned char *)src)[2]) << 16)
			| ((bee_u32)(((const unsigned char *)src)[3]) << 24);
	}
#endif
#else
	return (bee_u32)(((const unsigned char *)src)[0])
		| ((bee_u32)(((const unsigned char *)src)[1]) << 8)
		| ((bee_u32)(((const unsigned char *)src)[2]) << 16)
		| ((bee_u32)(((const unsigned char *)src)[3]) << 24);
#endif
}

//-----------------------------------------------------------------------------------------
//--.
//
// Decode a 32-bit value from the provided buffer (little endian convention).
// The source buffer must be properly aligned.
//
// @param src   the source buffer (32-bit aligned)
// @return  the decoded value
//
static BEE_INLINE bee_u32 bee_dec32le_aligned(const void *src)
{
#if BEE_LITTLE_ENDIAN
	return *(const bee_u32 *)src;
#elif BEE_BIG_ENDIAN
#if BEE_SPARCV9_GCC && !BEE_NO_ASM
	bee_u32 tmp;

	__asm__ __volatile__ ("lda [%1]0x88,%0" : "=r" (tmp) : "r" (src));
	return tmp;
//
// Not worth it generally.
//
//#elif (BEE_PPC32_GCC || BEE_PPC64_GCC) && !BEE_NO_ASM
//	bee_u32 tmp;
//
//	__asm__ __volatile__ ("lwbrx %0,0,%1" : "=r" (tmp) : "r" (src));
//	return tmp;
///
#else
	return bee_bswap32(*(const bee_u32 *)src);
#endif
#else
	return (bee_u32)(((const unsigned char *)src)[0])
		| ((bee_u32)(((const unsigned char *)src)[1]) << 8)
		| ((bee_u32)(((const unsigned char *)src)[2]) << 16)
		| ((bee_u32)(((const unsigned char *)src)[3]) << 24);
#endif
}

#if BEE_64
	//-----------------------------------------------------------------------------------------
	//--.
	//
	//  Encode a 64-bit value into the provided buffer (big endian convention).
	//
	// @param dst   the destination buffer
	// @param val   the 64-bit value to encode
	//
	static BEE_INLINE void
	bee_enc64be(void *dst, bee_u64 val)
	{
	#if defined BEE_UPTR
	#if BEE_UNALIGNED
	#if BEE_LITTLE_ENDIAN
		val = bee_bswap64(val);
	#endif
		*(bee_u64 *)dst = val;
	#else
		if (((BEE_UPTR)dst & 7) == 0) {
	#if BEE_LITTLE_ENDIAN
			val = bee_bswap64(val);
	#endif
			*(bee_u64 *)dst = val;
		} else {
			((unsigned char *)dst)[0] = (val >> 56);
			((unsigned char *)dst)[1] = (val >> 48);
			((unsigned char *)dst)[2] = (val >> 40);
			((unsigned char *)dst)[3] = (val >> 32);
			((unsigned char *)dst)[4] = (val >> 24);
			((unsigned char *)dst)[5] = (val >> 16);
			((unsigned char *)dst)[6] = (val >> 8);
			((unsigned char *)dst)[7] = val;
		}
	#endif
	#else
		((unsigned char *)dst)[0] = (val >> 56);
		((unsigned char *)dst)[1] = (val >> 48);
		((unsigned char *)dst)[2] = (val >> 40);
		((unsigned char *)dst)[3] = (val >> 32);
		((unsigned char *)dst)[4] = (val >> 24);
		((unsigned char *)dst)[5] = (val >> 16);
		((unsigned char *)dst)[6] = (val >> 8);
		((unsigned char *)dst)[7] = val;
	#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Encode a 64-bit value into the provided buffer (big endian convention).
	// The destination buffer must be properly aligned.
	//
	// @param dst   the destination buffer (64-bit aligned)
	// @param val   the value to encode
	//
	static BEE_INLINE void bee_enc64be_aligned(void *dst, bee_u64 val)
	{
	#if BEE_LITTLE_ENDIAN
		*(bee_u64 *)dst = bee_bswap64(val);
	#elif BEE_BIG_ENDIAN
		*(bee_u64 *)dst = val;
	#else
		((unsigned char *)dst)[0] = (val >> 56);
		((unsigned char *)dst)[1] = (val >> 48);
		((unsigned char *)dst)[2] = (val >> 40);
		((unsigned char *)dst)[3] = (val >> 32);
		((unsigned char *)dst)[4] = (val >> 24);
		((unsigned char *)dst)[5] = (val >> 16);
		((unsigned char *)dst)[6] = (val >> 8);
		((unsigned char *)dst)[7] = val;
	#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Decode a 64-bit value from the provided buffer (big endian convention).
	//
	// @param src   the source buffer
	// @return  the decoded value
	//
	static BEE_INLINE bee_u64 bee_dec64be(const void *src)
	{
	#if defined BEE_UPTR
	#if BEE_UNALIGNED
	#if BEE_LITTLE_ENDIAN
		return bee_bswap64(*(const bee_u64 *)src);
	#else
		return *(const bee_u64 *)src;
	#endif
	#else
		if (((BEE_UPTR)src & 7) == 0) {
	#if BEE_LITTLE_ENDIAN
			return bee_bswap64(*(const bee_u64 *)src);
	#else
			return *(const bee_u64 *)src;
	#endif
		} else {
			return ((bee_u64)(((const unsigned char *)src)[0]) << 56)
				| ((bee_u64)(((const unsigned char *)src)[1]) << 48)
				| ((bee_u64)(((const unsigned char *)src)[2]) << 40)
				| ((bee_u64)(((const unsigned char *)src)[3]) << 32)
				| ((bee_u64)(((const unsigned char *)src)[4]) << 24)
				| ((bee_u64)(((const unsigned char *)src)[5]) << 16)
				| ((bee_u64)(((const unsigned char *)src)[6]) << 8)
				| (bee_u64)(((const unsigned char *)src)[7]);
		}
	#endif
	#else
		return ((bee_u64)(((const unsigned char *)src)[0]) << 56)
			| ((bee_u64)(((const unsigned char *)src)[1]) << 48)
			| ((bee_u64)(((const unsigned char *)src)[2]) << 40)
			| ((bee_u64)(((const unsigned char *)src)[3]) << 32)
			| ((bee_u64)(((const unsigned char *)src)[4]) << 24)
			| ((bee_u64)(((const unsigned char *)src)[5]) << 16)
			| ((bee_u64)(((const unsigned char *)src)[6]) << 8)
			| (bee_u64)(((const unsigned char *)src)[7]);
	#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Decode a 64-bit value from the provided buffer (big endian convention).
	// The source buffer must be properly aligned.
	//
	// @param src   the source buffer (64-bit aligned)
	// @return  the decoded value
	//
	static BEE_INLINE bee_u64 bee_dec64be_aligned(const void *src)
	{
	#if BEE_LITTLE_ENDIAN
		return bee_bswap64(*(const bee_u64 *)src);
	#elif BEE_BIG_ENDIAN
		return *(const bee_u64 *)src;
	#else
		return ((bee_u64)(((const unsigned char *)src)[0]) << 56)
			| ((bee_u64)(((const unsigned char *)src)[1]) << 48)
			| ((bee_u64)(((const unsigned char *)src)[2]) << 40)
			| ((bee_u64)(((const unsigned char *)src)[3]) << 32)
			| ((bee_u64)(((const unsigned char *)src)[4]) << 24)
			| ((bee_u64)(((const unsigned char *)src)[5]) << 16)
			| ((bee_u64)(((const unsigned char *)src)[6]) << 8)
			| (bee_u64)(((const unsigned char *)src)[7]);
	#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Encode a 64-bit value into the provided buffer (little endian convention).
	//
	// @param dst   the destination buffer
	// @param val   the 64-bit value to encode
	//
	static BEE_INLINE void bee_enc64le(void *dst, bee_u64 val)
	{
		#if defined BEE_UPTR
		#if BEE_UNALIGNED
		#if BEE_BIG_ENDIAN
			val = bee_bswap64(val);
		#endif
			*(bee_u64 *)dst = val;
		#else
			if (((BEE_UPTR)dst & 7) == 0) {
		#if BEE_BIG_ENDIAN
				val = bee_bswap64(val);
		#endif
				*(bee_u64 *)dst = val;
			} else {
				((unsigned char *)dst)[0] = val;
				((unsigned char *)dst)[1] = (val >> 8);
				((unsigned char *)dst)[2] = (val >> 16);
				((unsigned char *)dst)[3] = (val >> 24);
				((unsigned char *)dst)[4] = (val >> 32);
				((unsigned char *)dst)[5] = (val >> 40);
				((unsigned char *)dst)[6] = (val >> 48);
				((unsigned char *)dst)[7] = (val >> 56);
			}
		#endif
		#else
			((unsigned char *)dst)[0] = val;
			((unsigned char *)dst)[1] = (val >> 8);
			((unsigned char *)dst)[2] = (val >> 16);
			((unsigned char *)dst)[3] = (val >> 24);
			((unsigned char *)dst)[4] = (val >> 32);
			((unsigned char *)dst)[5] = (val >> 40);
			((unsigned char *)dst)[6] = (val >> 48);
			((unsigned char *)dst)[7] = (val >> 56);
		#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Encode a 64-bit value into the provided buffer (little endian convention).
	// The destination buffer must be properly aligned.
	//
	// @param dst   the destination buffer (64-bit aligned)
	// @param val   the value to encode
	//
	static BEE_INLINE void bee_enc64le_aligned(void *dst, bee_u64 val)
	{
		#if BEE_LITTLE_ENDIAN
			*(bee_u64 *)dst = val;
		#elif BEE_BIG_ENDIAN
			*(bee_u64 *)dst = bee_bswap64(val);
		#else
			((unsigned char *)dst)[0] = val;
			((unsigned char *)dst)[1] = (val >> 8);
			((unsigned char *)dst)[2] = (val >> 16);
			((unsigned char *)dst)[3] = (val >> 24);
			((unsigned char *)dst)[4] = (val >> 32);
			((unsigned char *)dst)[5] = (val >> 40);
			((unsigned char *)dst)[6] = (val >> 48);
			((unsigned char *)dst)[7] = (val >> 56);
		#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Decode a 64-bit value from the provided buffer (little endian convention).
	//
	// @param src   the source buffer
	// @return  the decoded value
	//
	static BEE_INLINE bee_u64 bee_dec64le(const void *src)
	{
		#if defined BEE_UPTR
			#if BEE_UNALIGNED
				#if BEE_BIG_ENDIAN
					return bee_bswap64(*(const bee_u64 *)src);
				#else
					return *(const bee_u64 *)src;
				#endif
			#else
				if (((BEE_UPTR)src & 7) == 0) {
					#if BEE_BIG_ENDIAN
						#if BEE_SPARCV9_GCC_64 && !BEE_NO_ASM
								bee_u64 tmp;

								__asm__ __volatile__ (
									"ldxa [%1]0x88,%0" : "=r" (tmp) : "r" (src));
								return tmp;
						//
						// Not worth it generally.
						//
						//#elif BEE_PPC32_GCC && !BEE_NO_ASM
						//		return (bee_u64)bee_dec32le_aligned(src)
						//			| ((bee_u64)bee_dec32le_aligned(
						//				(const char *)src + 4) << 32);
						//#elif BEE_PPC64_GCC && !BEE_NO_ASM
						//		bee_u64 tmp;
						//
						//		__asm__ __volatile__ (
						//			"ldbrx %0,0,%1" : "=r" (tmp) : "r" (src));
						//		return tmp;
						//
						#else
								return bee_bswap64(*(const bee_u64 *)src);
						#endif
					#else
							return *(const bee_u64 *)src;
					#endif
				} else {
					return (bee_u64)(((const unsigned char *)src)[0])
						| ((bee_u64)(((const unsigned char *)src)[1]) << 8)
						| ((bee_u64)(((const unsigned char *)src)[2]) << 16)
						| ((bee_u64)(((const unsigned char *)src)[3]) << 24)
						| ((bee_u64)(((const unsigned char *)src)[4]) << 32)
						| ((bee_u64)(((const unsigned char *)src)[5]) << 40)
						| ((bee_u64)(((const unsigned char *)src)[6]) << 48)
						| ((bee_u64)(((const unsigned char *)src)[7]) << 56);
				}
			#endif
		#else
			return (bee_u64)(((const unsigned char *)src)[0])
				| ((bee_u64)(((const unsigned char *)src)[1]) << 8)
				| ((bee_u64)(((const unsigned char *)src)[2]) << 16)
				| ((bee_u64)(((const unsigned char *)src)[3]) << 24)
				| ((bee_u64)(((const unsigned char *)src)[4]) << 32)
				| ((bee_u64)(((const unsigned char *)src)[5]) << 40)
				| ((bee_u64)(((const unsigned char *)src)[6]) << 48)
				| ((bee_u64)(((const unsigned char *)src)[7]) << 56);
		#endif
	}

	//-----------------------------------------------------------------------------------------
	//--.
	//
	// Decode a 64-bit value from the provided buffer (little endian convention).
	// The source buffer must be properly aligned.
	//
	// @param src   the source buffer (64-bit aligned)
	// @return  the decoded value
	//
	static BEE_INLINE bee_u64 bee_dec64le_aligned(const void *src)
	{	
		#if BEE_LITTLE_ENDIAN
			return *(const bee_u64 *)src;
		#elif BEE_BIG_ENDIAN	
			#if BEE_SPARCV9_GCC_64 && !BEE_NO_ASM
				bee_u64 tmp;

				__asm__ __volatile__ ("ldxa [%1]0x88,%0" : "=r" (tmp) : "r" (src));
				return tmp;
				//
				// Not worth it generally.
				//
				//#elif BEE_PPC32_GCC && !BEE_NO_ASM
				//	return (bee_u64)bee_dec32le_aligned(src)
				//		| ((bee_u64)bee_dec32le_aligned((const char *)src + 4) << 32);
				//#elif BEE_PPC64_GCC && !BEE_NO_ASM
				//	bee_u64 tmp;
				//
				//	__asm__ __volatile__ ("ldbrx %0,0,%1" : "=r" (tmp) : "r" (src));
				//	return tmp;
				///
			#else
				return bee_bswap64(*(const bee_u64 *)src);
			#endif
		#else
			return (bee_u64)(((const unsigned char *)src)[0])
				| ((bee_u64)(((const unsigned char *)src)[1]) << 8)
				| ((bee_u64)(((const unsigned char *)src)[2]) << 16)
				| ((bee_u64)(((const unsigned char *)src)[3]) << 24)
				| ((bee_u64)(((const unsigned char *)src)[4]) << 32)
				| ((bee_u64)(((const unsigned char *)src)[5]) << 40)
				| ((bee_u64)(((const unsigned char *)src)[6]) << 48)
				| ((bee_u64)(((const unsigned char *)src)[7]) << 56);
		#endif
	}

#endif


#endif
