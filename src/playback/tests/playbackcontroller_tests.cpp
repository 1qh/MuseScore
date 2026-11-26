/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

#include <gtest/gtest.h>

#include <vector>
#include <QJsonDocument>

#include "playback/internal/rememberedinputslot.h"
#include "playback/internal/audibleinstrumenttracks.h"
#include "project/audioinputparamsjson.h"

namespace {
using mu::engraving::InstrumentTrackId;
using mu::engraving::InstrumentTrackIdSet;
namespace detail = mu::playback::detail;
InstrumentTrackId makeTrackId(int partId)
{
    return { muse::ID(partId), u"instrument" };
}

detail::InstrumentTrackSoloMuteState makeTrackState(const InstrumentTrackId& trackId,
                                                    bool mute = false,
                                                    bool solo = false,
                                                    bool hasPlaybackTrack = true,
                                                    bool isMetronome = false,
                                                    bool isChordSymbols = false)
{
    return { trackId, { mute, solo }, hasPlaybackTrack, isMetronome, isChordSymbols };
}

InstrumentTrackIdSet audibleTrackIds(
    const std::vector<detail::InstrumentTrackSoloMuteState>& trackStates,
    bool playChordSymbols = true,
    bool isRangePlaybackMode = false,
    const InstrumentTrackIdSet& allowedInstrumentTrackIds = {})
{
    return detail::audibleInstrumentTrackIds(
        detail::resolveEffectiveTrackSoloMuteStates(trackStates,
                                                    playChordSymbols,
                                                    isRangePlaybackMode,
                                                    allowedInstrumentTrackIds));
}
}

TEST(PlaybackControllerTests, UnmutedPlaybackTracksAreAudible)
{
    const InstrumentTrackId firstTrackId = makeTrackId(1);
    const InstrumentTrackId secondTrackId = makeTrackId(2);

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(firstTrackId),
        makeTrackState(secondTrackId)
    });

    EXPECT_EQ(result.size(), 2u);
    EXPECT_TRUE(result.count(firstTrackId) != 0);
    EXPECT_TRUE(result.count(secondTrackId) != 0);
}

TEST(PlaybackControllerTests, PersistentOrExcerptMuteMakesTrackInaudible)
{
    const InstrumentTrackId mutedTrackId = makeTrackId(1);
    const InstrumentTrackId audibleTrackId = makeTrackId(2);

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(mutedTrackId, true),
        makeTrackState(audibleTrackId)
    });

    EXPECT_FALSE(result.count(mutedTrackId) != 0);
    EXPECT_TRUE(result.count(audibleTrackId) != 0);
}

TEST(PlaybackControllerTests, SoloMakesNonSoloTracksInaudible)
{
    const InstrumentTrackId soloTrackId = makeTrackId(1);
    const InstrumentTrackId nonSoloTrackId = makeTrackId(2);

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(soloTrackId, false, true),
        makeTrackState(nonSoloTrackId)
    });

    EXPECT_TRUE(result.count(soloTrackId) != 0);
    EXPECT_FALSE(result.count(nonSoloTrackId) != 0);
}

TEST(PlaybackControllerTests, DisabledChordSymbolsTrackIsInaudible)
{
    const InstrumentTrackId notationTrackId = makeTrackId(1);
    const InstrumentTrackId chordSymbolsTrackId = makeTrackId(2);
    const std::vector<detail::InstrumentTrackSoloMuteState> trackStates = {
        makeTrackState(notationTrackId),
        makeTrackState(chordSymbolsTrackId, false, false, true, false, true)
    };

    const InstrumentTrackIdSet disabledResult = audibleTrackIds(trackStates, false);
    const InstrumentTrackIdSet enabledResult = audibleTrackIds(trackStates, true);

    EXPECT_FALSE(disabledResult.count(chordSymbolsTrackId) != 0);
    EXPECT_TRUE(disabledResult.count(notationTrackId) != 0);
    EXPECT_TRUE(enabledResult.count(chordSymbolsTrackId) != 0);
}

TEST(PlaybackControllerTests, RangePlaybackOnlyAllowsSelectedTracks)
{
    const InstrumentTrackId selectedTrackId = makeTrackId(1);
    const InstrumentTrackId outsideRangeTrackId = makeTrackId(2);
    const InstrumentTrackIdSet allowedInstrumentTrackIds = { selectedTrackId };

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(selectedTrackId),
        makeTrackState(outsideRangeTrackId)
    }, true, true, allowedInstrumentTrackIds);

    EXPECT_TRUE(result.count(selectedTrackId) != 0);
    EXPECT_FALSE(result.count(outsideRangeTrackId) != 0);
}

TEST(PlaybackControllerTests, MetronomeIsExcludedAndDoesNotActivateSolo)
{
    const InstrumentTrackId notationTrackId = makeTrackId(1);
    const InstrumentTrackId metronomeTrackId = makeTrackId(2);

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(notationTrackId),
        makeTrackState(metronomeTrackId, false, true, true, true)
    });

    EXPECT_TRUE(result.count(notationTrackId) != 0);
    EXPECT_FALSE(result.count(metronomeTrackId) != 0);
}

