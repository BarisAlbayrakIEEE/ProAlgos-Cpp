/*
    Primes by Modular Counters
    ---------------------
    Description:
        Given a number N, find all prime numbers up to N (inclusive)
        using a counter based approach
        which finds the primes in the order of 10^7.
        The method is less efficient than the sieve algorithms
        but introduces an approach similar to the Incremental Sieve of Eratosthenes.

    Assumptions:
        1. Only the odd numbers can be prime
        2. 2 and 3 are known as primes before starting the inspections

    Approach:
        Starting from 5 and incrementing by 2, inspect all numbers for being a prime.
        Starts by creating two arrays:
            * An array to store the prime numbers
            * An array to store a counter for each prime number
        While inspecting the numbers starting from 5 and increasing by 2,
        each counter is incremented by 1.
        When a counter reaches the corresponding prime number,
        the counter is reset to 0 and the current number is known to be a composite.

        Initially we have 2 and 3 as the prime numbers
        and the corresponding counters are 1 and 0.
        Keep in mind that the algorithm semantically excludes 2.
        We can consider that the two arrays initially contain 3 and 0 respectively.
        Hence, initially:
            current counter array: { 0 }
            current prime array: { 3 } // 2 is excluded for convenience

        Then, in each cycle of the inspection loop,
        all the values in the counter array are incremented by 1.
        To illustrate, the 1st 3 cycles of the loop:
            for 5:
                current prime array: { 3 }
                current counter array: { 1 }
                -> no values in the counter array reached the corresponding prime value
                -> 5 is a prime
                update prime array: { 3, 5 } // 5 is added
                update counter array: { 1, 0 } // counter 0 is added for 5
            for 7:
                current prime array: { 3, 5 }
                current counter array: { 2, 1 }
                -> no values in the counter array reached the corresponding prime value
                -> 7 is a prime
                update prime array: { 3, 5, 7 } // 7 is added
                update counter array: { 2, 1, 0 } // counter 0 is added for 7
            for 9:
                current prime array: { 3, 5, 7 }
                current counter array: { 3, 2, 1 }
                -> the 1st value in the counter array reached the corresponding prime value
                -> 9 is not a prime
                update counter array: { 0, 2, 1 } // counter is reset to 0 for 3
                prime array remains the same

    Comparison with sieve algorithms:
        Pros:
            1. Multiplication is replaced by increment operation
            2. Better cache efficiency
            3. Fits very well to parallel computation as both arrays are parallel
               and store independent data.
        Cons:
            1. Lack of prediction of the future prime/composite numbers results with higher time complexity.
            2. After the square root of the limit value is reached,
               the sieve algorithm performs a single boolean comparison to detect the primes.
               However, the modular counter algorithm still performs increment operation on the counters.
    
    Additional discussion:
        The file has four additional algorithms based on the same modular counter approach.
        The additional algorithms are based on a simple idea that
        the counter increment operation can be replaced by a left shift.
        The basic algorithm defines the prime counters as integers (0,1,2,3,...)
        while the next four algorithms define the counters as powers of two: 2^i where i in [0,P).

        Hence, together with the basic counter algorithm, the file contains five solutions:
            1. The counters are integers and managed by increment operation.
               Time complexity per a cycle of the main loop:
                   O(n) where n is the number of primes for the current cycle
            2. The counters are stored in a static contiguous non-circular bitset and
               managed by left shift operation.
               Time complexity per a cycle of the main loop:
                   O(L) where L is the size of the static bitset, I assumed L=N
            3. The counters are stored in a static contiguous circular bitset and
               managed by an offset parameter.
               Time complexity per a cycle of the main loop:
                   O(1) + O(n) where
                       O(1) stands for the left shift (as its replaced by an offset: --offset)
                       O(n) stands for the counter reset operation (if counter == prime: reset counter)
            4. The counters are stored in a dynamic packed (uint64_t) non-circular bitset and
               managed by left shift operation.
               Time complexity per a cycle of the main loop:
                   O(L) where L is the bit-size of the static bitset, I assumed L=N
            5. The counters are stored in a dynamic packed (uint64_t) circular bitset and
               word-size packed bitset (vector<uint64_t>) and
               managed by an offset parameter.
               Time complexity per a cycle of the main loop:
                   O(1) + O(n) where
                       O(1) stands for the left shift (as its replaced by an offset: --offset)
                       O(n) stands for the counter reset operation (if counter == prime: reset counter)
        
        I performed a couple of benchmarks including the sieve algorithm and the above five.
        The results of the benchmarks for N=4096 in terms of the runtimes are:
            655, 476, 1689, 1060, 1879 and 534 microseconds where
            the 1st one is for the sieve algorithm in sieve_of_eratosthenes.hpp and
            the others are for the above five respectively.
        
        Firstly, the sieve algorithm looks taking longer time but
        notice that the number limit N is too small: 4096.
        For the small values of N, the counter algorithm would win the sieve algorithm
        as the modular counter algorithm has better spatial locality
        as it replaces the multiplication with increment operation.
        The 2nd result is that the bitset solutions are not
        more efficient than the basic counter algorithm
        although they end up with terminating the increment operation.
        This is a result of the fact that the bitset solutions have large bitset data (millions of bits)
        which terminates the locality yielding a bad cache usage.
        The 3rd result is that the last algorithm has the best runtime among the four bitset solutions
        which is actually expected due to:
            1. Packing the bits by uint64_t fits to the machine representation very well
               activating the straight 64-bit register operations.
            2. The circular data structure terminates the increment operation (i.e. left shift) at all.
    
    Parallelism:
        The problem with the traditional sieve algorithm is to achieve an effective parallelism
        while marking the multiples of a prime as composite.
        Consider, the sieve has a large boolean array to mark the composite numbers: bool arr[].
        In case of CPU parallelism, mainly, the following two problems are severe:
            1. Bad locality for large prime numbers
               arr[i * p] = false; where p is a large prime number
            2. CPU prefetcher becomes useless for large primes causing lots of cache misses.
        
        In case of GPU, mainly, the following two problems are severe:
            1. If threads conditionally write based on whether thread_id * p < N,
               some warps may partially diverge causing many idle threads.
            2. For large p, memory writes are scattered causing low cache reuse and huge latency.
        
        In case of the modular counter algorithm the above issues disappear.
        The algorithm is ideal, especially, for the GPU parallelism:
            ++counter_array[thread_id * p];
        
        The 2nd operation is similar which compares each counter with the corresponding prime number:
            If a counter C hits the corresponding prime P (i.e. C == P), it is reset to 0 (C = 0).
        
        The 3rd operation, on the other hand, is problematic as it works on a shared variable: is_prime.
        The comparison performed in the 2nd operation determines if is_prime is true or not.
        If no counter reaches to the corresponding prime, then is_prime is true.
        This is a reduction algorithm and has two solutions:
            1. Execute in parallel by synchronizing the shared data access for reads and writes
               which is highly serialized.
            2. Give each thread a bit in shared atomic words (uint64_t[]) and
               at the end, reduce the words using bitwise AND/OR operation.
               The reduction can be applied by divide and conquer strategy
               yielding the following complexities where P is the number of current primes:
                   CPU parallel: O(M/P + log(P)) where
                      M is the current number of primes P is the number of threads
                   GPU parallel: O(M/(B*T)) + O(log(T)) where
                      B is the number of blocks and T is the number of threads per block

    Time complexity
    ---------------
    Sequential (Asymptotically): Θ(N^(3/2) / ln(N)) where
        N is the number up to which primes have to be found
    CPU Parallel (Asymptotically): O(N^(3/2) / (T * log(N))) where
        T is the number of threads
    GPU Parallel (Asymptotically): O(N^(3/2) / (T * log(N)))
    
    Notice that the time complexity for the sequential solution is big theta
    while the other two are big O.
    Recall that, from sieve_of_eratosthenes.hpp, the time complexity of the sequential sieve algorithm is:
        O(N * log(log(N)))

    Space complexity
    ----------------
    Asymptotically: Θ(N), where N is the number up to which primes have to be found
*/

