#pragma once
enum EntityType : u32;

struct Platform{
    bool appRunning;
    
    struct {
        GLuint cursorTexture;
    } render;
    
    dv2 resolution;

    u64 frameIndex;

    f32 fps;
    u32 framesRenderedSinceLastProfileClear;
    bool showProfile;

    bool mouseOut;
};


