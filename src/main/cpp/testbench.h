// written by Albert Chen

#ifndef TESTBENCH_H
#define TESTBENCH_H

#include "veri_api.h"
#if VM_TRACE
#include "verilated_vcd_c.h"
#endif
#include "veri_aggregate_api.h"
#include "bits.h"
#include <iostream>
#include <verilated.h>
#include <vector>
#include <map>
#include <string>
#include <list>

class VerilatorBundle;

template<class Data>
class VerilatorVec;

template<class Module>
class Testbench {
public:
    unsigned long m_tickcount;
    Module *dut;
    bool failed;
    unsigned long first_failed_cycle;
    static vluint64_t main_time;

    Testbench() {
        dut = new Module;
        m_tickcount = 0l;
        failed = false;
        main_time = 0;
#if VM_TRACE
        tfp = NULL;
#endif
    }

    virtual ~Testbench() {
        delete dut;
        dut = NULL;
    }

    // sets reset to 1 for num_cycles cycles
    virtual void reset(int num_cycles) {
        for (int i = 0; i < num_cycles; i++) {
            dut->reset = 1;
            this->step(1);

        }
        dut->reset = 0;
    }
#if VM_TRACE
    void init_dump(VerilatedVcdC* _tfp) { tfp = _tfp; }
#endif

#if VM_TRACE
    VerilatedVcdC* tfp;
#endif
    // pokes each vec element with is corresponding element in values
    // i.e. poke(vec[i], values[i]) for i = 0 until length of vec
    template<class Data>
    void poke(VerilatorVec<Data> &vec, std::vector<Bits> &values) {
        for (size_t i = 0; i < vec.get_num_elements(); i++)
            poke(vec[i], values[i]);
    }

    // pokes each bundle element with is corresponding element in values
    // key strings of values should match the names of the bundle fields
    // use '.' to access fields of nested bundle elements
    void poke(VerilatorBundle &bundle, std::map<std::string, Bits> &values);

    // puts the value contained in bits into wire, will zero-extend or truncate
    // bits in order to fit the width of wire.
    // also prints "  POKE $wire_name <- $poke_value"
    void poke(VerilatorDataWrapper &wire, Bits bits) {
        //assert(wire.get_width() == bits.get_width());
        bits.set_width(wire.get_width());

        std::cout << "  POKE " << wire.get_name() << " <- " << bits << std::endl;
        wire.put_value(bits);
    }

    // returns a Bits instance containing the value of wire
    Bits peek(VerilatorDataWrapper &wire) {
        dut->eval();
        return wire.get_value();
    }

    std::map<std::string, Bits> peek(VerilatorBundle &bundle);

    template<class Data>
    std::vector<Bits> peek(VerilatorVec<Data> &vec);

    // toggles the clock num_steps times,
    // prints "STEP $current_cycle_count -> $new_cycle_count"
    virtual void step(int num_steps) {
        // Increment our own internal time reference
        std::cout << "STEP " << m_tickcount << " -> ";
        m_tickcount += num_steps;
        std::cout << m_tickcount << std::endl;

        for (int i = 0; i < num_steps; i++) {
            // Make sure any combinatorial logic depending upon
            // inputs that may have changed before we called tick()
            // has settled before the rising edge of the clock.
            dut->clock = 0;
            dut->eval();
#if VM_TRACE
            if (tfp) tfp->dump(main_time);
#endif
            // Toggle the clock
            main_time += 1;
            // Rising edge
            dut->clock = 1;
            dut->eval();
#if VM_TRACE
            if (tfp) tfp->dump(main_time);
#endif  
            main_time += 1;
        }
    }

    // first truncates or zero-extends expected_value to match the width of
    // wire, then checks if wire contains the same value as expected_value,
    void expect(VerilatorDataWrapper &wire, Bits expected_value) {
        dut->eval();
        expected_value.set_width(wire.get_width());
        std::cout << "EXPECT AT " << m_tickcount << "\t" << wire.get_name();

        Bits actual_value = wire.get_value();
        std::cout << " got " << actual_value << " expected " << expected_value;

        if (actual_value == expected_value) {
            std::cout << " PASS" << std::endl;
        } else {
            std::cout << " FAIL" << std::endl;
            if (!failed) {
                failed = true;
                first_failed_cycle = m_tickcount;
            }
        }
    }

    // calls expect on each value and the bundle element that corresponds to
    // its key.
    void expect(VerilatorBundle &bundle, std::map<std::string, Bits> &values);

    // calls expect on each vector element and its coresponding value in values
    template<class Data>
    void expect(VerilatorVec<Data> &vec, std::vector<Bits> &values) {
        for (size_t i = 0; i < vec.get_num_elements(); i++)
            expect(vec[i], values[i]);
    }

    virtual bool done() {
        return (Verilated::gotFinish());
    }

    virtual void finish() {
        std::cout << "RAN " << m_tickcount << " CYCLES ";
        if (failed)
            std::cout << "FAILED FIRST AT CYCLE " << first_failed_cycle << std::endl;
        else
            std::cout << "PASSED" << std::endl;
    }

    // actual implementation of testbench containing all peeks/pokes/expects
    virtual void run() = 0;
};

// used by double sc_time_stamp() function that is required by verilator
// main_time is equal to the cycle count
template<class Module>
vluint64_t Testbench<Module>::main_time = 0;

#endif
