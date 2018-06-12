// adapted from chisel3.iotesters by Albert Chen

#ifndef __VERILATOR_API__
#define __VERILATOR_API__

#include <verilated.h>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include "bits.h"

class VerilatorDataWrapper {
public:
    virtual Bits get_value() = 0;

    virtual void put_value(Bits& bits) = 0;

    virtual size_t get_width() = 0;

    virtual std::string get_name() = 0;
};

class VerilatorCData : public VerilatorDataWrapper {
public:
    VerilatorCData(std::string _name, size_t _width, CData *_signal) : name(std::move(_name)) {
      signal = _signal;
      width = _width;
    }

    Bits get_value() override {
        Bits value(*signal);
        value.set_width(get_width());
        return value;
    }

    void put_value(Bits& bits) override {
      uint64_t mask = 0xff;
      *signal = (CData)(mask & bits.get_word(0));
    }

    std::string get_name() override {
      return name;
    }

    size_t get_width() override {
      return width;
    }

private:
    CData *signal;
    const std::string name;
    size_t width;
};

class VerilatorSData : public VerilatorDataWrapper {
public:
    VerilatorSData(std::string _name, size_t _width, SData *_signal) : name(std::move(_name)) {
      signal = _signal;
      width = _width;
    }

    Bits get_value() override {
        Bits value(*signal);
        value.set_width(get_width());
        return value;
    }

    void put_value(Bits& bits) override {
      uint64_t mask = 0xffff;
      *signal = (SData)(mask & bits.get_word(0));
    }

    std::string get_name() override {
      return name;
    }

    size_t get_width() override {
      return width;
    }

private:
    SData *signal;
    const std::string name;
    size_t width;
};

class VerilatorIData : public VerilatorDataWrapper {
public:
    VerilatorIData(std::string _name, size_t _width, IData *_signal) : name(std::move(_name)) {
      signal = _signal;
      width = _width;
    }

    Bits get_value() override {
        Bits value(*signal);
        value.set_width(get_width());
        return value;
    }

    void put_value(Bits& bits) override {
      uint64_t mask = 0xffffffff;
      *signal = (IData)(mask & bits.get_word(0));
    }

    std::string get_name() override {
      return name;
    }

    size_t get_width() override {
      return width;
    }

private:
    IData *signal;
    std::string name;
    size_t width;
};

class VerilatorQData : public VerilatorDataWrapper {
public:
    VerilatorQData(std::string _name, size_t _width, QData *_signal) : name(std::move(_name)) {
      signal = _signal;
      width = _width;
    }

    Bits get_value() override {
        Bits value(*signal);
        value.set_width(get_width());
        return value;
    }

    void put_value(Bits& bits) override {
      *signal = (QData)(bits.get_word(0));
    }

    std::string get_name() override {
      return name;
    }

    size_t get_width() override {
      return width;
    }

private:
    QData *signal;
    std::string name;
    size_t width;
};

class VerilatorWData : public VerilatorDataWrapper {
public:
    VerilatorWData(std::string _name, size_t _width, WData *_wdatas) : name(std::move(_name)) {
      wdatas = _wdatas;
      numWdatas = (_width + 31)/32;
      width = _width;
    }

    Bits get_value() override {
        Bits bits = Bits::zeros(get_width());
        bool numWdatasEven = (numWdatas % 2) == 0;
        for (size_t i = 0; i < numWdatas / 2; i++) {
            uint64_t value = ((uint64_t) wdatas[i * 2 + 1]) << 32 | wdatas[i * 2];
            bits.set_word(i, value);
        }
        if (!numWdatasEven) {
            bits.set_word(numWdatas / 2, wdatas[numWdatas - 1]);
        }
        return bits;
    }

    void put_value(Bits& bits) override {
      bool numWdatasEven = (numWdatas % 2) == 0;
      for (int i = 0; i < numWdatas / 2; i++) {
        uint64_t word = (bits.get_width() > (i * 64)) ? bits.get_word(i) : 0;
        wdatas[i * 2] = word;
        wdatas[i * 2 + 1] = word >> 32;
      }
      if (!numWdatasEven) {
        uint32_t word = (bits.get_width() > ((numWdatas/2) * 64)) ? bits.get_word(numWdatas / 2) : 0;
        wdatas[numWdatas - 1] = word;
      }
    }

    std::string get_name() override {
      return name;
    }

    size_t get_width() override {
      return width;
    }

private:
    WData *wdatas;
    size_t numWdatas;
    std::string name;
    size_t width;
};

#endif
