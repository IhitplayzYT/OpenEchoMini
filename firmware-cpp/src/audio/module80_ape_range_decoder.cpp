#include "echo/audio/module80_ape_range_decoder.hpp"
#include "echo/audio/module80_ape_tables.hpp"

namespace echo::audio {

static_assert(kModule80ApeRangeTableA.cumulative.size() == 22);
static_assert(kModule80ApeRangeTableA.range_scale.size() == 22);
static_assert(kModule80ApeRangeTableB.cumulative.size() == 22);
static_assert(kModule80ApeRangeTableB.range_scale.size() == 22);

Module80ApeRangeDecoder::Module80ApeRangeDecoder(
    std::uint32_t range,
    std::uint32_t code,
    std::span<const std::uint8_t> refill_bytes) noexcept
    : range_(range), code_(code), refill_bytes_(refill_bytes) {}

bool Module80ApeRangeDecoder::normalize() noexcept {
    while (range_ < 0x800000u) {
        if (cursor_ >= refill_bytes_.size()) {
            failed_ = true;
            return false;
        }
        range_ <<= 8;
        code_ = (code_ << 8) | refill_bytes_[cursor_++];
    }
    return true;
}

std::optional<std::size_t> Module80ApeRangeDecoder::decode_symbol(
    std::span<const std::uint16_t> cumulative,
    std::span<const std::uint16_t> range_scale) noexcept {
    if (failed_ || cumulative.size() < 2 ||
        range_scale.size() < cumulative.size() - 1 ||
        cumulative.front() != 0 || !normalize()) {
        return std::nullopt;
    }

    for (std::size_t i = 1; i < cumulative.size(); ++i) {
        if (cumulative[i] <= cumulative[i - 1]) return std::nullopt;
    }
    const auto unit = range_ >> 16;
    if (unit == 0) {
        failed_ = true;
        return std::nullopt;
    }
    const auto scaled = code_ / unit;
    const auto total = cumulative.back();
    if (scaled >= total) {
        failed_ = true;
        return std::nullopt;
    }

    std::size_t symbol = 0;
    while (symbol + 1 < cumulative.size() &&
           scaled >= cumulative[symbol + 1]) {
        ++symbol;
    }
    const auto low = cumulative[symbol];
    const auto scale = range_scale[symbol];
    if (scale == 0) {
        failed_ = true;
        return std::nullopt;
    }
    code_ -= unit * low;
    range_ = unit * scale;
    return symbol;
}

std::uint32_t Module80ApeRangeDecoder::range() const noexcept { return range_; }
std::uint32_t Module80ApeRangeDecoder::code() const noexcept { return code_; }
std::size_t Module80ApeRangeDecoder::bytes_consumed() const noexcept { return cursor_; }
bool Module80ApeRangeDecoder::failed() const noexcept { return failed_; }

} // namespace echo::audio
