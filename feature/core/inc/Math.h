#pragma once

#include <cstdint>

class Math
    {
        public:
            // Constructors and Destructor
            Math() = default;
            Math(Math&) = delete;
            Math(Math&&) = delete;
            auto operator=(Math&) -> Math = delete;
            auto operator=(Math&&) -> Math = delete;
            ~Math() = default;
        
            // Method
            auto add(std::int32_t num1, std::int32_t num2) -> std::int32_t;
            auto subtract(std::int32_t num1, std::int32_t numb2) -> std::int32_t;
        
        private:
            static constexpr std::uint32_t MAX_LENGHT_UINT32 = 1;

    };

