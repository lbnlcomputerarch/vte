// written by Albert Chen

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "bits.h"

Bits::Bits(size_t _width, bool dont_care) {
    assert(_width > 0);

    width = _width;
    for (size_t i = 0; i < width; i += WORD_LEN)
        data.push_back(0);

    num_words = (width + WORD_LEN - 1) / WORD_LEN;
}

size_t Bits::get_num_words() {
    return num_words;
}

void Bits::set_num_words(size_t _num_words) {
    num_words = _num_words;
}

Bits Bits::zeros(size_t width) {
    return Bits(width, false);
}

Bits Bits::random(size_t width) {
    Bits result = Bits::zeros(width);
    for (size_t i = 0; i < result.get_num_words() - 1; i++) {
        result.data[i] = rand();
    }

    result.set_word(result.get_num_words() - 1, rand() % (((size_t) 1) << (width % WORD_LEN)));
    return result;
}

Bits::Bits() {
    width = 0;
    num_words = 0;
}


template<class Value>
Bits::Bits(Value value, bool dont_care1, bool dont_care2) {
    width = sizeof(Value) * 8;
    data.push_back((uint64_t) value);
    num_words = 1;
}

Bits::Bits(uint64_t value) : Bits(value, false, false) {}

// Bits::Bits(uint32_t value) : Bits(value, false, false) {}

// Bits::Bits(uint16_t value) : Bits(value, false, false) {}

// Bits::Bits(uint8_t value) : Bits(value, false, false) {}


template<class Value>
Bits::Bits(std::vector<Value> values, bool dont_care) : Bits(values.size() * sizeof(Value) * 8, false) {
    size_t values_per_word = WORD_LEN / (sizeof(Value) * 8);
    for (size_t i = 0; i < values.size(); i++)
        data[i / values_per_word] |= values[i] << (i % values_per_word);
}

Bits::Bits(std::vector<uint8_t> &values) : Bits(values, false) {}

Bits::Bits(std::vector<uint16_t> &values) : Bits(values, false) {}

Bits::Bits(std::vector<uint32_t> &values) : Bits(values, false) {}

Bits::Bits(std::vector<uint64_t> &values) : Bits(values, false) {}


Bits Bits::operator()(size_t high, size_t low) {
    assert(high < get_width());
    assert(low < high);

    size_t result_width = high - low;
    Bits result = *this >> low;
    result.set_width(result_width);

    return result;
}


void Bits::operator>>=(size_t shamt) {
    size_t word_idx = 0;

    /* if shamt is greater than width return zero */
    if (shamt >= get_width()) {
        for (; word_idx < get_num_words(); word_idx++)
            data[word_idx] = 0;

    } else {
        size_t word_shamt = shamt / WORD_LEN;

        /* shift bits, faster if word-aligned */
        if (shamt % WORD_LEN == 0) {
            for (; (word_idx + word_shamt) < get_num_words(); word_idx++)
                data[word_idx] = (data[word_idx + word_shamt]);

        } else {
            size_t word_offset = shamt % WORD_LEN;
            for (; (word_idx + word_shamt) < get_num_words(); word_idx++) {
                uint64_t word_lower = data[word_idx + word_shamt] >> word_offset;
                uint64_t word_upper = data[word_idx + word_shamt + 1] << (WORD_LEN - word_offset);
                data[word_idx] = word_upper | word_lower;
            }
        }

        /* add zeros so that widths will be equal */
        for (; word_idx < get_num_words(); word_idx++)
            data[word_idx] = 0;
    }
}

void Bits::operator<<=(size_t shamt) {
    size_t word_idx = 0;

    /* if shamt is greater than width return zero */
    if (shamt >= get_width()) {
        for (; word_idx < get_num_words(); word_idx++)
            data[word_idx] = 0;

    } else {
        size_t word_shamt = shamt / WORD_LEN;

        /* fill shift amount with zeros */
        for (; word_idx < word_shamt; word_idx++)
            data[word_idx] = 0;

        /* shift bits, faster if word-aligned */
        if (shamt % WORD_LEN == 0) {
            for (; word_idx < get_num_words(); word_idx++)
                data[word_idx] = (data[word_idx - word_shamt]);

        } else {
            size_t word_offset = shamt % WORD_LEN;
            for (; word_idx < get_num_words(); word_idx++) {
                uint64_t word_lower = data[word_idx - word_shamt] >> (WORD_LEN - word_offset);
                uint64_t word_upper = data[word_idx - word_shamt + 1] << word_offset;
                data[word_idx] = word_upper | word_lower;
            }
        }
    }
}

Bits Bits::operator>>(size_t shamt) {
    Bits result(get_width());
    result.data = data;
    result >>= shamt;
    return result;
}

Bits Bits::operator<<(size_t shamt) {
    Bits result(*this);
    result <<= shamt;
    return result;
}

Bits Bits::operator^(Bits &operand) {
    size_t max_width = get_width() > operand.get_width() ? get_num_words() :
                       operand.get_num_words();
    Bits result(*this);
    result.set_width(max_width);
    result ^= operand;
    return result;
}

