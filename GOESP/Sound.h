#pragma once

struct ActiveChannels {
    int count;
    short list[128];
};

struct Channel {
    int pad0[61];
    int soundSource;
    int pad1[40];
};
