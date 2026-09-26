#ifndef TIMELINE_H
#define TIMELINE_H

#include <cstdint>

class Timeline {
public:
    // Real time, root timeline
    Timeline(double tic = 1.0);

    // Rimeline anchored to another timeline
    Timeline(Timeline* anchor, double tic = 1.0);

    // Current time represented by this timeline
    int64_t getTime() const;

    // Elapsed game time in second since the previous update
    double getDeltaTime();

    // Pause controls
    void pause();
    void unpause();
    bool isPaused() const;

    // Timeline scale
    void setScale(double scale);
    double getScale() const;

    void setTic(double tic);
    double getTic() const;

private:
    // Optinal parent timeline
    Timeline* anchor;

    double tic;
    double scale;
    bool paused;

    // Checkpoint-based accounting: checkpointVirtualTime is this timeline's
    // own time as of checkpointSourceTime (a reading from the source clock).
    // getTime() only ever extrapolates forward from that checkpoint using the
    // CURRENT scale/tic, so it never has to re-apply a new scale to time that
    // already happened. Every place that changes scale, tic, or pause state
    // moves the checkpoint up to "now" first, which is what keeps getTime()
    // continuous (no jumps) across those changes.
    int64_t checkpointSourceTime;
    int64_t checkpointVirtualTime;

    int64_t pausedTime;

    // Last virtual time sampled by getDeltaTime(), used to compute the delta
    int64_t lastDeltaSample;

    // Return time from anchor, or real time if this is a root timeline
    int64_t getSourceTime() const;
};

#endif