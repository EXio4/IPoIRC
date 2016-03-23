#ifndef IPOIRC_MODULES_H
#define IPOIRC_MODULES_H
#include <boost/optional.hpp>
#include <sol/sol.hpp>
#include "log.h"
#include "comm.h"

using HelpText = std::vector<std::vector<std::string>>; // we gotta now a better way to do this

class InitExc {
    virtual std::string err() = 0;
};

class CoreModule {
public:
    virtual std::string module_name() const noexcept = 0;
    virtual HelpText help() const noexcept = 0;
};

class LogModule : public virtual CoreModule {
public:
    virtual std::ostream& log(Log::Level l) const {
        return Log::gen(module_name(), l);     // default implementation should be good enough most of them time?
    }
};

template <typename T>
class LocalModule : public virtual CoreModule, public LogModule {
public:
    using Config = typename T::Config;
    using Priv   = typename T::Priv  ;
    using Norm   = typename T::Norm  ;
    using State  = typename T::State ;
    virtual Config config(sol::table) const = 0;

    virtual Priv priv_init(Config) const = 0;
    virtual Norm norm_init(Config) const = 0;

    virtual State start_thread(Priv, Norm) const = 0;
    virtual void worker_reader(State, Comm::Socket) const = 0;
    virtual void worker_writer(State, Comm::Socket) const = 0;
};

struct Unit {};

#endif