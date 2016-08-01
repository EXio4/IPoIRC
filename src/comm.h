#ifndef IPOIRC_COMM_H
#define IPOIRC_COMM_H
#include <string>
#include <memory>
#include <vector>

namespace Comm {
    enum SocketSide {
            Net2Chat,
            Chat2Net
    };

    class SocketInt {
    public:
        virtual std::vector<uint8_t> recv() = 0;
        virtual bool send(const std::vector<uint8_t>&) = 0;
    };
    using Socket = std::shared_ptr<SocketInt>;

    class CtxInt {
    public:
        virtual Socket connect(SocketSide) = 0;
    };
    using Ctx = std::shared_ptr<CtxInt>;



}
#endif