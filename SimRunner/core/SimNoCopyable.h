#pragma once

namespace ss{
    class NoCopyable{
        public:
            NoCopyable();
            virtual ~NoCopyable();

            NoCopyable(const NoCopyable &)=delete;
            NoCopyable & operator=(const NoCopyable &)=delete;
    };
}