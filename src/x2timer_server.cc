/*
 * x2timer_server.cc
 *
 *  Created on: Aug 15, 2018
 *      Author: Martin Hierholzer
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <ChimeraTK/ApplicationCore/ApplicationCore.h>

#include "x2timer_server.h"

#include <eq_types.h>
#include <D_intern.h>

namespace ctk = ChimeraTK;

// Make sure SIGUSR1 signals are blocked before main() starts
struct SignalBlocker {
    SignalBlocker() {
      sigset_t                 mask;
      sigemptyset (&mask);
      sigaddset (&mask, SIGUSR1);
      pthread_sigmask (SIG_BLOCK, &mask, nullptr);
    }
};
static SignalBlocker signalBlocker;


void x2timer_server::defineConnections() {

    // STEP.1
    // initialize the library

    x2timer_init("TEST", "X2TIMER");

    // STEP.2
    // create a x2timer object and initialise

    x2timer = new X2Timer("MASTER");
    x2timer->init();

    // STEP.3
    // create a signal processing and update modules

    x2outputs = X2Outputs(this, x2timer, "x2outputs", "x2timer module");
    x2inputs= X2Inputs(this, x2timer, "x2inputs", "x2timer module");
    x2interrupt = X2Interrupt(this, x2timer, "X2Interrupt", "x2timer modules");

    x2outputs.connectTo(cs);
    x2inputs.connectTo(cs);
    x2interrupt.connectTo(cs);

}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

X2Outputs::X2Outputs(EntityOwner *owner, X2Timer *x2timer_, const std::string &name, const std::string &description,
       bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
: ctk::ApplicationModule(owner, name, description, eliminateHierarchy, tags),
  x2timer(x2timer_)
{
    EqData  src, dst;

    // get list of all properties
    std::vector<std::string> names;
    std::vector<int>         types;
    std::vector<int>         attrs;
    x2timer->names(names, attrs, types);
    assert(names.size() == attrs.size());
    assert(names.size() == types.size());

    // iterate list and create process variables
    for(size_t i=0; i<names.size(); ++i) {

      // ignore MACRO_PULSE_NUMBER, as this is handled by the interrupt module
      if(names[i] == "MACRO_PULSE_NUMBER") continue;

      // obtain property
      D_fct *prop = x2timer->property(names[i]);
      if( dynamic_cast<D_locname*>(prop) != nullptr || dynamic_cast<D_ecall*>(prop) != nullptr ||
          dynamic_cast<D_StsBit*>(prop) != nullptr                                                ) continue;
      prop->get(nullptr, &src, &dst, nullptr);
      size_t length = dst.length();

      // check if to generate output or input variable
      if(attrs[i] == 0) {   // read-only -> make output
        // int32_t types
        if(types[i] == DATA_INT || types[i] == DATA_A_INT || types[i] == DATA_IIII || types[i] == DATA_BOOL) {
          boost::fusion::at_key<int32_t>(outputs.table)[prop] = ctk::ArrayOutput<int32_t>(this, names[i], "", length,
                                                                                          "Property "+names[i]);
        }
        // int64_t types
        else if(types[i] == DATA_A_LONG) {
          boost::fusion::at_key<int64_t>(outputs.table)[prop] = ctk::ArrayOutput<int64_t>(this, names[i], "", length,
                                                                                          "Property "+names[i]);
        }
        // float types
        else if(types[i] == DATA_FLOAT || types[i] == DATA_A_FLOAT) {
          boost::fusion::at_key<float>(outputs.table)[prop] = ctk::ArrayOutput<float>(this, names[i], "", length,
                                                                                          "Property "+names[i]);
        }
        // double types
        else if(types[i] == DATA_DOUBLE || types[i] == DATA_A_DOUBLE) {
          boost::fusion::at_key<double>(outputs.table)[prop] = ctk::ArrayOutput<double>(this, names[i], "", length,
                                                                                          "Property "+names[i]);
        }
        // string types
        else if(types[i] == DATA_STRING || types[i] == DATA_TEXT) {
          // The x2timer SDK reports the string length, not the array lengths in dst.length(). Strings are always scalar!
          boost::fusion::at_key<std::string>(outputs.table)[prop] = ctk::ArrayOutput<std::string>(this, names[i], "", 1,
                                                                                          "Property "+names[i]);
        }
        else {
          std::cout << "WARNING: Unhandled data type for output variable: " << names[i] << " type = " << types[i] << std::endl;
        }
      }
      else if(attrs[i] == 1) {   // read-write -> make input (not handled in this module)
      }
      else {
        std::cout << "ERROR: Unknown access mode for variable: " << names[i] << " mode = " << attrs[i] << std::endl;
        exit(1);
      }
    }
}

/**********************************************************************************************************************/

template<typename T>
bool DataAssignOutput(ctk::ArrayOutput<T> &output, EqData &) {
    std::cout << " ERROR in DataAssignOutput - Type not supported: " << typeid(T).name() << " for variable " << output.getName() << std::endl;
    exit(1);
}

