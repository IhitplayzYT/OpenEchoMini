#include "echo/storage/module0_name_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace echo::storage {
namespace {

struct MappingRange {
    std::uint16_t first;
    std::uint8_t mode;
    std::uint8_t count;
    std::uint16_t table_offset;
};

// Literal range descriptors decoded from module 0 at 0x0306fd60 and
// 0x0306ff52. Mode 0 indexes the corresponding table; modes 1-8 are the
// arithmetic mappings in FUN_03058f22.
constexpr std::array<MappingRange, 35> kLowRanges{{
    {97U, 3U, 26U, 0U},
    {224U, 3U, 23U, 0U},
    {248U, 3U, 7U, 0U},
    {255U, 0U, 1U, 0U},
    {256U, 1U, 48U, 1U},
    {306U, 1U, 6U, 1U},
    {313U, 1U, 16U, 1U},
    {330U, 1U, 46U, 1U},
    {377U, 1U, 6U, 1U},
    {384U, 0U, 77U, 1U},
    {461U, 1U, 16U, 78U},
    {477U, 0U, 1U, 78U},
    {478U, 1U, 18U, 79U},
    {499U, 0U, 3U, 79U},
    {504U, 1U, 40U, 82U},
    {546U, 1U, 18U, 82U},
    {570U, 0U, 9U, 82U},
    {582U, 1U, 10U, 91U},
    {595U, 0U, 64U, 91U},
    {891U, 0U, 3U, 155U},
    {940U, 0U, 4U, 158U},
    {945U, 3U, 17U, 162U},
    {962U, 0U, 2U, 162U},
    {964U, 3U, 8U, 164U},
    {972U, 0U, 3U, 164U},
    {984U, 1U, 24U, 167U},
    {1010U, 0U, 10U, 167U},
    {1072U, 3U, 32U, 177U},
    {1104U, 7U, 16U, 177U},
    {1120U, 1U, 34U, 177U},
    {1162U, 1U, 54U, 177U},
    {1217U, 1U, 14U, 177U},
    {1231U, 0U, 1U, 177U},
    {1232U, 1U, 68U, 178U},
    {1377U, 4U, 38U, 178U},
}};
constexpr std::array<std::uint16_t, 178> kLowMappings{{
    376U, 579U, 385U, 386U, 386U, 388U, 388U, 390U, 391U, 391U,
    393U, 394U, 395U, 395U, 397U, 398U, 399U, 400U, 401U, 401U,
    403U, 404U, 502U, 406U, 407U, 408U, 408U, 573U, 411U, 412U,
    413U, 544U, 415U, 416U, 416U, 418U, 418U, 420U, 420U, 422U,
    423U, 423U, 425U, 426U, 427U, 428U, 428U, 430U, 431U, 431U,
    433U, 434U, 435U, 435U, 437U, 437U, 439U, 440U, 440U, 442U,
    443U, 444U, 444U, 446U, 503U, 448U, 449U, 450U, 451U, 452U,
    453U, 452U, 455U, 456U, 455U, 458U, 459U, 458U, 398U, 497U,
    500U, 500U, 11365U, 571U, 571U, 573U, 11366U, 575U, 576U, 577U,
    577U, 385U, 390U, 597U, 393U, 394U, 600U, 399U, 602U, 400U,
    604U, 605U, 606U, 607U, 403U, 609U, 610U, 404U, 612U, 613U,
    614U, 615U, 407U, 406U, 618U, 11362U, 620U, 621U, 622U, 412U,
    624U, 625U, 413U, 627U, 628U, 415U, 630U, 631U, 632U, 633U,
    634U, 635U, 636U, 11364U, 638U, 639U, 422U, 641U, 642U, 425U,
    644U, 645U, 646U, 647U, 430U, 580U, 433U, 434U, 581U, 653U,
    654U, 655U, 656U, 657U, 439U, 1021U, 1022U, 1023U, 902U, 904U,
    905U, 906U, 931U, 931U, 908U, 910U, 911U, 1017U, 1011U, 1012U,
    1013U, 1014U, 1015U, 1015U, 1017U, 1018U, 1018U, 1216U
}};

constexpr std::array<MappingRange, 31> kHighRanges{{
    {7549U, 0U, 1U, 0U},
    {7680U, 1U, 150U, 1U},
    {7840U, 1U, 90U, 1U},
    {7936U, 6U, 8U, 1U},
    {7952U, 6U, 6U, 1U},
    {7968U, 6U, 8U, 1U},
    {7984U, 6U, 8U, 1U},
    {8000U, 6U, 6U, 1U},
    {8017U, 0U, 7U, 1U},
    {8032U, 6U, 8U, 8U},
    {8048U, 0U, 14U, 8U},
    {8064U, 6U, 8U, 22U},
    {8080U, 6U, 8U, 22U},
    {8096U, 6U, 8U, 22U},
    {8112U, 0U, 4U, 22U},
    {8140U, 0U, 1U, 26U},
    {8144U, 6U, 2U, 27U},
    {8160U, 6U, 2U, 27U},
    {8165U, 0U, 1U, 27U},
    {8179U, 0U, 1U, 28U},
    {8526U, 0U, 1U, 29U},
    {8560U, 2U, 16U, 30U},
    {8580U, 0U, 1U, 30U},
    {9424U, 5U, 26U, 31U},
    {11312U, 4U, 47U, 31U},
    {11360U, 1U, 2U, 31U},
    {11367U, 1U, 6U, 31U},
    {11381U, 1U, 2U, 31U},
    {11392U, 1U, 100U, 31U},
    {11520U, 8U, 38U, 31U},
    {65345U, 3U, 26U, 31U},
}};
constexpr std::array<std::uint16_t, 31> kHighMappings{{
    11363U, 8025U, 8018U, 8027U, 8020U, 8029U, 8022U, 8031U, 8122U, 8123U,
    8136U, 8137U, 8138U, 8139U, 8154U, 8155U, 8184U, 8185U, 8170U, 8171U,
    8186U, 8187U, 8120U, 8121U, 8114U, 8124U, 8131U, 8172U, 8188U, 8498U,
    8579U
}};

[[nodiscard]] constexpr std::uint16_t rotate_right_one(
    const std::uint16_t value) noexcept {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(value >> 1U)
        | static_cast<std::uint16_t>(value << 15U));
}

