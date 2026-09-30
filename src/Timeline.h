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

    // Stores the last saved reference time.
    // We update this whenever settings change so time increases smoothly
    // without sudden jumps.
    int64_t checkpointSourceTime;
    int64_t checkpointVirtualTime;

    int64_t pausedTime;

    // Last virtual time sampled by getDeltaTime(), used to compute the delta
    int64_t lastDeltaSample;

    // Return time from anchor, or real time if this is a root timeline
    int64_t getSourceTime() const;
};

#endif