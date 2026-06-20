#include <chrono>
#include <iostream>
#include <thread>

#include <fiah/io/Udp.hh>

#include "mms/market_data/MarketDepthConsumer.hh"
#include "mms/structs/Structs.hh"

int main()
{
    fiah::UdpClient client{"127.0.0.1", 9001};

    if (!client.bind_local(9001)) {
        std::cerr << "Failed to bind to port 9001" << std::endl;;
        return 1;
    }

    mms::MarketDepthConsumer consumer{std::move(client)};

    std::cout << "Listening for market depth messages on port 9001..." << std::endl;;

    mms::MarketDepthMessage msg{};
    while (true) {
        if (consumer.try_pop(msg))
            std::cout << "Received seq=" << msg.seq << " symbol=" << msg.option.symbol.data
                      << " Strike=" << msg.option.strike << " Side=" << (msg.is_bid? "bid": "ask")
                      << std::endl;
        else
            std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    return 0;
}
