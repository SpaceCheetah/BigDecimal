#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "BigDecimal.h"
#include <concepts>

namespace {
    template<std::integral T>
    void checkConstructorI() {
        INFO("Type: ",typeid(T).name());
        //These constructors should copy the value exactly
        CHECK(BigDecimal{std::numeric_limits<T>::max()}.toLongLong() == std::numeric_limits<T>::max());
        CHECK(BigDecimal{std::numeric_limits<T>::min()}.toLongLong() == std::numeric_limits<T>::min());
    }
    template<std::floating_point T>
    void checkConstructorF() {
        INFO("Type: ",typeid(T).name());
        //Floating point numbers are not copied exactly
        T min = static_cast<T>(BigDecimal{std::numeric_limits<T>::min()}.toLongDouble());
        CHECK(min > 0);
        CHECK(min < 0.000001);
        CHECK(BigDecimal{static_cast<T>(0.5)}.toString() == "0.5");
    }
}

TEST_SUITE("Constructors") {
    TEST_CASE("Numeric") {
        CHECK(BigDecimal{}.toLongLong() == 0);
        checkConstructorI<long long>();
        checkConstructorI<long>();
        checkConstructorI<int>();
        checkConstructorI<short>();
        checkConstructorI<char>();
        checkConstructorF<long double>();
        checkConstructorF<double>();
        checkConstructorF<float>();
    }
    TEST_CASE("String") {
        CHECK(BigDecimal{"123456789.7e+50"}.toString() == "1234567897e+49");
        CHECK(BigDecimal{""} == 0);
        CHECK(BigDecimal{std::string("123456789.7e50")}.toString() == "1234567897e+49");
        CHECK(BigDecimal{"01000.000e5"} == BigDecimal{"1000e5"});
        CHECK(BigDecimal{".5e1"} == 5);
        CHECK_THROWS_AS(BigDecimal{"1.0ea"}, std::invalid_argument);
        CHECK_THROWS_AS(BigDecimal{"1e."}, std::invalid_argument);
        CHECK_THROWS_AS(BigDecimal{"a"}, std::invalid_argument);
        CHECK_THROWS_AS(BigDecimal{"1.b"}, std::invalid_argument);
    }
}

TEST_SUITE("Conversions") {
    TEST_CASE("toLongLong") {
        CHECK(BigDecimal{"1.7e10"}.toLongLong() == 17e9);
        CHECK_THROWS_AS(BigDecimal{"9e19"}.toLongLong(), std::overflow_error);
    }
    TEST_CASE("toLongDouble") {
        CHECK(BigDecimal{}.toLongDouble() == 0);
        //Small integers can be precisely represented as doubles
        CHECK(BigDecimal{12345}.toLongDouble() == 12345);
    }
    TEST_CASE("toString") {
        CHECK(BigDecimal{}.toString() == "0");
        CHECK(BigDecimal{"-50001e-2"}.toString() == "-500.01");
        CHECK(BigDecimal{"100e-4"}.toString() == "1e-2");
    }
}

TEST_SUITE("Operators") {
    TEST_CASE("+=") {
        BigDecimal bd{1};
        bd += BigDecimal{"1e10"};
        CHECK(bd == 10000000001);
        bd = BigDecimal{"1e10"};
        bd += 1;
        CHECK(bd == 10000000001);
        bd = BigDecimal{"233465.76894e-50"};
        bd += BigDecimal{"-233465.76894e-50"};
        CHECK(bd == 0);
    }
    TEST_CASE("-=") {
        BigDecimal bd{1234};
        bd -= bd;
        CHECK(bd == 0);
        bd = -1234;
        bd -= 1234;
        CHECK(bd == -2468);
    }
    TEST_CASE("*=") {
        BigDecimal bd{"1e50"};
        bd *= 5;
        CHECK(bd == BigDecimal{"5e50"});
        bd *= bd;
        CHECK(bd == BigDecimal{"25e100"});
        bd *= BigDecimal{"0.2"};
        CHECK(bd == BigDecimal{"5e100"});
    }
    TEST_CASE("/=") {
        BigDecimal bd{1000};
        bd /= BigDecimal{"0.2"};
        CHECK(bd == 5000);
        bd /= 200;
        CHECK(bd == 25);
        bd /= BigDecimal{"5e-5"};
        CHECK(bd == 500000);
        bd /= BigDecimal{"1e7"};
        CHECK(bd == BigDecimal{".05"});
    }
    TEST_CASE("%=") {
        BigDecimal bd{1000};
        bd %= 7;
        CHECK(bd == 6);
        bd %= 0.7;
        CHECK(bd == BigDecimal{"0.4"});
        bd %= BigDecimal{"2e50"};
        CHECK(bd == BigDecimal{"0.4"});
        bd %= bd;
        CHECK(bd == 0);
        bd = -13;
        bd %= 3;
        CHECK(bd == -1);
        bd = -13;
        bd %= -3;
        CHECK(bd == -1);
    }
    TEST_CASE("++ prefix") {
        BigDecimal bd = 5;
        CHECK(++bd == 6);
        bd = -5;
        CHECK(++bd == -4);
    }
    TEST_CASE("++ postfix") {
        BigDecimal bd = 5;
        CHECK(bd++ == 5);
        CHECK(bd == 6);
        bd = -5;
        CHECK(bd++ == -5);
        CHECK(bd == -4);
    }
    TEST_CASE("-- prefix") {
        BigDecimal bd = 5;
        CHECK(--bd == 4);
        bd = -5;
        CHECK(--bd == -6);
    }
    TEST_CASE("-- postfix") {
        BigDecimal bd = 5;
        CHECK(bd-- == 5);
        CHECK(bd == 4);
        bd = -5;
        CHECK(bd-- == -5);
        CHECK(bd == -6);
    }
    TEST_CASE("<<") {
        std::stringstream ss{};
        ss << BigDecimal{"-50001e-2"};
        CHECK(ss.str() == "-500.01");
    }
    TEST_CASE(">>") {
        BigDecimal bd;
        std::stringstream ss{"-5.7e+10 7.1ea\n12345"};
        ss >> bd;
        CHECK(bd.toString() == "-57e+9");
        CHECK_FALSE(ss.fail());
        ss >> bd;
        CHECK(ss.fail());
        ss.clear();
        ss >> bd;
        CHECK(bd == 12345);
        CHECK_FALSE(ss.fail());
    }
    //Non-assignment operators are all defined in terms of the assignment ones; if these fail, the corresponding assignment one should too
    TEST_CASE("+") {
        CHECK(BigDecimal{5} + 10 == 15);
        CHECK(1 + BigDecimal{"1e+5"} == 100001);
    }
    TEST_CASE("-") {
        CHECK(BigDecimal{5} - 10 == -5);
        CHECK(1 - BigDecimal{"1e+5"} == -99999);
    }
    TEST_CASE("*") {
        CHECK(BigDecimal{5} * 10 == 50);
        CHECK(2 * BigDecimal{"1e+5"} == 200000);
    }
    TEST_CASE("/") {
        CHECK(BigDecimal{5} / 10 == BigDecimal{"0.5"});
        CHECK(1 / BigDecimal{"1e+5"} == 1e-5);
    }
    TEST_CASE("%") {
        CHECK(BigDecimal{56} % 11 == 1);
        CHECK(10 % BigDecimal{"7e-5"} == BigDecimal{"1e-5"});
    }
}