#ifndef PRIMES_BY_COUNTERS_HPP
#define PRIMES_BY_COUNTERS_HPP

#include <vector>
typedef std::vector<std::size_t> _PN_t;
constexpr std::size_t CEIL{ 10000000 };

// Determines primes up to prime_limit and returns list of primes in a vector
// Method:
//     1. Assigning a counter for each prime
//     2. Increment the counters and ispect for the equality to the corresponding prime number
//     3. Prime counters are defined by vector of unsigned integers
_PN_t get_primes_counter(const std::size_t &prime_limit) {
    //  Check if larger than limit
    if (prime_limit > CEIL) {
        return _PN_t({});
    }
    
    // Initialize two vectors for the prime numbers and the corresponding counters.
    _PN_t prime_numbers{ 2, 3 };
    _PN_t prime_counters{ 1, 0 };
    std::size_t inspectable_prime_count{ 1 };

    // loop to inspect each odd number for being a prime: 5, 7, 9, ...
    for (std::size_t current_number = 5; current_number <= prime_limit; current_number += 2) {
        // increment the counters of the current prime numbers.
        // if any of the counters reaches the corresponding prime value,
        // (e.g. the counter of 3 reaches to 3 or counter of 11 reaches to 11)
        // that counter is reset to 0 and the current number is known to be a composite.
        // Notice that the index starts from 1 omitting the counter of the prime 2.
        bool check_prime{ true };
        for (int i = 0; i < inspectable_prime_count; i++) {
            ++prime_counters[i + 1];
            if (prime_counters[i + 1] == prime_numbers[i + 1]) {
                prime_counters[i + 1] = 0;
                check_prime = false;
            }
        }
        if (check_prime) {
            prime_numbers.push_back(current_number);

            // an optimization for square root of the limit.
            // no need to inspect the prime numbers larger than the square root of the limit.
            if (current_number <= std::sqrt(prime_limit)) {
                ++inspectable_prime_count;
                prime_counters.push_back(0);
            }
        }
    }
    return prime_numbers;
}



