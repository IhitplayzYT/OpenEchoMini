#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace echo::audio {

inline constexpr std::uint32_t module78_flac_max_block_samples = 0x1200;

class FlacBitReader {
public:
    explicit FlacBitReader(std::span<const std::uint8_t> bytes,
                           std::size_t bit_offset = 0) noexcept
        : bytes_(bytes), bit_offset_(bit_offset) {}

    [[nodiscard]] std::size_t bit_offset() const noexcept { return bit_offset_; }

    [[nodiscard]] bool read_bits(unsigned count, std::uint32_t& value) noexcept {
        if (count > 32 || bytes_.size() > std::numeric_limits<std::size_t>::max() / 8) {
            return false;
        }
        const std::size_t total_bits = bytes_.size() * 8;
        if (bit_offset_ > total_bits || count > total_bits - bit_offset_) {
            return false;
        }
        value = 0;
        for (unsigned i = 0; i < count; ++i) {
            const std::size_t bit = bit_offset_++;
            value = (value << 1) |
                ((bytes_[bit / 8] >> (7U - static_cast<unsigned>(bit & 7U))) & 1U);
        }
        return true;
    }

    [[nodiscard]] bool read_signed(unsigned count, std::int32_t& value) noexcept {
        if (count == 0 || count > 32) {
            return false;
        }
        std::uint32_t raw{};
        if (!read_bits(count, raw)) {
            return false;
        }
        if (count < 32) {
            const std::uint32_t sign = std::uint32_t{1} << (count - 1);
            raw = (raw ^ sign) - sign;
        }
        value = std::bit_cast<std::int32_t>(raw);
        return true;
    }