template <std::size_t RangeCount, std::size_t MappingCount>
[[nodiscard]] std::uint32_t map_from_table(
    const std::uint32_t codepoint,
    const std::array<MappingRange, RangeCount>& ranges,
    const std::array<std::uint16_t, MappingCount>& mappings) noexcept {
    for (const MappingRange& range : ranges) {
        if (codepoint < range.first) {
            break;
        }
        const std::uint32_t end =
            static_cast<std::uint32_t>(range.first) + range.count;
        if (codepoint >= end) {
            continue;
        }

        switch (range.mode) {
        case 0:
            return mappings[range.table_offset
                + static_cast<std::size_t>(codepoint - range.first)];
        case 1:
            return static_cast<std::uint16_t>(
                codepoint - ((codepoint - range.first) & 1U));
        case 2:
            return static_cast<std::uint16_t>(codepoint - 0x10U);
        case 3:
            return static_cast<std::uint16_t>(codepoint - 0x20U);
        case 4:
            return static_cast<std::uint16_t>(codepoint - 0x30U);
        case 5:
            return static_cast<std::uint16_t>(codepoint - 0x1aU);
        case 6:
            return static_cast<std::uint16_t>(codepoint + 8U);
        case 7:
            return static_cast<std::uint16_t>(codepoint - 0x50U);
        case 8:
            return static_cast<std::uint16_t>(codepoint + 0xe3a0U);
        default:
            return codepoint;
        }
    }
    return codepoint;
}

} // namespace

std::uint32_t map_module0_name_codepoint(
    const std::uint32_t codepoint) noexcept {
    if (codepoint > 0xffffU) {
        return codepoint;
    }
    if (codepoint < 0x1000U) {
        return map_from_table(codepoint, kLowRanges, kLowMappings);
    }
    return map_from_table(codepoint, kHighRanges, kHighMappings);
}

std::uint16_t hash_module0_directory_name(
    const std::span<const std::uint16_t> nul_terminated_name) noexcept {
    std::uint16_t hash = 0;
    for (const std::uint16_t unit : nul_terminated_name) {
        const auto low_byte = static_cast<std::uint8_t>(unit & 0xffU);
        if (low_byte == 0U) {
            break;
        }

        const auto folded = static_cast<std::uint8_t>(
            map_module0_name_codepoint(low_byte));
        hash = static_cast<std::uint16_t>(
            rotate_right_one(hash) + folded);
        hash = rotate_right_one(hash);
    }
    return hash;
}

} // namespace echo::storage
