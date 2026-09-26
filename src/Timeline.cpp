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
    // If this timeline is anchored to another timeline, use that 
    // timeline as the source of time
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

    // Calculate time passed since the last checkpoint
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

    // Save state so time resumes smoothly without counting paused time
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
 
    // Save current time before changing speed so past time isn't affected
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
 
    // Save current time before changing tic size
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