TEST(PlaybackControllerTests, TrackWithoutPlaybackChannelIsInaudible)
{
    const InstrumentTrackId loadedTrackId = makeTrackId(1);
    const InstrumentTrackId unloadedTrackId = makeTrackId(2);

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(loadedTrackId),
        makeTrackState(unloadedTrackId, false, false, false)
    });

    EXPECT_TRUE(result.count(loadedTrackId) != 0);
    EXPECT_FALSE(result.count(unloadedTrackId) != 0);
}

TEST(PlaybackControllerTests, SoloTrackWithoutPlaybackChannelDoesNotMuteLoadedTracks)
{
    const InstrumentTrackId loadedTrackId = makeTrackId(1);
    const InstrumentTrackId unloadedSoloTrackId = makeTrackId(2);

    const InstrumentTrackIdSet result = audibleTrackIds({
        makeTrackState(loadedTrackId),
        makeTrackState(unloadedSoloTrackId, false, true, false)
    });

    EXPECT_TRUE(result.count(loadedTrackId) != 0);
    EXPECT_FALSE(result.count(unloadedSoloTrackId) != 0);
}

TEST(PlaybackControllerTests, RememberedInputSlotsFollowPartOrder)
{
    const InstrumentTrackId metronomeId = makeTrackId(99);
    const std::vector<muse::ID> orderedParts = { muse::ID(42), muse::ID(7) };
    EXPECT_EQ(detail::rememberedInputSlot(makeTrackId(42), metronomeId, false, orderedParts), "position/1");
    EXPECT_EQ(detail::rememberedInputSlot(makeTrackId(7), metronomeId, false, orderedParts), "position/2");
    const InstrumentTrackId differentInstrument { muse::ID(42), u"flute" };
    EXPECT_EQ(detail::rememberedInputSlot(differentInstrument, metronomeId, false, orderedParts), "position/1");
    const std::vector<muse::ID> reorderedParts = { muse::ID(7), muse::ID(42) };
    EXPECT_EQ(detail::rememberedInputSlot(makeTrackId(42), metronomeId, false, reorderedParts), "position/2");
    EXPECT_EQ(detail::rememberedInputSlot(makeTrackId(8), metronomeId, false, orderedParts), "");
}

TEST(PlaybackControllerTests, RememberedInputSlotsSeparateMetronomeAndChords)
{
    const InstrumentTrackId metronomeId = makeTrackId(99);
    const std::vector<muse::ID> orderedParts = { muse::ID(42), muse::ID(7) };
    EXPECT_EQ(detail::rememberedInputSlot(metronomeId, metronomeId, false, orderedParts), "metronome");
    EXPECT_EQ(detail::rememberedInputSlot(makeTrackId(42), metronomeId, true, orderedParts), "chords");
    EXPECT_EQ(detail::rememberedInputSlot(makeTrackId(7), metronomeId, true, orderedParts), "chords");
}

TEST(PlaybackControllerTests, RememberedInputSlotJsonRoundTrip)
{
    muse::audio::AudioInputParams pick;
    pick.resourceMeta.id = "reader.sf2";
    pick.resourceMeta.vendor = "Local";
    pick.resourceMeta.type = muse::audio::AudioResourceType::FluidSoundfont;
    pick.resourceMeta.hasNativeEditorSupport = true;
    pick.resourceMeta.attributes.emplace(u"preset", u"17");
    pick.configuration.emplace("state", std::string("a\0b", 3));
    const InstrumentTrackId metronomeId = makeTrackId(99);
    const std::vector<muse::ID> orderedParts = { muse::ID(42), muse::ID(7) };
    const QString slotName = QString::fromStdString(
        detail::rememberedInputSlot(makeTrackId(7), metronomeId, false, orderedParts));
    ASSERT_EQ(slotName, QString("position/2"));
    QJsonObject slotInputs;
    slotInputs.insert(slotName, mu::project::audioSettingsJson::inputParamsToJson(pick));
    const QByteArray json = QJsonDocument(slotInputs).toJson(QJsonDocument::Compact);
    const QJsonObject restoredSlots = QJsonDocument::fromJson(json).object();
    ASSERT_TRUE(restoredSlots.contains("position/2"));
    const QJsonObject slot = restoredSlots.value("position/2").toObject();
    EXPECT_EQ(slot.value("resourceMeta").toObject().value("type").toString(), QString("fluid_soundfont"));
    EXPECT_EQ(slot.value("unitConfiguration").toObject().value("state").toString(), QString("YQBi"));
    const auto restored = mu::project::audioSettingsJson::inputParamsFromJson(slot);
    ASSERT_TRUE(restored.isValid());
    EXPECT_EQ(restored.resourceMeta.id, "reader.sf2");
    EXPECT_EQ(restored.resourceMeta.vendor, "Local");
    EXPECT_EQ(restored.resourceMeta.type, muse::audio::AudioResourceType::FluidSoundfont);
    EXPECT_TRUE(restored.resourceMeta.hasNativeEditorSupport);
    EXPECT_EQ(restored.resourceMeta.attributeVal(u"preset"), muse::String(u"17"));
    ASSERT_EQ(restored.configuration.count("state"), 1u);
    EXPECT_EQ(restored.configuration.at("state"), std::string("a\0b", 3));
    EXPECT_TRUE(restored == pick);
    EXPECT_FALSE(mu::project::audioSettingsJson::inputParamsFromJson(restoredSlots.value("position/3").toObject()).isValid());
}
