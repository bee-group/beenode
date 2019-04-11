#ifndef FACET_ONE_H
#define FACET_ONE_H

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "honeycomb_types.h"


//#undef BEE_64		//

/**
 * This structure is a context for HoneyComb Facet #1 computations: it contains the
 * intermediate values and some data from the last entered block. Once a
 * HoneyComb Facet #1 computation has been performed, the context can be reused for another computation.
 *
 * The contents of this structure are private. A running HoneyComb Facet #1 computation
 * can be cloned by copying the context (e.g. with a simple memcpy() ).
 */
typedef struct {
	unsigned char buf[144];    /* first field, for alignment */
	size_t ptr, lim;
	union 
	{
#if BEE_64			//FACET_LEN_64
		bee_u64 wide[25];
#endif
		bee_u32 narrow[50];		
	} u;

} facet_one_context;

/**
 * Initialize a HoneyComb Facet #1 context. This process performs no memory allocation.
 *
 * @param cc   the HoneyComb Facet #1 context ( pointer to a facet_one_context )
 */
void facet_one_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the HoneyComb Facet #1 context
 * @param data   the input data
 * @param len    the input data length (in bytes)
 */
void facet_one(void *cc, const void *data, size_t len);

/**
 * Terminate the current HoneyComb Facet #1 computation and output the result into
 * the provided buffer. The destination buffer must be wide enough to
 * accomodate the result (64 bytes). The context is automatically reinitialized.
 *
 * @param cc    the HoneyComb Facet #1 context
 * @param dst   the destination buffer
 */
void facet_one_close(void *cc, void *dst);

/**
 * Add a few additional bits (0 to 7) to the current computation, then
 * terminate it and output the result in the provided buffer, which must
 * be wide enough to accomodate the result (64 bytes). If bit number i
 * in <code>ub</code> has value 2^i, then the extra bits are those
 * numbered 7 downto 8-n (this is the big-endian convention at the byte
 * level). The context is automatically reinitialized.
 *
 * @param cc    the HoneyComb Facet #1 context
 * @param ub    the extra bits
 * @param n     the number of extra bits (0 to 7)
 * @param dst   the destination buffer
 */
void facet_one_addbits_and_close(void *cc, unsigned ub, unsigned n, void *dst);

#ifdef __cplusplus
}
#endif

#endif
