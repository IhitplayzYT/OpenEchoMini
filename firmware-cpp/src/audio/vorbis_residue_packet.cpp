#include "echo/audio/vorbis_residue.hpp"

#include <algorithm>
#include <limits>

namespace echo::audio {

bool decode_vorbis_residue_channels(
    const VorbisResidueSetup& setup,
    std::span<const VorbisCodebookSetup> codebooks,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::span<const std::uint8_t> channel_active,
    std::span<std::vector<std::int32_t>> channel_spectra) {
    if (setup.type > 1 || setup.partition_size == 0 || setup.classifications == 0 ||
        setup.classbook >= codebooks.size() || setup.classes.size() != setup.classifications ||
        channel_active.size() != channel_spectra.size() ||
        setup.end < setup.begin) {
        return false;
    }

    const auto& classbook = codebooks[setup.classbook];
    if (classbook.dimensions == 0) return false;
    const auto partition_count = (setup.end - setup.begin) / setup.partition_size;
    if (partition_count == 0) return true;
    for (std::size_t ch = 0; ch < channel_spectra.size(); ++ch) {
        if (channel_active[ch] != 0 && setup.end > channel_spectra[ch].size()) return false;
    }

    std::vector<std::vector<std::uint8_t>> classifications(channel_spectra.size());
    for (std::size_t ch = 0; ch < channel_spectra.size(); ++ch) {
        if (channel_active[ch] == 0) continue;
        auto& channel_classes = classifications[ch];
        channel_classes.resize(partition_count);
        for (std::size_t group = 0; group < partition_count;
             group += classbook.dimensions) {
            const auto symbol = decode_vorbis_codebook_symbol(classbook, packet, bit_offset);
            if (!symbol || *symbol >= classbook.entries) return false;
            auto codeword = *symbol;
            const auto group_end = std::min<std::size_t>(
                partition_count, group + classbook.dimensions);
            for (auto partition = group_end; partition > group; --partition) {
                channel_classes[partition - 1] = static_cast<std::uint8_t>(
                    codeword % setup.classifications);
                codeword /= setup.classifications;
            }
        }
    }

    for (std::uint8_t stage = 0; stage < setup.stage_count; ++stage) {
        for (std::size_t ch = 0; ch < channel_spectra.size(); ++ch) {
            if (channel_active[ch] == 0) continue;
            auto& spectrum = channel_spectra[ch];
            const auto& channel_classes = classifications[ch];
            for (std::size_t partition = 0; partition < partition_count; ++partition) {
                const auto class_index = channel_classes[partition];
                if (class_index >= setup.classes.size()) return false;
                const auto book_index = setup.classes[class_index].stage_books[stage];
                if (book_index == 0xff) continue;
                if (book_index >= codebooks.size()) return false;
                const auto& book = codebooks[book_index];
                if (book.lookup_type == 0 || book.dimensions == 0) return false;

                const auto partition_start = static_cast<std::size_t>(setup.begin) +
                    partition * setup.partition_size;
                const auto partition_end = partition_start + setup.partition_size;
                if (partition_end > setup.end || partition_end > spectrum.size()) return false;

                if (setup.type == 0) {
                    const auto step = setup.partition_size / book.dimensions;
                    if (step == 0) return false;
                    for (std::size_t offset = 0; offset < step; ++offset) {
                        const auto vector = decode_vorbis_codebook_vector(
                            book, packet, bit_offset, -8);
                        if (!vector || vector->size() != book.dimensions) return false;
                        for (std::size_t component = 0; component < book.dimensions; ++component) {
                            const auto index = partition_start + offset + component * step;
                            const auto sum = static_cast<std::int64_t>(spectrum[index]) +
                                (*vector)[component];
                            if (sum < std::numeric_limits<std::int32_t>::min() ||
                                sum > std::numeric_limits<std::int32_t>::max()) {
                                return false;
                            }
                            spectrum[index] = static_cast<std::int32_t>(sum);
                        }
                    }
                } else {
                    // The firmware consumes and scatters a full vector on the
                    // final partial partition step. This may extend into the
                    // following bins, so only the backing spectrum bounds it.
                    for (std::size_t offset = 0; offset < setup.partition_size;
                         offset += book.dimensions) {
                        const auto vector = decode_vorbis_codebook_vector(
                            book, packet, bit_offset, -8);
                        if (!vector || vector->size() != book.dimensions) return false;
                        for (std::size_t component = 0; component < book.dimensions;
                             ++component) {
                            const auto index = partition_start + offset + component;
                            if (index >= spectrum.size()) return false;
                            const auto sum = static_cast<std::int64_t>(spectrum[index]) +
                                (*vector)[component];
                            if (sum < std::numeric_limits<std::int32_t>::min() ||
                                sum > std::numeric_limits<std::int32_t>::max()) {
                                return false;
                            }
                            spectrum[index] = static_cast<std::int32_t>(sum);
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool decode_vorbis_residue_type2(
    const VorbisResidueSetup& setup,
    std::span<const VorbisCodebookSetup> codebooks,
    std::span<const std::uint8_t> packet,
    std::size_t& bit_offset,
    std::span<const std::uint8_t> channel_active,
    std::span<std::vector<std::int32_t>> channel_spectra) {
    if (setup.type != 2 || setup.partition_size == 0 ||
        setup.classifications == 0 || setup.classbook >= codebooks.size() ||
        setup.classes.size() != setup.classifications ||
        channel_active.size() != channel_spectra.size() ||
        channel_spectra.empty() || setup.end < setup.begin) {
        return false;
    }
    if (std::none_of(channel_active.begin(), channel_active.end(),
                     [](std::uint8_t active) { return active != 0; })) {
        return true;
    }

    const auto& classbook = codebooks[setup.classbook];
    if (classbook.dimensions == 0) return false;
    const auto partition_count = (setup.end - setup.begin) / setup.partition_size;
    if (partition_count == 0) return true;
    const auto channel_count = channel_spectra.size();
    const auto channel_partition_size = setup.partition_size / channel_count;
    const auto base_bin = setup.begin / channel_count;

    std::vector<std::uint8_t> classes(partition_count);
    for (std::size_t group = 0; group < partition_count;
         group += classbook.dimensions) {
        const auto symbol = decode_vorbis_codebook_symbol(classbook, packet, bit_offset);
        if (!symbol || *symbol >= classbook.entries) return false;
        auto codeword = *symbol;
        const auto group_end = std::min<std::size_t>(
            partition_count, group + classbook.dimensions);
        for (auto partition = group_end; partition > group; --partition) {
            classes[partition - 1] = static_cast<std::uint8_t>(
                codeword % setup.classifications);
            codeword /= setup.classifications;
        }
    }

    for (std::uint8_t stage = 0; stage < setup.stage_count; ++stage) {
        for (std::size_t partition = 0; partition < partition_count; ++partition) {
            const auto class_index = classes[partition];
            if (class_index >= setup.classes.size()) return false;
            const auto book_index = setup.classes[class_index].stage_books[stage];
            if (book_index == 0xff) continue;
            if (book_index >= codebooks.size()) return false;
            const auto& book = codebooks[book_index];
            if (book.lookup_type == 0 || book.dimensions == 0) return false;

            const auto start = base_bin + partition * channel_partition_size;
            const auto end = start + channel_partition_size;
            for (const auto& spectrum : channel_spectra) {
                if (end > spectrum.size()) return false;
            }
            std::size_t flattened_offset = 0;
            while (flattened_offset / channel_count < channel_partition_size) {
                const auto vector = decode_vorbis_codebook_vector(
                    book, packet, bit_offset, -8);
                if (!vector || vector->size() != book.dimensions) return false;
                for (std::size_t component = 0; component < book.dimensions;
                     ++component) {
                    const auto flat_index = flattened_offset + component;
                    const auto channel = flat_index % channel_count;
                    const auto bin = start + flat_index / channel_count;
                    if (bin >= channel_spectra[channel].size()) return false;
                    auto& value = channel_spectra[channel][bin];
                    const auto sum = static_cast<std::int64_t>(value) +
                        (*vector)[component];
                    if (sum < std::numeric_limits<std::int32_t>::min() ||
                        sum > std::numeric_limits<std::int32_t>::max()) {
                        return false;
                    }
                    value = static_cast<std::int32_t>(sum);
                }
                flattened_offset += book.dimensions;
            }
        }
    }
    return true;
}

} // namespace echo::audio
