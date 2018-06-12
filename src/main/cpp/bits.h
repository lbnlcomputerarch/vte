// written by Albert Chen

#ifndef OPENSOC_BITS_H
#define OPENSOC_BITS_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <sstream>

class Bits {
private:
    static const size_t WORD_LEN = 64;

    size_t width;
    size_t num_words;
    std::vector<uint64_t> data;

    // for constructing Bits of all zero dont_care parameter is to prevent
    // conflicts with other constructors
    explicit Bits(size_t _width, bool dont_care);

    // implementation of Bits(uint64_t/uint32_t/uint16_t/uint8_t) constructor,
    // makes a Bits with same bits as Value, dont_care1/2 are for preventing
    // conflicts with other constructors
    template <class Value>
    explicit Bits(Value value, bool dont_care1, bool dont_care2);

    // implementation of Bits(vector<uint64_t/uint32_t/uint16_t/uint8_t>)
    // constructor, packs the values of the vector into a Bits instance with
    // values[0] corresponding to the lower bits
    template <class Value>
    explicit Bits(std::vector<Value> values, bool dont_care);

    // implementation of append(uint64_t/uint32_t/uint16_t/uint8_t),
    // appends an extra sizeof(Value) * 8 bits with value
    template <class Value>
    void append_word(Value value);

    // implementation of prepend(uint64_t/uint32_t/uint16_t/uint8_t), left
    // shifts this instance by sizeof(Value) * 8 bits and fills the lower zeros
    // with value
    template <class Value>
    void prepend_word(Value value);

    inline size_t get_num_words();

    inline void set_num_words(size_t _num_words);

public:
    static Bits zeros(size_t width);
    static Bits random(size_t width);

    Bits();

    // constructs a 64 wide Bits with value
    Bits(uint64_t value);

    // constructs a 32 wide Bits with value
    // Bits(uint32_t value);

    // constructs a 16 wide Bits with value
    // Bits(uint16_t value);

    // constructs a 8 wide Bits with value
    // Bits(uint8_t value);

    // constructs a 64 * values.size() wide Bits packed with the elements of
    // i.e. *this((i * 64) -1, i * 64) = values[i] etc.
    Bits(std::vector<uint64_t> &values);

    // constructs a 32 * values.size() wide Bits packed with the elements of
    // i.e. *this((i * 32) -1, i * 32) = values[i] etc.
    Bits(std::vector<uint32_t> &values);

    // constructs a 16 * values.size() wide Bits packed with the elements of
    // i.e. *this((i * 16) -1, i * 16) = values[i] etc.
    Bits(std::vector<uint16_t> &values);

    // constructs a 8 * values.size() wide Bits packed with the elements of
    // i.e. *this((i * 8) -1, i * 8) = values[i] etc.
    Bits(std::vector<uint8_t> &values);

    // returns a new Bits instance containing bits from position high to
    // position low inclusive
    Bits operator()(size_t high, size_t low);

    Bits operator>>(size_t shamt);

    void operator>>=(size_t shamt);

    Bits operator<<(size_t shamt);

    void operator<<=(size_t shamt);

    Bits operator^(Bits &operand);

    void operator^=(Bits &operand);

    Bits operator|(Bits &operand);

    void operator|=(Bits &operand);

    Bits operator&(Bits &operand);

    void operator&=(Bits &operand);

    Bits operator~();

    // both bits must have the same width, will not sign or zero extend
    // before checking
    bool operator==(Bits &operand);

    // appends 64 bits to the end of this instance with value
    void append(uint64_t value);

    // appends 32 bits to the end of this instance with value
    void append(uint32_t value);

    // appends 16 bits to the end of this instance with value
    void append(uint16_t value);

    // appends 8 bits to the end of this instance with value
    void append(uint8_t value);

    // left shifts by 64 bits and fills the zeros with value
    void prepend(uint64_t value);

    // left shifts by 32 bits and fills the zeros with value
    void prepend(uint32_t value);

    // left shifts by 16 bits and fills the zeros with value
    void prepend(uint16_t value);

    // left shifts by 8 bits and fills the zeros with value
    void prepend(uint8_t value);

    size_t get_width();

    // truncates the higher order bits if new_width is shorter, otherwise
    // zero extends
    void set_width(size_t new_width);

    void set_word(size_t i, uint64_t word);

    uint64_t get_word(int i);

    // inputs the hexadecimal representation of this instance to o
    virtual std::ostream &print(std::ostream &o);
};

std::ostream &operator<<(std::ostream &o, Bits bits);

#endif
