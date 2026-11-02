#pragma once

#include "echo/audio/vorbis_mapping.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace echo::audio {

struct VorbisSubmapPacketPlan {
    std::uint8_t submap_index{};
    std::uint8_t residue_index{};
    std::vector<std::uint8_t> channels;
    std::vector<std::uint8_t> channel_active;
};

struct VorbisMappingPacketPlan {
    std::vector<std::uint8_t> channel_active_after_coupling;
    std::vector<VorbisSubmapPacketPlan> residue_submaps;
    std::vector<std::uint8_t> inverse_coupling_order;
    std::vector<std::uint8_t> floor_render_order;
    std::vector<std::uint8_t> inverse_transform_order;
};

// Plans the ordering observed in FUN_01009dae after each channel's floor
// packet has been decoded. Submap groups retain inactive channels because the
// residue worker receives both the activity mask and all channel spectra.
[[nodiscard]] inline std::optional<VorbisMappingPacketPlan>
plan_vorbis_mapping_packet(const VorbisMapping0& mapping,
                           std::span<const std::uint8_t> floor_present) {
    constexpr std::size_t maximum_channels = 255;
    if (floor_present.empty() || floor_present.size() > maximum_channels ||
        mapping.channel_mux.size() != floor_present.size() ||
        mapping.submaps.empty() || mapping.submaps.size() > 256 ||
        mapping.coupling_steps.size() > 256) {
        return std::nullopt;
    }

    VorbisMappingPacketPlan plan;
    plan.channel_active_after_coupling.assign(floor_present.begin(), floor_present.end());
    for (const auto active : plan.channel_active_after_coupling) {
        if (active > 1) return std::nullopt;
    }
    for (const auto mux : mapping.channel_mux) {
        if (mux >= mapping.submaps.size()) return std::nullopt;
    }

    for (const auto [magnitude, angle] : mapping.coupling_steps) {
        if (magnitude >= floor_present.size() || angle >= floor_present.size() ||
            magnitude == angle) {
            return std::nullopt;
        }
        if (plan.channel_active_after_coupling[magnitude] != 0 ||
            plan.channel_active_after_coupling[angle] != 0) {
            plan.channel_active_after_coupling[magnitude] = 1;
            plan.channel_active_after_coupling[angle] = 1;
        }
    }

    plan.residue_submaps.reserve(mapping.submaps.size());
    for (std::size_t submap = 0; submap < mapping.submaps.size(); ++submap) {
        VorbisSubmapPacketPlan group;
        group.submap_index = static_cast<std::uint8_t>(submap);
        group.residue_index = mapping.submaps[submap].residue_index;
        for (std::size_t channel = 0; channel < mapping.channel_mux.size(); ++channel) {
            if (mapping.channel_mux[channel] != submap) continue;
            group.channels.push_back(static_cast<std::uint8_t>(channel));
            group.channel_active.push_back(plan.channel_active_after_coupling[channel]);
        }
        if (!group.channels.empty()) plan.residue_submaps.push_back(std::move(group));
    }

    for (std::size_t i = mapping.coupling_steps.size(); i > 0; --i) {
        plan.inverse_coupling_order.push_back(static_cast<std::uint8_t>(i - 1U));
    }
    for (std::size_t channel = 0; channel < floor_present.size(); ++channel) {
        plan.floor_render_order.push_back(static_cast<std::uint8_t>(channel));
        plan.inverse_transform_order.push_back(static_cast<std::uint8_t>(channel));
    }
    return plan;
}

} // namespace echo::audio
