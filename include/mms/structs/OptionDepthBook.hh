// C++ Includes
#include <cstdint>
#include <utility>

// MarketMakerSimulator Includes

namespace mms 
{

enum class OptionSide : std::uint8_t
{
    Ask = 0,
    Bid = 1,
    Count = Bid + 1
};

template <class OptionStructT, std::size_t NO_OPTIONS>
class OptionDepthBook
{
public:
    

    using BlockT = std::array<OptionStructT, std::to_underlying(OptionSide::Count)>;

    OptionDepthBook() = default;
    ~OptionDepthBook();

    bool create() noexcept;
    
    auto get_block(this const auto& self, std::size_t idx) noexcept;

    template <OptionSide SIDE>
    auto get_side(this const auto& self, const std::size_t idx) noexcept;


protected:

private:
    BlockT* m_ptr{nullptr};
};

template <class OptionStructT, std::size_t NO_OPTIONS>
OptionDepthBook<OptionStructT, NO_OPTIONS>::~OptionDepthBook()
{
    delete m_ptr;
}

/// @todo custom allocator 
template <class OptionStructT, std::size_t NO_OPTIONS>
bool OptionDepthBook<OptionStructT, NO_OPTIONS>::create() noexcept
{
    return static_cast<bool>(m_ptr = new BlockT[NO_OPTIONS]{});
}

template <class OptionStructT, std::size_t NO_OPTIONS>
auto OptionDepthBook<OptionStructT, NO_OPTIONS>::get_block(this const auto& self, const std::size_t idx) noexcept
{
    return *(self.m_ptr + idx);
}

template <class OptionStructT, std::size_t NO_OPTIONS>
template <OptionSide SIDE>
auto OptionDepthBook<OptionStructT, NO_OPTIONS>::get_side(this const auto& self, const std::size_t idx) noexcept
{
    return self.get_block(idx)[std::to_underlying(SIDE)];
}

} // End namespace mms