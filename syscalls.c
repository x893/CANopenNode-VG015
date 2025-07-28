#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t *__sbrk_heap_end = NULL;

/**
 * @brief _sbrk() allocates memory to the newlib heap and is used by malloc
 *        and others from the C library
 *
 * @verbatim
 * #############################################################################
 * #  .data  #  .bss  #          heap         #          stack                 #
 * #         #        # Reserved by HEAP_SIZE # Reserved by STACK_SIZE         #
 * #############################################################################
 * ^-- RAM start      ^-- _heap_start         ^-- _heap_end _estack, RAM end --^
 * @endverbatim
 *
 * This implementation starts allocating at the '_heap_start' linker symbol
 * The 'HEAP_SIZE' linker symbol reserves a memory for the MSP stack
 * The implementation considers '_estack' linker symbol to be RAM end
 * NOTE: If the stack, at any point during execution, grows larger than the
 * reserved size, please increase the 'STACK_SIZE'.
 *
 * @param incr Memory size
 * @return Pointer to allocated memory
 */
void* _sbrk(ptrdiff_t incr)
{
	extern uint8_t _heap_start; /* Symbol defined in the linker script */
	extern uint8_t _heap_end;   /* Symbol defined in the linker script */
	register uint8_t *prev_heap_end;

	/* Initialize heap end at first call */
	if (__sbrk_heap_end == NULL)
		__sbrk_heap_end = &_heap_start;

	/* Protect heap from growing into the reserved MSP stack */
	if (__sbrk_heap_end + incr > &_heap_end)
	{
		errno = ENOMEM;
		return (void *)-1;
	}

	prev_heap_end = __sbrk_heap_end;
	__sbrk_heap_end += incr;

	return (void *)prev_heap_end;
}
