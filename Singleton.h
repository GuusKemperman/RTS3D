#pragma once

namespace Framework
{
    template <typename T>
    class Singleton
    {
        friend T;
    public:
        static inline T& Inst()
        {
            static T inst{};
            return inst;
        }

    private:
        Singleton() = default; // If you declare a constructor, ALWAYS make it private!
        ~Singleton() = default;
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
    };
}