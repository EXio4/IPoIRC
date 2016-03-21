#ifndef IPOIRC_COMM_H
#define IPOIRC_COMM_H
#include <string>
namespace Comm {
/*
    class Socket {
        virtual std::vector<uint8_t> recv();
        virtual void write(std::vector<uint8_t>);
    }
*/

    using Socket = void*;

    using Ctx = void*;

    // ??
    enum Host {
        LocalLibrary,
        ChatLibrary
    }

    class Context {
        virtual std::unique_ptr<Socket> connect(Host);
    }
    Socket connect(Ctx, const std::string&);

}
#endif