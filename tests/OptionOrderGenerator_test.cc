#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include "mms/exchange/OptionOrderGenerator.hh"
#include "mms/structs/Structs.hh"
#include "mms/market/Price.hh"

using namespace mms;

class OptionOrderGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_added.clear();
        m_removed.clear();

        OptionOrderGeneratorConfig cfg{
            Price{10000},   // mid_price  = 100.00
            Price{500},     // spread_half_width = 5.00
            Price{100},     // aggressor_offset = 1.00
            10u,            // mm_lot_max
            200u,           // aggressor_lot_max
            0.30f,          // aggressor_weight
            0.20f,          // cancel_weight
            std::chrono::nanoseconds{1}
        };

        Option opt{};
        opt.strike = Price{8000};
        std::memcpy(opt.symbol.data, "SPCX", 4);
        opt.year  = 26;
        opt.month = 6;
        opt.day   = 19;
        opt.flags = 1; // is_call

        m_gen = OptionOrderGenerator{
            [this](Order o) { m_added.push_back(o); return true; },
            [this](std::uint32_t id) { m_removed.push_back(id); return true; },
            opt,
            cfg
        };
        m_cfg = cfg;
    }

    std::vector<Order> m_added;
    std::vector<std::uint32_t> m_removed;
    OptionOrderGenerator m_gen;
    OptionOrderGeneratorConfig m_cfg{};
};

// --- generate_mm_order ---

TEST_F(OptionOrderGeneratorTest, MMBidIsBelowMid)
{
    const auto order = m_gen.generate_mm_order(Order::Side::Bid);
    EXPECT_EQ(order.side, Order::Side::Bid);
    const auto p = static_cast<Price::type>(order.price);
    EXPECT_LT(p, static_cast<Price::type>(m_cfg.mid_price));
    EXPECT_GE(p, static_cast<Price::type>(m_cfg.mid_price)
                 - static_cast<Price::type>(m_cfg.spread_half_width));
}

TEST_F(OptionOrderGeneratorTest, MMAskIsAboveMid)
{
    const auto order = m_gen.generate_mm_order(Order::Side::Ask);
    EXPECT_EQ(order.side, Order::Side::Ask);
    const auto p = static_cast<Price::type>(order.price);
    EXPECT_GT(p, static_cast<Price::type>(m_cfg.mid_price));
    EXPECT_LE(p, static_cast<Price::type>(m_cfg.mid_price)
                 + static_cast<Price::type>(m_cfg.spread_half_width));
}

TEST_F(OptionOrderGeneratorTest, MMQtyInRange)
{
    for (int i = 0; i < 50; ++i) 
    {
        const auto o = m_gen.generate_mm_order(Order::Side::Bid);
        EXPECT_GE(o.qty, 1u);
        EXPECT_LE(o.qty, m_cfg.mm_lot_max);
    }
}

TEST_F(OptionOrderGeneratorTest, MMOrderIdsAreUnique)
{
    const auto o1 = m_gen.generate_mm_order(Order::Side::Bid);
    const auto o2 = m_gen.generate_mm_order(Order::Side::Ask);
    EXPECT_NE(o1.id, o2.id);
}

// --- generate_aggressor_order ---

TEST_F(OptionOrderGeneratorTest, AggressorBidCrossesEmptyBookFallback)
{
    // No live orders — _best_ask() falls back to mid + spread_half_width.
    const auto fallback_ask = static_cast<Price::type>(m_cfg.mid_price)
                            + static_cast<Price::type>(m_cfg.spread_half_width);
    const auto order = m_gen.generate_aggressor_order(Order::Side::Bid);
    EXPECT_EQ(order.side, Order::Side::Bid);
    EXPECT_GE(static_cast<Price::type>(order.price),
              fallback_ask + static_cast<Price::type>(m_cfg.aggressor_offset));
}

TEST_F(OptionOrderGeneratorTest, AggressorAskCrossesEmptyBookFallback)
{
    const auto fallback_bid = static_cast<Price::type>(m_cfg.mid_price)
                            - static_cast<Price::type>(m_cfg.spread_half_width);
    const auto order = m_gen.generate_aggressor_order(Order::Side::Ask);
    EXPECT_EQ(order.side, Order::Side::Ask);
    EXPECT_LE(static_cast<Price::type>(order.price),
              fallback_bid - static_cast<Price::type>(m_cfg.aggressor_offset));
}

