#include "Timeline.h"
#include <SDL3/SDL.h>

Timeline::Timeline(double tic)
{
    anchor = nullptr;

    this->tic = tic;
    scale = 1.0;

    paused = false;
    pausedTime = 0;

    checkpointSourceTime = getSourceTime();
    checkpointVirtualTime = 0;
    lastDeltaSample = 0;
}

Timeline::Timeline(Timeline* anchor, double tic)
{
    this->anchor = anchor;

    this->tic = tic;
    scale = 1.0;

    paused = false;
    pausedTime = 0;

    checkpointSourceTime = getSourceTime();
    checkpointVirtualTime = 0;
    lastDeltaSample = 0;
}

int64_t Timeline::getSourceTime() const
{
    // If this timeline is anchored to another timeline, use that timeline as the source of time
    if (anchor != nullptr) {
        return anchor->getTime();
    }

    // Otherwise use real time from SDL
    return static_cast<int64_t>(SDL_GetTicks());
}

int64_t Timeline::getTime() const
{
    if (paused) {
        return pausedTime;
    }

    // Extrapolate forward from the last checkpoint using the CURRENT scale.
    // Because the checkpoint is re-anchored every time scale/tic/pause state
    // changes, this never has to reapply a new scale to time that already
    // elapsed under the old scale -- that retroactive rescaling was the bug
    // that made anchored timelines (like a per-entity local timeline) jump
    // backward/forward whenever the game speed changed.
    int64_t elapsedSource = getSourceTime() - checkpointSourceTime;

    return checkpointVirtualTime + static_cast<int64_t>(elapsedSource * scale / tic);
}

double Timeline::getDeltaTime()
{
    // When the timeline is paused, everything stop moving
    if (paused) {
        return 0.0;
    }

    int64_t currentVirtualTime = getTime();
 
    int64_t elapsed = currentVirtualTime - lastDeltaSample;
 
    lastDeltaSample = currentVirtualTime;
 
    return static_cast<double>(elapsed) / 1000.0;
}

void Timeline::pause()
{
    if (paused) {
        return;
    }

    pausedTime = getTime();
    paused = true;
}

void Timeline::unpause()
{
    if (!paused) {
        return;
    }

    // Resume counting forward from exactly the time we paused at, using a
    // fresh checkpoint, so no paused real/source time gets counted as
    // elapsed once we unpause.
    checkpointVirtualTime = pausedTime;
    checkpointSourceTime = getSourceTime();
 
    // Avoid a one-frame delta-time spike covering the entire paused duration.
    lastDeltaSample = pausedTime;

    paused = false;
}

bool Timeline::isPaused() const
{
    return paused;
}

void Timeline::setScale(double scale)
{
    if (scale <= 0.0) {
        return;
    }
 
    // Re-anchor the checkpoint to "now" BEFORE changing scale, so time
    // already elapsed keeps whatever scale was in effect when it happened,
    // and only time going forward uses the new scale.
    if (!paused) {
        checkpointVirtualTime = getTime();
        checkpointSourceTime = getSourceTime();
    }
 
    this->scale = scale;
}

double Timeline::getScale() const
{
    return scale;
}

void Timeline::setTic(double tic)
{
    if (tic <= 0.0) {
        return;
    }
 
    // Same re-anchoring as setScale(), and for the same reason.
    if (!paused) {
        checkpointVirtualTime = getTime();
        checkpointSourceTime = getSourceTime();
    }
 
    this->tic = tic;
}

double Timeline::getTic() const
{
    return tic;
}