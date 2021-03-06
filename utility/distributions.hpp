/**
 * Some additional useful distributions (only bare operator()).
 */
#ifndef SKYLARK_DISTRIBUTIONS_HPP
#define SKYLARK_DISTRIBUTIONS_HPP

#include <boost/random.hpp>

namespace skylark {
namespace utility {


/**
 * Levy distribution
 */
template< typename ValueType >
struct standard_levy_distribution_t {

    // TODO not really sure this is the standard, or implemented correctly

    typedef ValueType result_type;

    standard_levy_distribution_t() {

    }

    template< typename URNG >
    ValueType operator()(URNG &prng) const {
        boost::random::gamma_distribution<ValueType> dist(0.5, 2);
        result_type y = static_cast<ValueType>(dist(prng));
        return (1.0 / y);
    }
    void reset() {}

};

/**
 * Radamachar distribution - +1 and -1 with equal probability.
 */
template< typename ValueType >
struct rademacher_distribution_t {

    typedef ValueType result_type;

    template< typename URNG >
    ValueType operator()(URNG &prng) const {
        double probabilities[] = { 0.5, 0.0, 0.5 };
        boost::random::discrete_distribution<> dist(probabilities);
        return static_cast<ValueType>(dist(prng)) - 1.0;
    }
    void reset() {}
};

/**
 * Uniform distribution
 */
template <typename ValueType> struct uniform_distribution_t {
    typedef ValueType result_type;
};

/**
 * Uniform distribution specialization for double's
 */
template <> struct uniform_distribution_t <double> {

    typedef double result_type;

    boost::random::uniform_real_distribution<double> distribution;

    uniform_distribution_t() {}

    uniform_distribution_t(double low, double high) :
      distribution(low, high) {}

    template< typename URNG >
    double operator()(URNG &urng) const {
        return distribution(urng);
    }
    void reset() {}
};

/**
 * Uniform distribution specialization for int's
 */
template <> struct uniform_distribution_t <int> {

    typedef int result_type;

    boost::random::uniform_int_distribution<int> distribution;

    uniform_distribution_t() {}

    uniform_distribution_t(int low, int high) :
      distribution(low, high) {}

    template< typename URNG >
    int operator()(URNG &urng) const {
        return distribution(urng);
    }
    void reset() {}
};

/**
 * Uniform distribution specialization for size_t
 */
template <> struct uniform_distribution_t <size_t> {

    typedef int result_type;

    boost::random::uniform_int_distribution<size_t> distribution;

    uniform_distribution_t() {}

    uniform_distribution_t(size_t low, size_t high) :
      distribution(low, high) {}

    template< typename URNG >
    int operator()(URNG &urng) const {
        return distribution(urng);
    }
    void reset() {}
};


/**
 * Uniform distribution specialization for bool's
 */
template <> struct uniform_distribution_t <bool> {

    typedef bool result_type;

    boost::random::uniform_int_distribution<int> distribution;

    uniform_distribution_t() {}

    template< typename URNG >
    bool operator()(URNG &urng) const {
        return (1==distribution(urng));
    }
    void reset() {}
};

} // namespace utility
} // namespace skylark

#endif // SKYLARK_DISTRIBUTIONS_HPP
