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
#include "mms/exchange/OptionOrderGenerator.hh"
#include "mms/exchange/OptionOrderBook.hh"

int main()
{
    using namespace std::chrono_literals;
    using namespace mms;

    fiah::UdpServer local_server{"127.0.0.1", 9000};
    fiah::UdpClient consumer_endpoint{"127.0.0.1", 9001};
    std::array<fiah::UdpClient, 1> endpoints{std::move(consumer_endpoint)};


    Option opt{};
    opt.strike = Price{10000};
    std::memcpy(opt.symbol.data, "SPCX", 4);
    opt.year = 26; opt.month = 6; opt.day = 20;
    opt.flags = std::to_underlying(Option::Flags::IsCall);

    OptionOrderGeneratorConfig cfg
    {
        Price{2000},               // mid
        Price{600},                // spread_half_width
        Price{400},                // aggressor_offset
        10u,                       // mm_lot_max
        20u,                       // aggressor_lot_max
        0.20f,                     // aggressor_weight
        0.0f,                      // cancel_weight
        50ms
    };

    OptionOrderBook<10, "SPCX", true> order_book{};

    OptionOrderGenerator order_gen
    {
        [&order_book](Order o) { return order_book.add(o); },
        [&order_book](std::uint32_t id) { return order_book.remove(id); },
        opt,
        cfg
    };
    order_book.set_fill_fn([&order_gen](std::uint32_t id) { order_gen.notify_fill(id); });


    std::cout << "Sending market depth messages to 127.0.0.1:9001" << std::endl;
    order_gen.start();
    std::this_thread::sleep_until(std::chrono::steady_clock::time_point::max());

    return EXIT_SUCCESS;
}
