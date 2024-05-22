#pragma once
struct Platform{
    bool appRunning;
    
    struct {
        GLuint cursorTexture;
    } render;
    
    dv2 resolution;

    u64 frameIndex;

    f32 fps;
    bool showProfile;

    bool mouseOut;
};


