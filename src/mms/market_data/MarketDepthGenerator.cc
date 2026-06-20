// MarketMakerSimulator Includes
#include <cstring>
#include <random>

#include "mms/market_data/MarketDepthGenerator.hh"
#include "mms/structs/Structs.hh"

#include <fiah/utils/XorBitant.hh>

namespace mms
{
std::expected<void, Error> MarketDataGenerator::setup() noexcept
{
    if (!m_udp.start())
        return std::unexpected(Error::INIT_ERROR);
    return {};
}

MarketDepthMessage MarketDataGenerator::generate_depth_message(fiah::TimeStamp<> ts, Option option) const noexcept
{
    fiah::XorBitant xor_gen{std::random_device{}()};
    std::uniform_int_distribution price_dist{30'000, 40'000};
    std::uniform_int_distribution side_dist{0, 1};
    ts.update_now();
    return {option, ts.get_ticks(), Price{static_cast<Price::type>(price_dist(xor_gen))}, 0, side_dist(xor_gen)};
}


void MarketDataGenerator::send_depth_message() noexcept
{
    MarketDepthMessage msg;
    if (!m_send_q.pop(msg))
        return;

    std::byte buf[DEPTH_MSG_LEN];
    std::memcpy(buf, &msg, DEPTH_MSG_LEN);

    for (auto i{0uz}; i < m_clients.size(); ++i)
        m_udp.send(buf, DEPTH_MSG_LEN, m_clients[i].get_sockaddr_in());
}

Option MarketDataGenerator::generate_option() const
{
    fiah::XorBitant xor_gen{std::random_device{}()};
    std::uniform_int_distribution symbol_dist{0, static_cast<int>(m_symbols.size() - 1)};
    std::uniform_int_distribution strike_dist{0, static_cast<int>(m_strikes.size() - 1)};
    return {m_symbols[symbol_dist(xor_gen)], m_strikes[strike_dist(xor_gen)], 26, 6, 19};
}

void MarketDataGenerator::enqueue(const MarketDepthMessage& msg) noexcept
{
    m_send_q.push(msg);
}

} // End namespace mms