template<>
bool DataAssignOutput<int>(ctk::ArrayOutput<int> &output, EqData &data) {
    if(output.getNElements() == 1) {
      if(output[0] == data.get_int()) return false;
      output[0] = data.get_int();
    }
    else {
      auto *ptrToArray = data.get_int_array();
      if(ptrToArray == nullptr) {
        //std::cout << "Could not get data for variable: " << output.getName() << std::endl;
        return false;
      }
      auto vector = std::vector<int>(ptrToArray, ptrToArray+output.getNElements());
      //if(output == vector) return false;
      output = vector;
    }
    return true;
}

template<>
bool DataAssignOutput<float>(ctk::ArrayOutput<float> &output, EqData &data) {
    if(output.getNElements() == 1) {
      output[0] = data.get_float();
      if(output[0] == data.get_float()) return false;
    }
    else {
      auto *ptrToArray = data.get_float_array();
      if(ptrToArray == nullptr) {
        //std::cout << "Could not get data for variable: " << output.getName() << std::endl;
        return false;
      }
      output = std::vector<float>(ptrToArray, ptrToArray+output.getNElements());
    }
    return true;
}

template<>
bool DataAssignOutput<double>(ctk::ArrayOutput<double> &output, EqData &data) {
    if(output.getNElements() == 1) {
      output[0] = data.get_double();
      if(output[0] == data.get_double()) return false;
    }
    else {
      auto *ptrToArray = data.get_double_array();
      if(ptrToArray == nullptr) {
        //std::cout << "Could not get data for variable: " << output.getName() << std::endl;
        return false;
      }
      output = std::vector<double>(ptrToArray, ptrToArray+output.getNElements());
    }
    return true;
}

template<>
bool DataAssignOutput<std::string>(ctk::ArrayOutput<std::string> &output, EqData &data) {
    assert(output.getNElements() == 1);
    if(output[0] == data.get_string()) return false;
    output[0] = data.get_string();
    return true;
}


/**********************************************************************************************************************/

struct FunctorUpdate {
    FunctorUpdate(X2Outputs *that_) : that(that_) {}

    template<typename PAIR>
    void operator()(PAIR &pair) const {
      auto &map = pair.second;
      EqData  src, dst;

      for(auto &mapPair : map) {
        mapPair.first->get(nullptr, &src, &dst, nullptr);

        // verify array length (strings are not arrays, dst.length() reports the string length)
        if(dst.type() != DATA_STRING && dst.type() != DATA_TEXT &&
           static_cast<unsigned int>(dst.length()) != mapPair.second.getNElements()) {
          /* std::cout << "Length of variable " << mapPair.second.getName() << " changed! Was: " <<
                       mapPair.second.getNElements() << " is now: " << dst.length() << std::endl; */
          continue;
        }

        // copy data to accessor
        bool updated = DataAssignOutput(mapPair.second, dst);

        // write value if updated
        if(updated) mapPair.second.write();
      }
    }

    X2Outputs *that;
};

/**********************************************************************************************************************/

