#pragma once

#include "Pad.h"

struct ActiveChannels {
    int count;
    short list[128];
};

struct Channel {
    PAD(244)
    int soundSource;
    PAD(160)
};
