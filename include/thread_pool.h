#pragma once

#include "./thread_guard.h"
#include "./two_lock_queue.h"

namespace custom
{
    template<typename Func, typename... Args>
    concept Function = requires(Func func, Args... args)
    {
        func(args...);
    };
    template<typename Func_ptr, typename Obj_ptr, typename... Args>
    concept Object_Func = requires(Func_ptr func, Obj_ptr obj, Args... args)
    {
        (obj->*func)(args...);
    };
}