void X2Outputs::mainLoop() {

    while(true) {
      // wait one second
      sleep(1);

      // call update routine
      x2timer->update();

      // update all outputs
      boost::fusion::for_each( outputs.table, FunctorUpdate(this) );

    }
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

X2Inputs::X2Inputs(EntityOwner *owner, X2Timer *x2timer_, const std::string &name, const std::string &description,
       bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
: ctk::ApplicationModule(owner, name, description, eliminateHierarchy, tags),
  x2timer(x2timer_)
{
    EqData  src, dst;

    // get list of all properties
    std::vector<std::string> names;
    std::vector<int>         types;
    std::vector<int>         attrs;
    x2timer->names(names, attrs, types);
    assert(names.size() == attrs.size());
    assert(names.size() == types.size());

    // iterate list and create process variables
    for(size_t i=0; i<names.size(); ++i) {

      // obtain property
      D_fct *prop = x2timer->property(names[i]);
      if( dynamic_cast<D_locname*>(prop) != nullptr || dynamic_cast<D_ecall*>(prop) != nullptr ||
          dynamic_cast<D_StsBit*>(prop) != nullptr                                                ) continue;
      prop->get(nullptr, &src, &dst, nullptr);
      size_t length = dst.length();

      // check if to generate output or input variable
      if(attrs[i] == 0) {   // read-only -> make output (not handled in this module)
      }
      else if(attrs[i] == 1) {   // read-write -> make input
        // int32_t types
        if(types[i] == DATA_INT || types[i] == DATA_A_INT || types[i] == DATA_IIII || types[i] == DATA_BOOL) {
          boost::fusion::at_key<int32_t>(inputs.table).emplace_back(this, names[i], "", length, "Property "+names[i]);
        }
        // int64_t types
        else if(types[i] == DATA_A_LONG) {
          boost::fusion::at_key<int64_t>(inputs.table).emplace_back(this, names[i], "", length, "Property "+names[i]);
        }
        // float types
        else if(types[i] == DATA_FLOAT || types[i] == DATA_A_FLOAT) {
          boost::fusion::at_key<float>(inputs.table).emplace_back(this, names[i], "", length, "Property "+names[i]);
        }
        // double types
        else if(types[i] == DATA_DOUBLE || types[i] == DATA_A_DOUBLE) {
          boost::fusion::at_key<double>(inputs.table).emplace_back(this, names[i], "", length, "Property "+names[i]);
        }
        // string types
        else if(types[i] == DATA_STRING || types[i] == DATA_TEXT) {
          boost::fusion::at_key<std::string>(inputs.table).emplace_back(this, names[i], "", length, "Property "+names[i]);
        }
        else {
          std::cout << "WARNING: Unhandled data type for input variable: " << names[i] << " type = " << types[i] << std::endl;
        }
      }
      else {
        std::cout << "ERROR: Unknown access mode for variable: " << names[i] << " mode = " << attrs[i] << std::endl;
        exit(1);
      }
    }
}

/**********************************************************************************************************************/

struct FunctorFillMap {
    FunctorFillMap(X2Inputs *that_) : that(that_) {}

    template<typename PAIR>
    void operator()(PAIR &pair) const {
      typedef typename PAIR::first_type T;

      auto &map = pair.second;

      for(auto &input : map) {
        D_fct *prop = that->x2timer->property(input.getName().substr(1));   // remove leading slash
        assert(prop != nullptr);
        that->propertyMap[input.getId()] = prop;
        boost::fusion::at_key<T>(that->accessorMap.table)[input.getId()] = &input;
      }
    }

    X2Inputs *that;
};

/**********************************************************************************************************************/

void X2Inputs::updateProperty(ctk::TransferElementID id) {

    // get the property
    D_fct *prop = propertyMap[id];
    auto type = prop->data_type();

    // copy value to EqData structure
    EqData src;

    // int32_t types
    if(type == DATA_INT || type == DATA_A_INT || type == DATA_IIII || type == DATA_BOOL) {
      auto *acc = boost::fusion::at_key<int32_t>(accessorMap.table)[id];
      for(size_t i=0; i<acc->getNElements(); ++i) src.set((*acc)[i], static_cast<int>(i));
    }
    // int64_t types
    else if(type == DATA_A_LONG) {
      auto *acc = boost::fusion::at_key<int64_t>(accessorMap.table)[id];
      for(size_t i=0; i<acc->getNElements(); ++i) src.set(static_cast<long long int>((*acc)[i]), static_cast<int>(i));
    }
    // float types
    else if(type == DATA_FLOAT || type == DATA_A_FLOAT) {
      auto *acc = boost::fusion::at_key<float>(accessorMap.table)[id];
      for(size_t i=0; i<acc->getNElements(); ++i) src.set((*acc)[i], static_cast<int>(i));
    }
    // double types
    else if(type == DATA_DOUBLE || type == DATA_A_DOUBLE) {
      auto *acc = boost::fusion::at_key<double>(accessorMap.table)[id];
      for(size_t i=0; i<acc->getNElements(); ++i) src.set((*acc)[i], static_cast<int>(i));
    }
    // string types
    else if(type == DATA_STRING || type == DATA_TEXT) {
      src.set((*boost::fusion::at_key<std::string>(accessorMap.table)[id])[0]);
    }
    else {
      std::cout << "WARNING: Unhandled data type for input variable: " << type << std::endl;
    }

    // send value to the SDK
    EqData dst;
    prop->set(nullptr, &src, &dst, nullptr);

}

/**********************************************************************************************************************/

void X2Inputs::mainLoop() {
    auto group = readAnyGroup();

    // fill input map
    boost::fusion::for_each( inputs.table, FunctorFillMap(this) );

    // update all properties once with the initial values
    for(auto &pair : propertyMap) updateProperty(pair.first);

    while(true) {

      // wait for changed input
      auto id = group.readAny();

      // update the property
      updateProperty(id);


    }
}
/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

X2Interrupt::X2Interrupt(EntityOwner *owner, X2Timer *x2timer_, const std::string &name, const std::string &description,
       bool eliminateHierarchy, const std::unordered_set<std::string> &tags)
: ctk::ApplicationModule(owner, name, description, eliminateHierarchy, tags),
  x2timer(x2timer_)
{}

/**********************************************************************************************************************/

void X2Interrupt::mainLoop() {
    EqData src, dst;

    sigset_t            mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGUSR1);
    //pthread_sigmask (SIG_BLOCK, &mask, nullptr);

    auto p_macroPulseNumber = x2timer->property("MACRO_PULSE_NUMBER");

    while(true) {

      // wait for interrupt to be received from the kernel driver
      int sig;
      sigwait(&mask, &sig);
      if (sig != SIGUSR1) continue;

      // process interrupt by the library
      x2timer->interrupt();

      // update macro pulse number
      p_macroPulseNumber->get(nullptr, &src, &dst, nullptr);
      macroPulseNumber = dst.get_long(0);
      macroPulseNumber.write();

      std::cout << "USR1 " << macroPulseNumber << " " << dst.type_string() << std::endl;

    }
}
