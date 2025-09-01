#include "micro_timer.h"

// uint32_t mt_measure_overhead_us(unsigned iters)
// {
// 	if (iters < 3)
// 		iters = 3;
// 	MicroTimer mt;
// 	mt_init(&mt);
// 	uint32_t best = UINT32_MAX, mid = UINT32_MAX, worst = 0;

// 	for (unsigned i = 0; i < iters; i++)
// 	{
// 		mt_start(&mt);
// 		mt_e2e_stop(&mt);
// 		uint32_t us = mt_elapsed_us(&mt);

// 		if (us < best)
// 		{
// 			worst = mid;
// 			mid = best;
// 			best = us;
// 		}
// 		else if (us < mid)
// 		{
// 			worst = mid;
// 			mid = us;
// 		}
// 		else if (us > worst)
// 		{
// 			worst = us;
// 		}
// 	}
// 	if (mid == UINT32_MAX)
// 		return best;
// 	/* pick value closer to mid to avoid outliers */
// 	return ((best > mid ? best - mid : mid - best) < (worst > mid ? worst - mid : mid - worst)) ? best : worst;
// }
