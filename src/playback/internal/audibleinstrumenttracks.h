#ifndef MU_PLAYBACK_AUDIBLEINSTRUMENTTRACKS_H
#define MU_PLAYBACK_AUDIBLEINSTRUMENTTRACKS_H

#include <vector>

#include "notation/inotationsolomutestate.h"

namespace mu::playback::detail {
struct InstrumentTrackSoloMuteState {
    engraving::InstrumentTrackId instrumentTrackId;
    notation::INotationSoloMuteState::SoloMuteState soloMuteState;
    bool hasPlaybackTrack = false;
    bool isMetronome = false;
    bool isChordSymbols = false;
};
struct EffectiveTrackSoloMuteState {
    engraving::InstrumentTrackId instrumentTrackId;
    notation::INotationSoloMuteState::SoloMuteState soloMuteState;
    bool forceMute = false;
};

inline std::vector<EffectiveTrackSoloMuteState> resolveEffectiveTrackSoloMuteStates(
    const std::vector<InstrumentTrackSoloMuteState>& trackStates, bool playChordSymbols, bool isRangePlaybackMode,
    const engraving::InstrumentTrackIdSet& allowedInstrumentTrackIds)
{
    bool hasSolo = false;
    for (const auto& state : trackStates) {
        if (!state.isMetronome && state.hasPlaybackTrack && state.soloMuteState.solo) {
            hasSolo = true;
            break;
        }
    }
    std::vector<EffectiveTrackSoloMuteState> result;
    for (const auto& state : trackStates) {
        if (!state.hasPlaybackTrack || state.isMetronome) {
            continue;
        }
        bool shouldForceMute = hasSolo && !state.soloMuteState.solo;
        if (state.isChordSymbols && !shouldForceMute) {
            shouldForceMute = !playChordSymbols;
        }
        if (isRangePlaybackMode && !shouldForceMute) {
            shouldForceMute = allowedInstrumentTrackIds.count(state.instrumentTrackId) == 0;
        }
        result.push_back({ state.instrumentTrackId, state.soloMuteState, shouldForceMute });
    }
    return result;
}

inline engraving::InstrumentTrackIdSet audibleInstrumentTrackIds(const std::vector<EffectiveTrackSoloMuteState>& trackStates)
{
    engraving::InstrumentTrackIdSet result;
    for (const auto& state : trackStates) {
        if (!state.soloMuteState.mute && !state.forceMute) {
            result.insert(state.instrumentTrackId);
        }
    }
    return result;
}
}

#endif