TEST_F(OptionOrderGeneratorTest, AggressorBidCrossesSeededBestAsk)
{
    // Seed a passive ask at 102.00 (Price{10200}).
    Order passive_ask{};
    passive_ask.side  = Order::Side::Ask;
    passive_ask.price = Price{10200};
    passive_ask.qty   = 5;
    passive_ask.id    = 9999u;
    m_gen.seed(passive_ask);

    const auto order = m_gen.generate_aggressor_order(Order::Side::Bid);
    EXPECT_GE(static_cast<Price::type>(order.price),
              10200 + static_cast<Price::type>(m_cfg.aggressor_offset));
}

TEST_F(OptionOrderGeneratorTest, AggressorAskCrossesSeededBestBid)
{
    Order passive_bid{};
    passive_bid.side  = Order::Side::Bid;
    passive_bid.price = Price{9800};
    passive_bid.qty   = 5;
    passive_bid.id    = 9998u;
    m_gen.seed(passive_bid);

    const auto order = m_gen.generate_aggressor_order(Order::Side::Ask);
    EXPECT_LE(static_cast<Price::type>(order.price),
              9800 - static_cast<Price::type>(m_cfg.aggressor_offset));
}

TEST_F(OptionOrderGeneratorTest, AggressorQtyInRange)
{
    for (int i = 0; i < 50; ++i) {
        const auto o = m_gen.generate_aggressor_order(Order::Side::Bid);
        EXPECT_GE(o.qty, 1u);
        EXPECT_LE(o.qty, m_cfg.aggressor_lot_max);
    }
}

// --- pick_cancel_id ---

TEST_F(OptionOrderGeneratorTest, PickCancelIdEmptyReturnsNullopt)
{
    EXPECT_EQ(m_gen.pick_cancel_id(), std::nullopt);
}

TEST_F(OptionOrderGeneratorTest, PickCancelIdReturnsLiveOrderId)
{
    Order o{};
    o.side  = Order::Side::Bid;
    o.price = Price{9900};
    o.qty   = 1;
    o.id    = 42u;
    m_gen.seed(o);

    const auto result = m_gen.pick_cancel_id();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42u);
}

TEST_F(OptionOrderGeneratorTest, PickCancelIdPicksFromLiveOrders)
{
    for (std::uint32_t i = 1; i <= 5; ++i) {
        Order o{};
        o.side  = Order::Side::Bid;
        o.price = Price{9000 + static_cast<Price::type>(i) * 100};
        o.qty   = 1;
        o.id    = i;
        m_gen.seed(o);
    }
    // Over many samples, every ID in [1,5] should be returned at least once.
    std::vector<std::uint32_t> seen;
    for (int trial = 0; trial < 200; ++trial) {
        auto id = m_gen.pick_cancel_id();
        ASSERT_TRUE(id.has_value());
        EXPECT_GE(*id, 1u);
        EXPECT_LE(*id, 5u);
        seen.push_back(*id);
    }
    for (std::uint32_t i = 1; i <= 5; ++i)
        EXPECT_TRUE(std::ranges::contains(seen, i)) << "id " << i << " never picked";
}

// --- start() / threaded integration ---

TEST_F(OptionOrderGeneratorTest, StartPopulatesBookWithOrders)
{
    OptionOrderGeneratorConfig cfg{
        Price{10000}, Price{500}, Price{100},
        10u, 200u, 0.30f, 0.20f,
        std::chrono::milliseconds{1}
    };
    Option opt{};
    opt.strike = Price{8000};
    std::memcpy(opt.symbol.data, "SPCX", 4);
    opt.year = 26; opt.month = 6; opt.day = 19; opt.flags = 1;

    std::vector<Order> added;
    OptionOrderGenerator gen{
        [&added](Order o) { added.push_back(o); return true; },
        [](std::uint32_t) { return true; },
        opt, cfg
    };

    gen.start();
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    EXPECT_GT(added.size(), 0u);
}
