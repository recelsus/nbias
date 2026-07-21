#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace nbias::test
{
    class check_failure : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    inline int failure_count{0};

    inline void run_case(std::string const& name, std::function<void()> const& body)
    {
        try {
            body();
            std::cout << "[PASS] " << name << '\n';
        }
        catch(check_failure const& failure) {
            std::cerr << "[FAIL] " << name << ": " << failure.what() << '\n';
            ++failure_count;
        }
        catch(std::exception const& problem) {
            std::cerr << "[FAIL] " << name << ": unexpected exception: " << problem.what() << '\n';
            ++failure_count;
        }
    }

    inline int report()
    {
        return failure_count == 0 ? 0 : 1;
    }
}  // namespace nbias::test

#define NBIAS_CHECK(expr) \
    do { \
        if(!(expr)) { \
            std::ostringstream nbias_check_message{}; \
            nbias_check_message << "CHECK failed: " #expr " (" << __FILE__ << ':' << __LINE__ << ')'; \
            throw ::nbias::test::check_failure(nbias_check_message.str()); \
        } \
    } while(false)
