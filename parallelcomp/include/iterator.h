//
// Created by Finaris on 11/25/19.
//

#include <functional>

#ifndef METROCASTER_ITERATOR_H
#define METROCASTER_ITERATOR_H

void parallel_for(unsigned n_elements, const std::function<void(int start, int end)>& func, bool parallelize=true);

#endif // METROCASTER_ITERATOR_H
