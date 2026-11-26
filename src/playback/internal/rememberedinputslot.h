#ifndef MU_PLAYBACK_REMEMBEREDINPUTSLOT_H
#define MU_PLAYBACK_REMEMBEREDINPUTSLOT_H

#include <string>
#include <vector>

#include "async/notifylist.h"
#include "engraving/dom/part.h"
#include "notation/inotationparts.h"
#include "notation/inotationplayback.h"

namespace mu::playback::detail {
inline std::string rememberedInputSlot(const engraving::InstrumentTrackId& trackId,
                                       const engraving::InstrumentTrackId& metronomeId, bool isChordSymbols,
                                       const std::vector<muse::ID>& orderedPartIds)
{
    if (trackId == metronomeId) {
        return "metronome";
    }
    if (isChordSymbols) {
        return "chords";
    }
    for (size_t index = 0; index < orderedPartIds.size(); ++index) {
        if (orderedPartIds[index] == trackId.partId) {
            return "position/" + std::to_string(index + 1);
        }
    }
    return {};
}

inline std::string rememberedInputSlot(const engraving::InstrumentTrackId& trackId,
                                       const notation::INotationPlayback& playback, const notation::INotationParts& parts)
{
    std::vector<muse::ID> orderedPartIds;
    for (const engraving::Part* part : parts.partList()) {
        orderedPartIds.push_back(part->id());
    }
    return rememberedInputSlot(trackId, playback.metronomeTrackId(), playback.isChordSymbolsTrack(trackId), orderedPartIds);
}
}

#endif