    [[nodiscard]] bool read_unary_zeros(std::uint32_t& count) noexcept {
        count = 0;
        for (;;) {
            std::uint32_t bit{};
            if (!read_bits(1, bit)) {
                return false;
            }
            if (bit != 0) {
                return true;
            }
            if (count == 0x7fffffffU) {
                return false;
            }
            ++count;
        }
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t bit_offset_{};
};

// Models FUN_01003fdc's partitioned residual decoder. The firmware accepts
// Rice methods 0 and 1, uses 4- or 5-bit parameters, and treats all-ones
// parameters as an escape followed by a 5-bit raw width. Escape values are
// retained as the observed zero-extended bit patterns.
[[nodiscard]] inline std::optional<std::vector<std::int32_t>>
decode_module78_flac_residuals(FlacBitReader& reader, std::uint32_t block_size,
                               std::uint32_t predictor_order) {
    if (block_size == 0 || block_size > module78_flac_max_block_samples) {
        return std::nullopt;
    }
    std::uint32_t method{};
    std::uint32_t partition_order{};
    if (!reader.read_bits(2, method) || method > 1 ||
        !reader.read_bits(4, partition_order)) {
        return std::nullopt;
    }

    const std::uint32_t partitions = std::uint32_t{1} << partition_order;
    const std::uint32_t samples_per_partition = block_size >> partition_order;
    if (predictor_order > samples_per_partition) {
        return std::nullopt;
    }
    const std::uint32_t residual_count = partitions * samples_per_partition - predictor_order;
    std::vector<std::int32_t> residuals;
    residuals.reserve(residual_count);

    const unsigned parameter_bits = method == 0 ? 4U : 5U;
    const std::uint32_t escape_parameter = (std::uint32_t{1} << parameter_bits) - 1U;
    for (std::uint32_t partition = 0; partition < partitions; ++partition) {
        const std::uint32_t count = samples_per_partition -
            (partition == 0 ? predictor_order : 0U);
        std::uint32_t parameter{};
        if (!reader.read_bits(parameter_bits, parameter)) {
            return std::nullopt;
        }

        if (parameter == escape_parameter) {
            std::uint32_t raw_width{};
            if (!reader.read_bits(5, raw_width)) {
                return std::nullopt;
            }
            for (std::uint32_t i = 0; i < count; ++i) {
                std::uint32_t raw{};
                if (!reader.read_bits(raw_width, raw)) {
                    return std::nullopt;
                }
                residuals.push_back(std::bit_cast<std::int32_t>(raw));
            }
            continue;
        }

        for (std::uint32_t i = 0; i < count; ++i) {
            std::uint32_t quotient{};
            std::uint32_t remainder{};
            if (!reader.read_unary_zeros(quotient) ||
                !reader.read_bits(parameter, remainder)) {
                return std::nullopt;
            }
            const std::uint32_t folded = (quotient << parameter) | remainder;
            const std::uint32_t sign = 0U - (folded & 1U);
            residuals.push_back(std::bit_cast<std::int32_t>((folded >> 1) ^ sign));
        }
    }
    return residuals;
}

// Models the fixed-predictor portion of FUN_01004116 after the caller has
// parsed the subframe header. LPC-coded subframes are handled elsewhere.
[[nodiscard]] inline std::optional<std::vector<std::int32_t>>
decode_module78_flac_fixed_subframe(FlacBitReader& reader,
                                    std::uint32_t block_size,
                                    unsigned sample_bits,
                                    unsigned predictor_order) {
    if (block_size == 0 || block_size > module78_flac_max_block_samples ||
        sample_bits == 0 || sample_bits > 32 || predictor_order > 4 ||
        predictor_order > block_size) {
        return std::nullopt;
    }

    std::vector<std::int32_t> samples(block_size);
    for (unsigned i = 0; i < predictor_order; ++i) {
        if (!reader.read_signed(sample_bits, samples[i])) {
            return std::nullopt;
        }
    }
    auto residuals = decode_module78_flac_residuals(reader, block_size, predictor_order);
    if (!residuals || residuals->size() != block_size - predictor_order) {
        return std::nullopt;
    }

    std::size_t residual = 0;
    for (std::size_t i = predictor_order; i < samples.size(); ++i) {
        std::uint32_t predicted = 0;
        const auto sample = [&](std::size_t back) {
            return static_cast<std::uint32_t>(samples[i - back]);
        };
        switch (predictor_order) {
        case 0:
            break;
        case 1:
            predicted = sample(1);
            break;
        case 2:
            predicted = 2U * sample(1) - sample(2);
            break;
        case 3:
            predicted = 3U * sample(1) - 3U * sample(2) + sample(3);
            break;
        case 4:
            predicted = 4U * sample(1) - 6U * sample(2) +
                        4U * sample(3) - sample(4);
            break;
        }
        const auto residual_bits = static_cast<std::uint32_t>((*residuals)[residual++]);
        samples[i] = std::bit_cast<std::int32_t>(predicted + residual_bits);
    }
    return samples;
}

// Reconstructs LPC samples with the coefficient/sample order passed to
// FUN_01003034. Module 78 runs that software kernel directly for predictor
// orders 1 and 2 and uses DSP routines for higher orders; this bounded portable
// implementation applies the same multiply-accumulate and arithmetic shift.
[[nodiscard]] inline std::optional<std::vector<std::int32_t>>
restore_module78_flac_lpc_subframe(std::span<const std::int32_t> warmup,
                                   std::span<const std::int32_t> residuals,
                                   std::span<const std::int32_t> coefficients,
                                   unsigned shift,
                                   std::uint32_t block_size) {
    if (block_size == 0 || block_size > module78_flac_max_block_samples ||
        warmup.empty() || warmup.size() != coefficients.size() ||
        warmup.size() > block_size || residuals.size() != block_size - warmup.size() ||
        shift > 31) {
        return std::nullopt;
    }

    std::vector<std::int32_t> samples(block_size);
    for (std::size_t i = 0; i < warmup.size(); ++i) {
        samples[i] = warmup[i];
    }
    for (std::size_t i = warmup.size(); i < samples.size(); ++i) {
        std::int64_t sum = 0;
        const std::size_t first_sample = i - warmup.size();
        for (std::size_t coefficient = 0; coefficient < coefficients.size(); ++coefficient) {
            sum += static_cast<std::int64_t>(coefficients[coefficient]) *
                   samples[first_sample + coefficient];
        }
        const std::int64_t predicted = sum >> shift;
        const std::uint32_t wrapped = static_cast<std::uint32_t>(predicted) +
            static_cast<std::uint32_t>(residuals[i - warmup.size()]);
        samples[i] = std::bit_cast<std::int32_t>(wrapped);
    }
    return samples;
}

} // namespace echo::audio
