// written by Albert Chen

#ifndef __VERILATOR_AGGREGATE_API__
#define __VERILATOR_AGGREGATE_API__

#include "veri_api.h"
#include "testbench.h"
#include <vector>
#include <map>
#include <string>
#include <list>

static const char bundle_field_delim = '.';
static const char idx_field_delim = '_';

class VerilatorBundle : public VerilatorDataWrapper {
protected:
    std::map<std::string, VerilatorDataWrapper *> elements;
    std::list<VerilatorDataWrapper *> order;
    VerilatorDataWrapper *first_wire;

public:
    virtual VerilatorDataWrapper &operator[](std::string name) {
        if (elements.find(name) == elements.end()) {
            std::cout << "tried to access nonexistent bundle element: " << name << std::endl;
            assert(false);
        }

        return *elements.at(name);
    }

    Bits get_value() override {
        return first_wire->get_value();
    }

    void put_value(Bits &bits) override {
        return first_wire->put_value(bits);
    }

    size_t get_width() override {
        return first_wire->get_width();
    }

    std::string get_name() override {
        return first_wire->get_name();
    }

    template<class Module>
    friend std::map<std::string, Bits> Testbench<Module>::peek(VerilatorBundle&);
};

template<class Data>
class VerilatorVec : public VerilatorDataWrapper {
private:
    std::vector<Data> elements;

public:
    template<class ...Data1>
    explicit VerilatorVec(Data1... data): elements{data...} {
    }

    virtual Data &operator[](size_t idx) {
        return elements.at(idx);
    }

    Bits get_value() override {
        return elements.at(0).get_value();
    }

    void put_value(Bits &bits) override {
        return elements.at(0).put_value(bits);
    }

    size_t get_width() override {
        return elements.at(0).get_width();
    }

    std::string get_name() override {
        return elements.at(0).get_name();
    }

    size_t get_num_elements() {
        return elements.size();
    }

    template<class Module>
    template<class Data1>
    friend std::vector<Bits> Testbench<Module>::peek(VerilatorVec<Data1> &vec);

    template<class Module>
    friend void Testbench<Module>::poke(VerilatorBundle &bundle, std::map<std::string, Bits> &values);
};

template<class Module>
std::map<std::string, Bits> Testbench<Module>::peek(VerilatorBundle &bundle) {
    std::map<std::string, Bits> values;
    for (std::pair<std::string, VerilatorDataWrapper *> p : bundle.elements)
        values[p.first] = p.second->get_value();
    return values;
}

template<class Module>
template<class Data>
std::vector<Bits> Testbench<Module>::peek(VerilatorVec<Data> &vec) {
    std::vector<Bits> values;
    for (Data &p : vec.elements)
        values.push_back(p.get_value());
    return values;
}

template<class Module>
void Testbench<Module>::poke(VerilatorBundle &bundle, std::map<std::string, Bits> &values) {
    for (const std::pair<const std::string, Bits> &p : values) {
        std::stringstream ss(p.first);
        std::string sub_key;
        std::vector<std::string> sub_keys;
        while (std::getline(ss, sub_key, bundle_field_delim))
            sub_keys.push_back(sub_key);

        VerilatorDataWrapper *sub_bundle = &bundle;
        for (const std::string &key : sub_keys) {
            size_t idx_delim_pos = key.rfind(idx_field_delim);
            const std::string &idx_string = key.substr(idx_delim_pos + 1);
            if (idx_delim_pos != std::string::npos && idx_string.find_first_not_of("0123456789") == std::string::npos) {
                sub_bundle = &((*(VerilatorBundle *) sub_bundle)[key.substr(0, idx_delim_pos)]);
                sub_bundle = &((*(VerilatorVec<VerilatorDataWrapper> *) sub_bundle)[std::stoi(idx_string)]);
            } else
                sub_bundle = &((*(VerilatorBundle *) sub_bundle)[key]);
        }

        poke(*sub_bundle, p.second);
    }
}

template<class Module>
void Testbench<Module>::expect(VerilatorBundle &bundle, std::map<std::string, Bits> &values) {
    for (const std::pair<const std::string, Bits> &p : values) {
        std::stringstream ss(p.first);
        std::string sub_key;
        std::vector<std::string> sub_keys;
        while (std::getline(ss, sub_key, bundle_field_delim))
            sub_keys.push_back(sub_key);

        VerilatorDataWrapper *sub_bundle = &bundle;
        for (const std::string &key : sub_keys) {
            size_t idx_delim_pos = key.rfind(idx_field_delim);
            const std::string &idx_string = key.substr(idx_delim_pos + 1);
            if (idx_delim_pos != std::string::npos && idx_string.find_first_not_of("0123456789") == std::string::npos) {
                sub_bundle = &((*(VerilatorBundle *) sub_bundle)[key.substr(0, idx_delim_pos)]);
                sub_bundle = &((*(VerilatorVec<VerilatorDataWrapper> *) sub_bundle)[std::stoi(idx_string)]);
            } else
                sub_bundle = &((*(VerilatorBundle *) sub_bundle)[key]);
        }

        expect(*sub_bundle, p.second);
    }
}

#endif
