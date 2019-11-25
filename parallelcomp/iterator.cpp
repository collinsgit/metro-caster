//
// Created by Finaris on 11/25/19.
//

#include <iterator.h>

#include <algorithm>
#include <functional>
#include <thread>
#include <vector>
#include <iostream>

void parallel_for(unsigned n_elements, const std::function<void(int start, int end)>& func) {
    // Find the number of threads and allocate batch sizes.
    unsigned n_threads_hint = std::thread::hardware_concurrency();
    unsigned n_threads = n_threads_hint == 0 ? 8 : (n_threads_hint);
    unsigned batch_size = n_elements / n_threads;
    unsigned batch_remainder = n_elements % n_threads;

    // Create threads for each batch of work.
    std::vector<std::thread> threads(n_threads);
    for (unsigned i = 0; i < n_threads; ++i) {
        int start = i * batch_size;
        threads[i] = std::thread(func, start, start+batch_size);
    }

    // Deform elements and launch threads, waiting for them to finish.
    int start = n_threads * batch_size;
    func(start, start+batch_remainder);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}
