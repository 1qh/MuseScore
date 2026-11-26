/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2022 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MU_NOTATION_PIANOKEYBOARDTYPES_H
#define MU_NOTATION_PIANOKEYBOARDTYPES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <QtGlobal>

namespace mu::notation {
using piano_key_t = uint8_t;

static constexpr qreal SMALL_KEY_WIDTH_SCALING = 0.5;
static constexpr qreal NORMAL_KEY_WIDTH_SCALING = 1.0;
static constexpr qreal LARGE_KEY_WIDTH_SCALING = 2.0;

using PianoKeyLayers = std::array<size_t, 128>;
inline constexpr std::array<uint32_t, 6> PIANO_KEY_LAYER_COLORS {
    0xFFE69F00, 0xFF56B4E9, 0xFFCC79A7, 0xFF8DB600, 0xFF009E73, 0xFFF99379
};

inline constexpr size_t pianoKeyLayerColorIndex(size_t layer)
{
    return (layer - 1) % PIANO_KEY_LAYER_COLORS.size();
}

template<typename Pitches>
inline void applyPianoKeyLayer(PianoKeyLayers& keys, const Pitches& pitches, size_t layer)
{
    for (const auto pitch : pitches) {
        if (pitch < keys.size() && (keys[pitch] == 0 || layer < keys[pitch])) {
            keys[pitch] = layer;
        }
    }
}

inline PianoKeyLayers resolvePianoKeyLayers(const std::vector<std::vector<piano_key_t> >& layers)
{
    PianoKeyLayers keys {};
    for (size_t layer = 0; layer < layers.size(); ++layer) {
        applyPianoKeyLayer(keys, layers[layer], layer + 1);
    }
    return keys;
}

enum class KeyState {
    None,
    OtherInSelectedChord,
    Selected
};
}

#endif // MU_NOTATION_PIANOKEYBOARDTYPES_H
