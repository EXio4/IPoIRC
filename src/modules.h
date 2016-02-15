#ifndef IPOIRC_MODULES_H
#define IPOIRC_MODULES_H
#include <boost/optional.hpp>
#include <sol/sol.hpp>
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

template <typename T>
class LocalModule : public CoreModule {
public:
    using Config = typename T::Config;
    using Priv   = typename T::Priv  ;
    using Norm   = typename T::Norm  ;
    using State  = typename T::State ;
    virtual Config config(sol::table) = 0;

    virtual Priv priv_init(Config) = 0;
    virtual Norm norm_init(Config) = 0;

    virtual State start_thread(Priv&, Norm&) = 0;
    virtual void worker_reader(State, Comm::Socket) = 0;
    virtual void worker_writer(State, Comm::Socket) = 0;
};

struct Unit {};

#endif