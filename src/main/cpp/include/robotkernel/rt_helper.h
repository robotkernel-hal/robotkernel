//! robotkernel rt helpers
/*!
 * author: Jan Cremer <jan.cremer@dlr.de>
 */

/*
 * This file is part of robotkernel.
 *
 * robotkernel is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * robotkernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with robotkernel.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef ROBOTKERNEL_RT_HELPER_H
#define ROBOTKERNEL_RT_HELPER_H

#include <pthread.h>

namespace robotkernel {
#ifdef EMACS
}
#endif

void setPriority(int priority, int policy = SCHED_FIFO);
void setAffinityMask(int affinity_mask);

#ifdef EMACS
{
#endif
}

#endif //PROJECT_RT_HELPER_H