// *****************************************************************************
// CAUTION:
//   THE CODE BELOW IS FOR THE DISCUSSION RELATED TO THE LEFT SHIFT OPERATION
//   HELD IN THE MAIN DOCUMENTATION OF THIS HEADER
//   AND NOT INCLUDED IN THE UNIT TEST (primes_by_counters.cpp)
// *****************************************************************************



#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <bitset>

typedef std::vector<uint64_t> _Packed_t;

constexpr std::size_t POOL_SIZE{ 4096 };
constexpr std::size_t BLOCK_SIZE0{ 64 };
constexpr std::size_t BLOCK_SIZE1{ 63 };

struct Contiguous;
struct Packed;

/*
Base template for:
  Static contiguous non-circular bitset
*/
template <typename Memory_Type, bool Circular, std::size_t N>
class BitSet {
public:
    bool inline get_bit(std::size_t pos) const noexcept { return _data[pos]; }
    void inline set_bit(std::size_t pos, bool value) noexcept { _data.set(pos, value); }
    void inline shift_left() noexcept { _data <<= 1; }

private:
    std::bitset<N> _data;
};

/*
Template specialization for:
  Static contiguous circular bitset
*/
template <std::size_t N>
class BitSet<Contiguous, true, N> {
public:
    BitSet() : _offset(N) {};

    bool inline get_bit(std::size_t pos) const noexcept { return _data[_offset + pos]; }
    void inline set_bit(std::size_t pos, bool value) noexcept { _data.set(_offset + pos, value); }
    void inline shift_left() noexcept { --_offset; } // move window right -> O(1)

private:
    std::bitset<2 * N> _data;
    std::size_t _offset;
};

/*
Template specialization for:
  Dynamic packed (uint64_t) non-circular bitset
*/
template <std::size_t N>
class BitSet<Packed, false, N> {
public:
    BitSet() {
        _data.resize((N + BLOCK_SIZE1) / BLOCK_SIZE0, 0ULL);
    }
    bool inline get_bit(std::size_t pos) const noexcept {
        return (_data[pos / BLOCK_SIZE0] >> (pos % BLOCK_SIZE0)) & 1ULL;
    }
    void set_bit(std::size_t pos, bool value) noexcept {
        std::size_t pos_block = pos / BLOCK_SIZE0;
        std::size_t pos_bit   = pos % BLOCK_SIZE0;
        uint64_t mask = 1ULL << pos_bit;
        if (value) _data[pos_block] |= mask;
        else _data[pos_block] &= ~mask;
    }
    void shift_left() noexcept {
        uint64_t carry = 0ULL;
        for (size_t i = 0; i < _data.size(); ++i) {
            uint64_t new_carry = _data[i] >> BLOCK_SIZE1;
            _data[i] = (_data[i] << 1) | carry;
            carry = new_carry;
        }
    }

private:
    _Packed_t _data;
};

