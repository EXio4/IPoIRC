#ifndef IPOIRC_MODEXISTS_H
#define IPOIRC_MODEXISTS_H


/* everything in this header is meant to encode the "sequence" forced
 * by the existential types in the "modules" (emulated with virtual inheritance)
 * in modules.h
 */
#include "modules.h"
#include <memory>

namespace EX {


    template <typename T>
    class Local_M : public virtual CoreModule, public virtual LogModule {
    public:
        const LocalModule<T> *l;
        Local_M(const LocalModule<T> *_l) : l(_l) {};
    public:
        std::string module_name() const noexcept {
            return l->module_name();
        }
        HelpText help() const noexcept {
            return l->help();
        }
        std::ostream& log(Log::Level v) const {
            return l->log(v);
        }
    };

    class Local_Config;
    class Local_PrivInit;
    class Local_NormInit;
    class Local_Start;
    class Local_Done;

    class Local_Config : public virtual CoreModule, public virtual LogModule {
    public:
        virtual std::shared_ptr<Local_PrivInit> config(sol::table) = 0;
    };

    class Local_PrivInit : public virtual CoreModule, public virtual LogModule {
    public:
        virtual std::shared_ptr<Local_NormInit> priv_init() = 0;
    };

    class Local_NormInit : public virtual CoreModule, public virtual LogModule {
    public:
        virtual std::shared_ptr<Local_Start> norm_init() = 0;
    };

    class Local_Start : public virtual CoreModule, public virtual LogModule {
    public:
        virtual std::shared_ptr<Local_Done> start_th() = 0;
    };

    class Local_Done : public virtual CoreModule, public virtual LogModule {
    public:
        virtual void worker_writer(Comm::Socket) = 0;
        virtual void worker_reader(Comm::Socket) = 0;
    };

    template <typename T>
    class Local_Done_Impl : public Local_Done, public Local_M<T> {
    private:
        const typename T::State s;
    public:
        Local_Done_Impl(const LocalModule<T>* _l, const typename T::State _s) : Local_M<T>(_l) , s(_s) {};
        void worker_writer(Comm::Socket k) {
            return Local_M<T>::l->worker_writer(s, k);
        }
        void worker_reader(Comm::Socket k) {
            return Local_M<T>::l->worker_reader(s, k);
        }
    };
    template <typename T>
    class Local_Start_Impl : public Local_Start, public Local_M<T> {
    private:
        const typename T::Priv p;
        const typename T::Norm n;
    public:
        Local_Start_Impl(const LocalModule<T>* _l, const typename T::Priv _p, const typename T::Norm _n) : Local_M<T>(_l), p(_p), n(_n) {};
        std::shared_ptr<Local_Done> start_th() {
            return std::shared_ptr<Local_Done>(new Local_Done_Impl<T>(Local_M<T>::l, Local_M<T>::l->start_thread(p, n)));
        }
    };

    template <typename T>
    class Local_NormInit_Impl : public Local_NormInit, public Local_M<T> {
    private:
        const typename T::Config c;
        const typename T::Priv   p;
    public:
        Local_NormInit_Impl(const LocalModule<T>* _l, const typename T::Config _c, const typename T::Priv _p) : Local_M<T>(_l), c(_c), p(_p) {};
        std::shared_ptr<Local_Start> norm_init() {
            return std::shared_ptr<Local_Start>(new Local_Start_Impl<T>(Local_M<T>::l, p, Local_M<T>::l->norm_init(c)));
        }
    };
    template <typename T>
    class Local_PrivInit_Impl : public Local_PrivInit, public Local_M<T> {
    private:
        const typename T::Config c;
    public:
        Local_PrivInit_Impl(const LocalModule<T>* _l, const typename T::Config _c) : Local_M<T>(_l), c(_c) {};
        std::shared_ptr<Local_NormInit> priv_init() {
            return std::shared_ptr<Local_NormInit>(new Local_NormInit_Impl<T>(Local_M<T>::l, c, Local_M<T>::l->priv_init(c)));
        }
    };

    template <typename T>
    class Local_Config_Impl : public Local_Config, public Local_M<T> {
    public:
        Local_Config_Impl(const LocalModule<T>* _l) : Local_M<T>(_l) {};
        std::shared_ptr<Local_PrivInit> config(sol::table e) {
            return std::shared_ptr<Local_PrivInit>(new Local_PrivInit_Impl<T>(Local_M<T>::l, Local_M<T>::l->config(e)));
        }
    };

    template <typename T>
    std::shared_ptr<Local_Config> local_module(LocalModule<T>* l) {
        return std::shared_ptr<Local_Config>(new Local_Config_Impl<T>(l));
    }

}

#endif