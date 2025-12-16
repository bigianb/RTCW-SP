#pragma once

class ClipModel
{
    public:
        int loadMap(const char *name);
        void clearMap();
};

class TheClipModel
{
    public:
        ClipModel& get() { return clipModel; }
    private:
        ClipModel clipModel;
};
