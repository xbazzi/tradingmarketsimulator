#include <array>
#include <chrono>
#include <iostream>
#include <thread>
#include <cstdlib>

#include <fiah/handle/UniquePtr.hh>
#include <fiah/io/Udp.hh>
#include <fiah/utils/Timer.hh>

#include "mms/market_data/MarketDepthGenerator.hh"
#include "mms/structs/Structs.hh"

int main()
{
    using namespace std::chrono_literals;

    fiah::UdpClient consumer_endpoint{"127.0.0.1", 9001};
    std::array<fiah::UdpClient, 1> endpoints{std::move(consumer_endpoint)};

    auto generator = fiah::make_unique<mms::MarketDepthGenerator>(
        fiah::UdpServer{"127.0.0.1", 9000},
        std::span<fiah::UdpClient>{endpoints},
        500ms 
    );

    generator->start();
    std::cout << "Sending market depth messages to 127.0.0.1:9001" << std::endl;;

    return EXIT_SUCCESS;
}
