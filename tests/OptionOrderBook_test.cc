#include <gtest/gtest.h>
#include <memory>

#include "test_utils.hh"
#include "mms/exchange/OptionOrderBook.hh"
#include "mms/utils/Types.hh"
#include "mms/structs/Structs.hh"

using namespace mms;

class OptionOrderBookTest : public ::testing::Test
{
protected:
    static constexpr auto SIZE{1ULL << 10};
    static constexpr StringT SYMBOL{"SPCX"};

    void SetUp() override
    {
        m_obj = OptionOrderBook<SIZE, SYMBOL, LOG>{};
    }

    void TearDown() override
    {
        return;
    }


    static Order make_order(Order::Side side, std::uint32_t id)
    {
        return Order {
            "AAPL",
            side,
            Price{30000},
            3,
            id,
            1234,
            0x01,
        };
    }

    OptionOrderBook<SIZE, SYMBOL, LOG> m_obj;
    std::uint32_t m_id_seq{};
};

TEST_F(OptionOrderBookTest, OptionOrderBookTest_AddRemove)
{
    EXPECT_EQ(SIZE, m_obj.get_container(Order::Side::Bid).capacity());
    EXPECT_EQ(SIZE, m_obj.get_container(Order::Side::Ask).capacity());

    std::uint32_t id{0};
    auto side = Order::Side::Bid;
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_EQ(4, m_obj.get_container(Order::Side::Bid).size());

    side = Order::Side::Ask;
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_TRUE(m_obj.add(make_order(side, ++id)));
    EXPECT_EQ(4, m_obj.get_container(Order::Side::Bid).size());

    EXPECT_TRUE(m_obj.remove(id));
    EXPECT_FALSE(m_obj.has(id));
    EXPECT_TRUE(m_obj.remove(--id));
    EXPECT_FALSE(m_obj.has(id));

    EXPECT_EQ(SIZE, m_obj.get_container(Order::Side::Bid).capacity());
    EXPECT_EQ(SIZE, m_obj.get_container(Order::Side::Ask).capacity());
}