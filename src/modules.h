#ifndef IPOIRC_MODULES_H
#define IPOIRC_MODULES_H
#include <boost/optional.hpp>
#include <yaml-cpp/yaml.h>
#include "comm.h"

using HelpText = std::vector<std::vector<std::string>>; // we gotta now a better way to do this

class InitExc {
    virtual std::string err() = 0;
};

class CoreModule {
public:
    virtual std::string module_name() noexcept = 0;
    virtual HelpText help() noexcept = 0;
};

template <typename CFG, typename C1, typename C2, typename T>
class LocalModule : public CoreModule {
public:
    virtual boost::optional<CFG> config(YAML::Node) noexcept = 0; /* missing parameters */

    virtual C1 priv_init  (CFG) = 0;
    virtual C2 normal_init(CFG) = 0;

    virtual T start_thread(C1&, C2&) = 0;
    virtual void worker_reader(T, Comm::Socket) = 0;
    virtual void worker_writer(T, Comm::Socket) = 0;
};

struct Unit {};

#endif