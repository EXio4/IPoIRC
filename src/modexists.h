#ifndef IPOIRC_MODEXISTS_H
#define IPOIRC_MODEXISTS_H


/* everything in this header is meant to encode the "sequence" forced
 * by the existential types in the "modules" (emulated with virtual inheritance)
 * in modules.h
 */
#include "modules.h"
#include <memory>

namespace EX {


    class Local_Config;
    class Local_PrivInit;
    class Local_NormInit;
    class Local_Start;
    class Local_Done;

    class Local_Config : public virtual CoreModule, public LogModule {
    public:
        virtual std::shared_ptr<Local_PrivInit> config(sol::table) = 0;
    };

    class Local_PrivInit : public virtual CoreModule, public LogModule {
    public:
        virtual std::shared_ptr<Local_NormInit> priv_init() = 0;
    };

    class Local_NormInit : public virtual CoreModule, public LogModule {
    public:
        virtual std::shared_ptr<Local_Start> norm_init() = 0;
    };

    class Local_Start : public virtual CoreModule, public LogModule {
    public:
        virtual std::shared_ptr<Local_Done> start_th() = 0;
    };

    class Local_Done : public virtual CoreModule, public LogModule {
    public:
        virtual void worker_writer(Comm::Socket) = 0;
        virtual void worker_reader(Comm::Socket) = 0;
    };

    template <typename T>
    class Local_Done_Impl : public Local_Done {
    private:
        LocalModule<T> *l;
        typename T::State s;
    public:
        Local_Done_Impl(LocalModule<T>* _l, typename T::State _s) : l(_l) , s(_s) {};
        void worker_writer(Comm::Socket k) {
            return l->worker_writer(s, k);
        }
        void worker_reader(Comm::Socket k) {
            return l->worker_reader(s, k);
        }

        /* BOILERPLATE */
        std::string module_name() noexcept {
            return l->module_name();
        }
        HelpText help() noexcept {
            return l->help();
        }
        std::ostream& log(Log::Level v) {
            return l->log(v);
        }

    };
    template <typename T>
    class Local_Start_Impl : public Local_Start {
    private:
        LocalModule<T> *l;
        typename T::Priv p;
        typename T::Norm n;
    public:
        Local_Start_Impl(LocalModule<T>* _l, typename T::Priv _p, typename T::Norm _n) : l(_l), p(_p), n(_n) {};
        std::shared_ptr<Local_Done> start_th() {
            return std::shared_ptr<Local_Done>(new Local_Done_Impl<T>(l, l->start_thread(p, n)));
        }

        /* BOILERPLATE */
        std::string module_name() noexcept {
            return l->module_name();
        }
        HelpText help() noexcept {
            return l->help();
        }
        std::ostream& log(Log::Level v) {
            return l->log(v);
        }

    };
    template <typename T>
    class Local_NormInit_Impl : public Local_NormInit {
    private:
        LocalModule<T> *l;
        typename T::Config c;
        typename T::Priv   p;
    public:
        Local_NormInit_Impl(LocalModule<T>* _l, typename T::Config _c, typename T::Priv _p) : l(_l), c(_c), p(_p) {};
        std::shared_ptr<Local_Start> norm_init() {
            return std::shared_ptr<Local_Start>(new Local_Start_Impl<T>(l, p, l->norm_init(c)));
        }

        /* BOILERPLATE */
        std::string module_name() noexcept {
            return l->module_name();
        }
        HelpText help() noexcept {
            return l->help();
        }
        std::ostream& log(Log::Level v) {
            return l->log(v);
        }

    };
    template <typename T>
    class Local_PrivInit_Impl : public Local_PrivInit {
    private:
        LocalModule<T> *l;
        typename T::Config c;
    public:
        Local_PrivInit_Impl(LocalModule<T>* _l, typename T::Config _c) : l(_l), c(_c) {};
        std::shared_ptr<Local_NormInit> priv_init() {
            return std::shared_ptr<Local_NormInit>(new Local_NormInit_Impl<T>(l, c, l->priv_init(c)));
        }

        /* BOILERPLATE */
        std::string module_name() noexcept {
            return l->module_name();
        }
        HelpText help() noexcept {
            return l->help();
        }
        std::ostream& log(Log::Level v) {
            return l->log(v);
        }

    };

    template <typename T>
    class Local_Config_Impl : public Local_Config {
    private:
        LocalModule<T> *l;
    public:
        Local_Config_Impl(LocalModule<T>* _l) : l(_l) {};
        std::shared_ptr<Local_PrivInit> config(sol::table e) {
            return std::shared_ptr<Local_PrivInit>(new Local_PrivInit_Impl<T>(l, l->config(e)));
        }

        /* BOILERPLATE */
        std::string module_name() noexcept {
            return l->module_name();
        }
        HelpText help() noexcept {
            return l->help();
        }
        std::ostream& log(Log::Level v) {
            return l->log(v);
        }

    };

    template <typename T>
    std::shared_ptr<Local_Config> local_module(LocalModule<T>* l) {
        return std::shared_ptr<Local_Config>(new Local_Config_Impl<T>(l));
    }

}

#endif