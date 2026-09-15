#include <iostream>
#include "../include/thread_safe_queue.h"
#include "../include/two_lock_queue.h"

template<typename T>
concept movable = std::movable<T>;

int main()
{
    std::cout << "Hello world!!!";
    return 0;
}