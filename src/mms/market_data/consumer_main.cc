#include <chrono>
#include <print>
#include <thread>
#include <cstdio>

#include <fiah/io/Udp.hh>

#include "mms/market_data/MarketDepthConsumer.hh"
#include "mms/structs/Structs.hh"

int main()
{
    using namespace mms;
    fiah::UdpClient client{"127.0.0.1", 9001};

    if (!client.bind_local(9001)) {
        std::print(stderr, "Failed to bind to port 9001\n");
        return 1;
    }

    MarketDepthConsumer consumer{std::move(client)};

    std::print("Listening for market depth messages on port 9001...\n");

    InternalDepthMessage msg{};
    fiah::TimeStamp<>::rep t_last{};
    while (true) {
        if (consumer.try_pop(msg))
        {
            std::print("[{:<6}]Received: {}{:2d}{:02d}{:02d}{}{:<9.2f} {}@{:0.4f} seq={}, market_ts={}, t_local - t_market={:<10.4f}us, t - t_last={}us\n",
                msg.local_ts,
                msg.option.symbol.data, msg.option.year, msg.option.month, msg.option.day,
                (msg.option.flags bitand std::to_underlying(Flags::IsCall))? "C": "P", msg.option.strike.tof(), 
                (msg.flags & std::to_underlying(InternalDepthMessage::Flags::IsBid))? "bid": "ask",
                msg.price.tof(),
                msg.seq,
                msg.market_ts_ns, fiah::TimeStamp<>::to_micro(msg.local_ts - msg.market_ts_ns), fiah::TimeStamp<>::to_micro(msg.local_ts - t_last)
            );
            t_last = msg.local_ts;
        }
        else
            std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    return 0;
}
