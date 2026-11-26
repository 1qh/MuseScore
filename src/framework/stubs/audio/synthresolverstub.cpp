/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
#include "synthresolverstub.h"

#include "synthesizerstub.h"

using namespace muse::audio;
using namespace muse::audio::synth;

ISynthesizerPtr SynthResolverStub::resolveSynth(const TrackId, const AudioInputParams& params, const audio::OutputSpec&,
                                                const PlaybackSetupData&) const
{
    return std::make_shared<SynthesizerStub>(params);
}

AudioResourceMetaList SynthResolverStub::resolveAvailableResources() const
{
    return {};
}

SoundPresetList SynthResolverStub::resolveAvailableSoundPresets(const AudioResourceMeta&) const
{
    return {};
}

void SynthResolverStub::registerResolver(const AudioSourceType, IResolverPtr)
{
}

void SynthResolverStub::unregisterResolver(const AudioSourceType)
{
}

void SynthResolverStub::clearSources()
{
}
