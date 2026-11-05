#include "tester.hpp"

#include <limits>

namespace {

bool nearCheckFails(double actual, double expected, double epsilon) {
    try { CHECK_NEAR(actual, expected, epsilon); }
    catch (const tester::Failure&) { return true; }
    return false;
}

TEST(TesterFramework, RejectsNaN) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    CHECK(nearCheckFails(nan, 1.0, 0.1));
    CHECK(nearCheckFails(1.0, nan, 0.1));
    CHECK(nearCheckFails(nan, nan, 0.1));
}

TEST(TesterFramework, RejectsInvalidTolerance) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(nearCheckFails(1.0, 1.0, -0.1));
    CHECK(nearCheckFails(1.0, 1.0, nan));
    CHECK(nearCheckFails(1.0, 1.0, inf));
}

TEST(TesterFramework, HandlesInfinity) {
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(tester::near_equal(inf, inf, 0.0));
    CHECK(tester::near_equal(-inf, -inf, 0.0));
    CHECK(!tester::near_equal(inf, -inf, 0.0));
    CHECK(!tester::near_equal(inf, 1.0, 1.0));
}

TEST(TesterFramework, BoundaryDifference) {
    CHECK(tester::near_equal(1.0, 1.25, 0.25));
    CHECK(!tester::near_equal(1.0, 1.2501, 0.25));
}

}
