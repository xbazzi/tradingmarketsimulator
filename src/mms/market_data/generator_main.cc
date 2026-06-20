#include <array>
#include <chrono>
#include <iostream>
#include <thread>

#include <fiah/handle/UniquePtr.hh>
#include <fiah/io/Udp.hh>
#include <fiah/utils/Timer.hh>

#include "mms/market_data/MarketDepthGenerator.hh"
#include "mms/structs/Structs.hh"

int main()
{
    fiah::UdpClient consumer_endpoint{"127.0.0.1", 9001};
    std::array<fiah::UdpClient, 1> endpoints{std::move(consumer_endpoint)};

    auto generator = fiah::make_unique<mms::MarketDataGenerator>(
        fiah::UdpServer{"127.0.0.1", 9000},
        std::span<fiah::UdpClient>{endpoints}
    );

    if (!generator->setup()) {
        std::cerr << "Failed to setup generator" << std::endl;;
        return 1;
    }

    std::cout << "Sending market depth messages to 127.0.0.1:9001" << std::endl;;

    std::uint16_t seq = 0;
    fiah::TimeStamp ts;

    while (true) {
        mms::Option opt = generator->generate_option();
        mms::MarketDepthMessage msg = generator->generate_depth_message(ts, opt);

        msg.seq = seq++;
        generator->enqueue(msg);
        generator->send_depth_message();

        char arr[]{'A', 'G', 'H', 'E', '\0'};
        std::cout << "Sent seq=" << msg.seq << " symbol=" << msg.option.symbol.data << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
