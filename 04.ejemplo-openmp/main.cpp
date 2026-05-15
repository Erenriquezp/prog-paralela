#include <iostream>
#include <fmt/core.h>
#include <omp.h>

int main() {
    //int threads_count = omp_get_num_threads();
    //int thread_id = omp_get_thread_num();

    //std::printf("Goodbye serial world, hello OpenMP!\n");
    //std::printf("I have %d thread(s) and my thread_id is %d\n", threads_count, thread_id);

    //return 0;
   
    /*#pragma omp parallel num_threads(4)
    {
        #pragma omp master
        {
            int threads_count = omp_get_num_threads();
            fmt::println("Hello serial word, goodbye OpenMP!");
            fmt::println("I have {} threads", threads_count);
        }
        int thread_id = omp_get_thread_num();

        fmt::println("My thread_id is {}", thread_id);
    }
    return 0;*/

    /*#pragma omp parallel 
    {
       int thread_id = omp_get_thread_num();

        std::string msg = "";
        #pragma omp paraller for
        {
            for (int i = 0; i < thread_id; i++)
            {
                msg = msg + "*"; 
            }
        }
        fmt::println("My thread_id is {}, msg{}", thread_id, msg);
       
    } */

    int num_elementos = 23;

    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 23; i++) {
        fmt::println("i: {}, thread_id: {}", i, omp_get_thread_num());
    }

    #pragma omp parallel num_threads(4)
    {
        int thread_id = omp_get_thread_num();
        int thread_num = omp_get_num_threads();

        int delta = std::ceil(num_elementos * 1.0 / thread_num);
        int start = thread_id * delta;
        int end = (thread_id+1) * delta;

        if (thread_id == thread_num-1) {
            end = num_elementos;
        }
        
        fmt::println("Thread_{}: start={}, end={}", thread_id, start, end);

    }
    
    #pragma omp parallel num_threads(4)
    {
        int thread_id = omp_get_thread_num();
        int thread_num = omp_get_num_threads();

        for (int i = thread_id; i < num_elementos; i+=4) {
            fmt::println("thread_id: {}, index={}", thread_id, i);
        }
    }
    
     #pragma omp parallel
     {
        while (true)
        {
            /* code */
        }
        
     }

    return 0;
}