void Bits::operator^=(Bits &operand) {
    size_t word_idx = 0;
    size_t min_words = get_width() > operand.get_width() ? operand.get_num_words() :
                       get_num_words();

    for (; word_idx < min_words; word_idx++)
        data[word_idx] = data[word_idx] ^ operand.data[word_idx];

    for (; word_idx < get_num_words(); word_idx++)
        data[word_idx] = 0;
}

void Bits::operator|=(Bits &operand) {
    size_t word_idx = 0;
    size_t min_words = get_width() > operand.get_width() ? operand.get_num_words() :
                       get_num_words();

    for (; word_idx < min_words; word_idx++)
        data[word_idx] = data[word_idx] | operand.data[word_idx];

    for (; word_idx < get_num_words(); word_idx++)
        data[word_idx] = 0;
}

Bits Bits::operator|(Bits &operand) {
    size_t max_width = get_width() > operand.get_width() ? get_num_words() :
                       operand.get_num_words();
    Bits result(*this);
    result.set_width(max_width);
    result |= operand;
    return result;
}

Bits Bits::operator&(Bits &operand) {
    size_t max_width = get_width() > operand.get_width() ? get_num_words() :
                       operand.get_num_words();
    Bits result(*this);
    result.set_width(max_width);
    result &= operand;
    return result;
}

void Bits::operator&=(Bits &operand) {
    size_t word_idx = 0;
    size_t min_words = get_width() > operand.get_width() ? operand.get_num_words() :
                       get_num_words();

    for (; word_idx < min_words; word_idx++)
        data[word_idx] = data[word_idx] & operand.data[word_idx];

    for (; word_idx < get_num_words(); word_idx++)
        data[word_idx] = 0;
}

Bits Bits::operator~() {
    Bits result(*this);
    for (size_t i = 0; i < get_num_words(); i ++) {
        result.data[i] = ~result.data[i];
    }

    size_t extra_bits = WORD_LEN - (get_width() % WORD_LEN);
    result.data[get_num_words() - 1] &= (~((size_t) 0)) >> extra_bits;
    return result;
}

size_t Bits::get_width() {
    return width;
}

void Bits::set_width(size_t new_width) {
    assert(new_width > 0);

    size_t old_num_words = get_num_words();
    size_t new_num_words = (get_width() + WORD_LEN - 1) / WORD_LEN;
    if (new_num_words > old_num_words) {
        for (size_t i = old_num_words; i < new_num_words; i++)
            data.push_back(0);
    } else {
        for (size_t i = new_num_words; i < old_num_words; i++)
            data.pop_back();
    }

    size_t extra_bits = WORD_LEN - (new_width % WORD_LEN);
    data[new_num_words - 1] &= (~((size_t) 0)) >> extra_bits;
    set_num_words(new_num_words);
}

bool Bits::operator==(Bits &operand) {

    if (get_width() != operand.get_width()) {
        return false;
    }

    size_t word_idx = 0;
    for (; word_idx < get_num_words(); word_idx++) {
        if (data[word_idx] != operand.data[word_idx]) {
            return false;
        }
    }

    return true;
}

void Bits::set_word(size_t i, uint64_t word) {
    data[i] = word;
}

uint64_t Bits::get_word(int i) {
    return data[i];
}

std::ostream &Bits::print(std::ostream &o) {
    std::ios state(NULL);
    state.copyfmt(o);
    o << "0x";
    for (size_t i = get_num_words() - 1; i >= 1; i--)
        o << std::hex << data[i] << " ";
    o << std::hex << data[0];

    o.copyfmt(state);
    return o;
}


template<class Value>
void Bits::append_word(Value value) {
    uint64_t word = (uint64_t) value;
    size_t value_width = sizeof(Value) * 8;

    size_t shamt = get_width() % WORD_LEN;

    data[get_num_words() - 1] = (word << shamt) | data[get_num_words() - 1];

    if (shamt > (WORD_LEN - value_width)) {
        data.push_back(word >> (WORD_LEN - shamt));
        num_words += 1;
    }

    width += value_width;
}

void Bits::append(uint64_t value) { append_word(value); }

void Bits::append(uint32_t value) { append_word(value); }

void Bits::append(uint16_t value) { append_word(value); }

void Bits::append(uint8_t value) { append_word(value); }


template<class Value>
void Bits::prepend_word(Value value) {
    uint64_t word = (uint64_t) value;
    size_t value_width = sizeof(Value) * 8;
    (*this) << value_width;

    data[0] |= word;
}

void Bits::prepend(uint64_t value) { prepend_word(value); }

void Bits::prepend(uint32_t value) { prepend_word(value); }

void Bits::prepend(uint16_t value) { prepend_word(value); }

void Bits::prepend(uint8_t value) { prepend_word(value); }

std::ostream &operator<<(std::ostream &o, Bits bits) {
    return bits.print(o);
}
