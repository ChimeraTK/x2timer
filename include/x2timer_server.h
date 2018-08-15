/*
 * x2timer_server.h
 *
 *  Created on: Nov 8, 2016
 *      Author: Martin Hierholzer
 */

#ifndef X2TIMER_SERVER_H
#define X2TIMER_SERVER_H

#include <iostream>

#undef GENERATE_XML
#include <ChimeraTK/ApplicationCore/ApplicationCore.h>

#include <x2timer.h>

namespace ctk = ChimeraTK;

struct X2Outputs : public ctk::ApplicationModule {
    X2Outputs() {}
    X2Outputs(EntityOwner *owner, X2Timer *x2timer, const std::string &name, const std::string &description,
              bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

    void mainLoop() override;

    /** Define map types which can be put into the TemplateUserTypeMap */
    template<typename T>
    using MapOfArrayOutput = std::map<D_fct*, ctk::ArrayOutput<T>>;

    /** Type-depending maps of arrays */
    mtca4u::TemplateUserTypeMap<MapOfArrayOutput> outputs;

    /** x2timer SDK object */
    X2Timer *x2timer;

};

struct X2Inputs : public ctk::ApplicationModule {
    X2Inputs() {}
    X2Inputs(EntityOwner *owner, X2Timer *x2timer, const std::string &name, const std::string &description,
             bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

    void mainLoop() override;

    void updateProperty(ctk::TransferElementID id);

    /** Define map types which can be put into the TemplateUserTypeMap */
    template<typename T>
    using VectorOfArrayInput = std::vector<ctk::ArrayPushInput<T>>;

    template<typename T>
    using MapOfArrayInput = std::map<ctk::TransferElementID, ctk::ArrayPushInput<T>*>;

    /** Type-depending maps of arrays */
    mtca4u::TemplateUserTypeMap<VectorOfArrayInput> inputs;

    /** Map of accessor IDs to properties */
    std::map<ctk::TransferElementID, D_fct*> propertyMap;

    /** Map of accessor IDs to accessors */
    mtca4u::TemplateUserTypeMap<MapOfArrayInput> accessorMap;

    /** x2timer SDK object */
    X2Timer *x2timer;

};

struct X2Interrupt : public ctk::ApplicationModule {
    X2Interrupt() {}
    X2Interrupt(EntityOwner *owner, X2Timer *x2timer, const std::string &name, const std::string &description,
                bool eliminateHierarchy=false, const std::unordered_set<std::string> &tags={});

    ctk::ScalarOutput<int32_t> macroPulseNumber{this, "MACRO_PULSE_NUMBER", "",
                           "The macro pulse number, updated when the IRQ is received (can be used as a trigger)."};

    void mainLoop() override;

    X2Timer *x2timer;

};


struct x2timer_server : public ctk::Application {
    x2timer_server() : Application("x2timer_server") {}
    ~x2timer_server() { shutdown(); }

    X2Outputs x2outputs;
    X2Inputs x2inputs;
    X2Interrupt x2interrupt;

    ctk::ControlSystemModule cs;

    void defineConnections();

    X2Timer *x2timer;
};

#endif // LLRFSERVER_H
