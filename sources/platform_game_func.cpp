#pragma once
enum EntityType : u32;

struct Timer{
    f64 period;
    f64 progressAbsolute;
    f64 progressNormalized;
    bool ticked;
};

struct Platform{
    bool appRunning;
    
    Timer timers[255];
    i32 timersCount;
    
    struct {
        GLuint cursorTexture;
    } render;
    
    dv2 resolution;

    u64 frameIndex;

    float fps;
    u32 framesRenderedSinceLastProfileClear;
    bool showProfile;

    bool mouseOut;
};

i32 platformAddTimer(f64 tick){
    i32 index = platform->timersCount;
    Timer * t = &platform->timers[index];
    memset(CAST(void*, t), 0, sizeof(Timer));
    t->period = tick;
    platform->timersCount++;
    return index;
}

Timer * platformGetTimer(i32 index){
    return &platform->timers[index];
}

void platformResetTimer(i32 index, f64 tick){
    Timer * t = &platform->timers[index];
    memset(CAST(void*, t), 0, sizeof(Timer));
    t->period = tick;
}