/*
Template specialization for:
  Dynamic packed (uint64_t) circular bitset
*/
template <std::size_t N>
class BitSet<Packed, true, N> {
public:
    BitSet()
        : _size_logical(N), _size_physical(2 * N), _offset(N)
    {
        _data.resize((_size_physical + BLOCK_SIZE1) / BLOCK_SIZE0, 0ULL);
    }
    bool inline get_bit(std::size_t pos) const noexcept {
        std::size_t pos_global = _offset + pos;
        return (_data[pos_global / BLOCK_SIZE0] >> (pos_global % BLOCK_SIZE0)) & 1ULL;
    }
    void set_bit(std::size_t pos, bool value) noexcept {
        std::size_t pos_global = _offset + pos;
        std::size_t pos_block = pos_global / BLOCK_SIZE0;
        std::size_t pos_bit   = pos_global % BLOCK_SIZE0;
        uint64_t mask = 1ULL << pos_bit;
        if (value) _data[pos_block] |= mask;
        else _data[pos_block] &= ~mask;
    }
    void inline shift_left() noexcept { --_offset; } // move window right -> O(1)

private:
    _Packed_t _data;
    std::size_t _size_logical;
    std::size_t _size_physical;
    std::size_t _offset;
};

// Determines primes up to prime_limit and returns list of primes in a vector
// Method:
//     1. Assigning a counter for each prime
//     2. Multiply the counters by two (i.e. left shift) and
//        inspect for the equality to 2 to the power of the corresponding prime number
//     3. For specializations for defining the counters
//        a. Memory_Type=Contiguous && Circular=false
//           static contiguous non-circular bitset
//        b. Memory_Type=Contiguous && Circular=true
//           static contiguous circular bitset
//        c. Memory_Type=Packed && Circular=false
//           dynamic packed (uint64_t) non-circular bitset
//        d. Memory_Type=Packed && Circular=true
//           dynamic packed (uint64_t) circular bitset
template <typename Memory_Type, bool Circular, std::size_t N>
_PN_t get_primes_bitset() {
    //  Check if larger than limit
    if (N > CEIL) {
        return _PN_t({});
    }

    // Initialize the prime number vector and the counters.
    BitSet<Memory_Type, Circular, N> prime_counters;
    prime_counters.set_bit(0, true);
    _PN_t prime_numbers{ 2, 3 };
    std::size_t inspectable_prime_count{ 1 };
 
    // loop to inspect each odd number for being a prime: 5, 7, 9, ...
    long int pos_initial{ -1 };
    for (std::size_t current_number = 5; current_number <= N; current_number += 2) {
        // multiply the counters of the current prime numbers by two (i.e. left shift).
        prime_counters.shift_left();

        // if any of the counters reaches 2 to the power of the corresponding prime value,
        // (e.g. the counter of 3 reaches to 2^3 or counter of 11 reaches to 2^11)
        // that counter is reset to 1 and the current number is known to be a composite.
        bool check_prime{ true };
        long int pos = pos_initial;
        for (int i = 0; i < inspectable_prime_count; i++) {
            pos += prime_numbers[i + 1] + 1;
            if (prime_counters.get_bit(pos)) {
                prime_counters.set_bit(pos, false);
                prime_counters.set_bit(pos - prime_numbers[i + 1], true);
                check_prime = false;
            }
        }
        if (check_prime) {
            prime_numbers.push_back(current_number);

            // an optimization for square root of the limit.
            // no need to inspect the prime numbers larger than the square root of the limit.
            if (current_number <= std::sqrt(N)) {
                ++inspectable_prime_count;
                prime_counters.set_bit(pos + 1, true);
            }
        }
    }
    return prime_numbers;
}

#endif // PRIMES_BY_COUNTERS_HPP
