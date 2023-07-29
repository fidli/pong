#include "platform/windows_nocrt.h"
#include <Windows.h>
#include <dwmapi.h>
#define DIRECTINPUT_VERSION 0x0800
#include <Dinput.h>
#include "platform/windows_types.h"
#define FIXED_STEP 0.0166666666666

#define GAME_UNIQUE_NAME "Pong"
#define PERSISTENT_MEM MEGABYTE(1000)
#define TEMP_MEM MEGABYTE(500)

#include "common.h"
#include "util_mem.h"
#define PROFILE 1
#include "util_profile.cpp"

//NOTE(AK): this-app instance
HINSTANCE hInstance;

#include "platform/windows_time.cpp"
#include "platform/windows_filesystem.cpp"
#include "platform/windows_io.cpp"
#include "util_log.cpp"
#include "util_string.cpp"
#include "util_data.cpp"
#include "util_font.cpp"
#include "platform/windows_opengl.cpp"
#include "util_opengl.cpp"
HWND window;
#include "platform/windows_imgui.cpp"
#include "platform/windows_audio.cpp"

//NOTE(AK): these are set after memory initialisation
struct Platform;
Platform * platform;
struct OpenGL;
OpenGL * gl;

#include "platform_game_func.cpp"
i32 platformGetDpi(){
    i32 r = GetDpiForWindow(window);
    if(r == 0){
        return 96;
    }
    return r;
}

struct Key{
    bool down;
    bool changed;
} keys[256];

struct OpenglSprite{
    i32 framesX;
    i32 framesY;
    i32 framesTotal;

    GLuint textureId;
};

struct Animation{
    char name[50];
    OpenglSprite sprite;
    Timer * timer;
};

struct AnimationDesc{
    const static i32 VERSION = 1;
    char spriteFilename[255];
    Animation animation;
    f64 duration;
};

Animation animations[255];
i32 animationCount = 0;

bool parseAnimationDescFileLine(i32 fileVersion, u64 line, const char * option, const char * value, void ** context){
    AnimationDesc * animationDesc = *(CAST(AnimationDesc**, context));
    if(line == 1 && fileVersion != AnimationDesc::VERSION){
        return false;
    }
    if (!strcmp(option, "file")){
        strcpy(animationDesc->spriteFilename, value);
        return true;
    }
    else if (!strcmp(option, "framesX")){
        return sscanf(value, "%d", &animationDesc->animation.sprite.framesX) > 0;
    }
    else if (!strcmp(option, "framesY")){
        return sscanf(value, "%d", &animationDesc->animation.sprite.framesY) > 0;
    }
    else if (!strcmp(option, "framesTotal")){
        return sscanf(value, "%d", &animationDesc->animation.sprite.framesTotal) > 0;
    }
    else if (!strcmp(option, "duration")){
        return sscanf(value, "%lf", &animationDesc->duration) > 0;
    }
    return false;
}

Animation * findAnimation(const char *name){
    for(i32 i = 0; i < animationCount; i++){
        if (strcmp(name, animations[i].name) == 0){
            return &animations[i];
        }
    }
    return NULL;
}

Animation * loadAnimation(const char * descFilePath){
    PROFILE_FUNC;
    PUSHI;
    AnimationDesc desc = {};
    bool r = loadConfig(descFilePath, parseAnimationDescFileLine, CAST(void*, &desc));
    ASSERT(r);
    
    Animation * animation = &animations[animationCount++];
    *animation = desc.animation;
    strcpy(animation->name, filename(descFilePath));
    *CAST(char*, extension(animation->name) - 1) = 0;
    
    FileContents imageFile = {};
    char imagePath[255] = {};
    strncpy(imagePath, descFilePath, filename(descFilePath) - descFilePath);
    strcpy(imagePath + strlen(imagePath), desc.spriteFilename);
    r &= readFile(imagePath, &imageFile);
    ASSERT(r);
    Image image;
    r &= decodePNG(&imageFile, &image);
    ASSERT(r);
    ASSERT(image.info.interpretation == BitmapInterpretationType_RGBA || BitmapInterpretationType_RGB);
    GLint interp = GL_RGBA;
    if(image.info.interpretation == BitmapInterpretationType_RGBA){
        interp = GL_RGBA;
    }else if(image.info.interpretation == BitmapInterpretationType_RGB){
        interp = GL_RGB;
    }
    else{
        INV;
    }
    ASSERT(image.info.origin == BitmapOriginType_TopLeft);
    animation->timer = addTimer(desc.duration);
    OpenglSprite * sprite = &animation->sprite;
    glGenTextures(1, &sprite->textureId);
    glBindTexture(GL_TEXTURE_2D, sprite->textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, interp, image.info.width, image.info.height, 0, interp, GL_UNSIGNED_BYTE, image.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    POPI;
    return animation;
}


#include "game.cpp"

//NOTE(AK): these are set after memory initialisation
Game * game;
#include "platform_game_render.cpp"


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
    if(!guiHandleInputWin(message, wParam, lParam)){
        switch(message)
        {
            case WM_NCMOUSEMOVE:{
                platform->mouseOut = true;
                return 0;
            }break;
            case WM_MOUSEMOVE:{
                i16 x = CAST(i16, lParam);
                i16 y = CAST(i16, lParam >> 16);
                game->entities[2].player.input.mouse.pos = DV2(x, y);
                platform->mouseOut = false;
            }break;
            case WM_SIZING:{
                RECT * dims = (RECT *) lParam;
                platform->resolution.x = dims->right - dims->left;
                platform->resolution.y = dims->bottom - dims->top;
                return TRUE;
            }break;
            case WM_SIZE:{
                platform->resolution.y = (i16) (lParam >> 16);
                platform->resolution.x = (i16) (lParam);
                return 0;
            }break;
            case WM_CLOSE:
            case WM_DESTROY:{
                    gameExit();
                    return 0;
            } break;
            case WM_KEYDOWN:{
                keys[CAST(u8, wParam)].changed = !keys[CAST(u8, wParam)].down;
                keys[CAST(u8, wParam)].down = true;
            }break;
            case WM_KEYUP:{
                keys[CAST(u8, wParam)].changed = keys[CAST(u8, wParam)].down;
                keys[CAST(u8, wParam)].down = false;
            }break;
        }
    }
    return DefWindowProc (hWnd, message, wParam, lParam);
}

enum ControllerObject{
    ControllerObject_Invalid,

    ControllerObject_Button,
    ControllerObject_XAxis,
    ControllerObject_YAxis,
    ControllerObject_ZAxis,

    ControllerObject_RxAxis,
    ControllerObject_RyAxis,
    ControllerObject_RzAxis,

    ControllerObjectCount
};

const char * controllerObjToStr(ControllerObject type)
{
    switch(type){
        case ControllerObject_Button:
            return "Button";
        case ControllerObject_XAxis:
            return "X Axis";
        case ControllerObject_YAxis:
            return "Y Axis";
        case ControllerObject_ZAxis:
            return "Z Axis";
        case ControllerObject_RxAxis:
            return "Rotation X";
        case ControllerObject_RyAxis:
            return "Rotation Y";
        case ControllerObject_RzAxis:
            return "Rotation Z";
        default:
            return "Unknown";
    }
}

struct Controller{
    GUID winId;
    LPDIRECTINPUTDEVICE8A dev;
    char name[50];

    struct {
        char name[50];
        ControllerObject type;
    } buttons[50];
    i32 buttonsCount;
} controller;

BOOL DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    controller.winId = lpddi->guidInstance;
    strncpy(controller.name, lpddi->tszInstanceName, ARRAYSIZE(controller.name));
    return DIENUM_CONTINUE;
}

BOOL DIEnumDeviceObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
    strncpy(controller.buttons[controller.buttonsCount].name, lpddoi->tszName, ARRAYSIZE(controller.buttons[controller.buttonsCount].name));
    ControllerObject type = ControllerObject_Invalid;
    if (lpddoi->dwType & DIDFT_PSHBUTTON && lpddoi->guidType == GUID_Button){
        type = ControllerObject_Button;
    } else if (lpddoi->dwType & DIDFT_ABSAXIS){
        if (lpddoi->guidType == GUID_XAxis){
            type = ControllerObject_XAxis;
        }
        if (lpddoi->guidType == GUID_YAxis){
            type = ControllerObject_YAxis;
        }
        if (lpddoi->guidType == GUID_ZAxis){
            type = ControllerObject_ZAxis;
        }
        if (lpddoi->guidType == GUID_RxAxis){
            type = ControllerObject_RxAxis;
        }
        if (lpddoi->guidType == GUID_RyAxis){
            type = ControllerObject_RyAxis;
        }
        if (lpddoi->guidType == GUID_RzAxis){
            type = ControllerObject_RzAxis;
        }
    }
    if (type != ControllerObject_Invalid){
        ASSERT((lpddoi->dwFlags & DIDOI_POLLED) == 0);
    }
    controller.buttons[controller.buttonsCount].type = type;
    controller.buttonsCount++;
    return DIENUM_CONTINUE;
}

BOOL DIEnumEffectsCallback(LPCDIEFFECTINFO pdei, LPVOID pvRef)
{
    return DIENUM_CONTINUE;
}

int main(LPWSTR * argvW, int argc) {
    (void)argvW;
    (void)argc;
    LPVOID memoryStart = VirtualAlloc(NULL, TEMP_MEM + PERSISTENT_MEM, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (memoryStart) {
        //START OF INITING ROUTINES
        initMemory(memoryStart);
        {
            HANDLE processToken  = NULL;
            if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &processToken))
            {
                DWORD privsize = 0;
                GetTokenInformation(processToken, TokenPrivileges, NULL, 0, &privsize);
                TOKEN_PRIVILEGES * priv = (TOKEN_PRIVILEGES *) &PUSHA_SCOPE(byte, privsize);
                if(GetTokenInformation(processToken, TokenPrivileges, priv , privsize, &privsize))
                {
                    for(DWORD i = 0; i < priv->PrivilegeCount; ++i )
                    { 
                        priv->Privileges[i].Attributes = SE_PRIVILEGE_REMOVED;
                    }
                    if(AdjustTokenPrivileges(processToken, TRUE, priv, NULL, NULL, NULL) == 0)
                    {
                        VirtualFree(memoryStart, 0, MEM_RELEASE);
                        return 1;
                    }
                }
            }else{
                VirtualFree(memoryStart, 0, MEM_RELEASE);
                return 1;
            }
            CloseHandle(processToken);
        }
        HANDLE instance = CreateMutexEx(NULL, "Global_" GAME_UNIQUE_NAME, 0, 0);
        if(instance == NULL){
            HWND runningEd = FindWindow("GameMainClass", GAME_UNIQUE_NAME);
            if(runningEd == NULL){
                MessageBox(NULL, "Other instance of game is running (maybe crashed, kill in task manager and check error log)", "Already running", MB_OK);
                VirtualFree(memoryStart, 0, MEM_RELEASE);
                return 1;
            }else{
                ShowWindow(runningEd, SW_SHOWNORMAL);
                SetForegroundWindow(runningEd);
                VirtualFree(memoryStart, 0, MEM_RELEASE);
                return 0;
            }
        }

        bool initSuccess = initIo();
        initSuccess &= initLog();
        if(!initSuccess){
            VirtualFree(memoryStart, 0, MEM_RELEASE);
            return 1;
        }
        
        //memory assignation
        platform = &PPUSH(Platform);
        game = &PPUSH(Game);
        gl = &PPUSH(OpenGL);
        
        //modules initalization
        initSuccess = initSuccess && initTime();
        LOG(default, startup, "Init time success: %u", initSuccess);
        initSuccess = initSuccess && initOpenGL();
        LOG(default, startup, "Init opengl success: %u", initSuccess);
        //creating window
        WNDCLASSEX style = {};
        style.cbSize = sizeof(WNDCLASSEX);
        style.style = CS_OWNDC | CS_DBLCLKS;
        style.lpfnWndProc = WindowProc;
        style.hInstance = hInstance;
        style.lpszClassName = "GameMainClass";
        initSuccess &= initSuccess && RegisterClassEx(&style) != 0;
        if(initSuccess){
            window = CreateWindowEx(NULL,
                                    "GameMainClass", GAME_UNIQUE_NAME, WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
                                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
            initSuccess &= window != NULL;
        }
        LOG(default, startup, "Init window success: %u", initSuccess);
        //Open gl initialisation
        //is used later, also when deleting
        HGLRC gameGlContext = {};
        HDC dc = GetDC(window);
        //creating drawing context, so that we can use opengl
        if(initSuccess){
              PIXELFORMATDESCRIPTOR contextFormat = {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                PFD_TYPE_RGBA,
                32, //32 bits per color
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                24, //24 bit z buffer
                0, //0 bit stencil buffer (these two values must not exceed 32 bit)
                0,
                0,
                0,
                0
            };
            
            const int attribListFormat[] =
            {
                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB, 32,
                WGL_DEPTH_BITS_ARB, 24,
                WGL_STENCIL_BITS_ARB, 0,
                //WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
                //WGL_SAMPLES_ARB, 8,
                0 // End
            };

            int pixelFormat = 0;
            UINT numFormats = 0;
            initSuccess &= wglChoosePixelFormatARB(dc, attribListFormat, NULL, 1, &pixelFormat, &numFormats) != 0 && numFormats > 0;
            initSuccess &= pixelFormat != 0;
            //setting the wished parameters upon our window
            initSuccess &= initSuccess && SetPixelFormat(dc, pixelFormat, &contextFormat) == TRUE;
            //creating opengl context
            if(initSuccess){
                const int attribListContext[] =
                {
                    0, // End
                };
                gameGlContext = wglCreateContextAttribsARB(dc, NULL, attribListContext);
                initSuccess &= gameGlContext != NULL;
                wglMakeCurrent(dc, gameGlContext);
            }
              
        }
        //end of opengl init
        LOG(default, startup, "Init opengl on window success: %u", initSuccess);
        HCURSOR cursor = (HCURSOR) LoadImage(hInstance, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
        //END OF INITIALISATION ROUTINES
        
        initSuccess &= initAudio(window);
        LOG(default, startup, "Init audio success: %u", initSuccess);

        if(initSuccess){
            //show window and get resolution
            {
                LOG(default, startup, "Showing window");
                ShowWindow(window, 1);
            }
            
            initGameRender();
            initSuccess &= initSuccess && guiInit("resources\\fonts\\lib-mono\\lib-mono.bmp", "resources\\fonts\\lib-mono\\lib-mono.fnt");
            LOG(default, startup, "Init gui success: %u", initSuccess);
            // CURSORS 
            FileContents imageFile = {};
            const char * fullPath = "resources\\hud\\cursor.bmp";
            bool r = readFile(fullPath, &imageFile);
            ASSERT(r);
            Image image;
            r &= decodeBMP(&imageFile, &image);
            ASSERT(r);
            ASSERT(image.info.interpretation == BitmapInterpretationType_RGBA || BitmapInterpretationType_RGB);
            if(image.info.origin == BitmapOriginType_BottomLeft){
                r &= flipY(&image);
                ASSERT(r);
            }
            GLint interp = GL_RGBA;
            if(image.info.interpretation == BitmapInterpretationType_RGBA){
                interp = GL_RGBA;
            }else if(image.info.interpretation == BitmapInterpretationType_RGB){
                interp = GL_RGB;
            }
            else{
                INV;
            }
            ASSERT(image.info.origin == BitmapOriginType_TopLeft);
            glGenTextures(1, &platform->render.cursorTexture);
            glBindTexture(GL_TEXTURE_2D, platform->render.cursorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, interp, image.info.width, image.info.height, 0, interp, GL_UNSIGNED_BYTE, image.data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            POP;
        }
#if 0
        if(initSuccess){
            profileBegin();
            loadAnimation("resources\\sprites\\pig-run.txt");
            profileEnd();
            printCurrentProfileStats();
            return 0;
            loadAnimation("resources\\sprites\\pig-idle.txt");
        }
#endif
        bool r = compileShaders(___sources_opengl_shaders_game_vert, ___sources_opengl_shaders_game_vert_len, ___sources_opengl_shaders_game_frag, ___sources_opengl_shaders_game_frag_len, &gl->game.vertexShader, &gl->game.fragmentShader, &gl->game.program);
        ASSERT(r);
        r &= compileShaders(___sources_opengl_shaders_hud_vert, ___sources_opengl_shaders_hud_vert_len, ___sources_opengl_shaders_hud_frag, ___sources_opengl_shaders_hud_frag_len, &gl->hud.vertexShader, &gl->hud.fragmentShader, &gl->hud.program);
        ASSERT(r);
        r &= compileShaders(___sources_opengl_shaders_wire_vert, ___sources_opengl_shaders_wire_vert_len, ___sources_opengl_shaders_wire_frag, ___sources_opengl_shaders_wire_frag_len, &gl->wire.vertexShader, &gl->wire.fragmentShader, &gl->wire.program);
        ASSERT(r);
        initGameShader();
        LOG(default, shaders, "Game shaders loaded");

        AudioTrack track;
        { // oink
            const char * fullPath = "resources\\audio\\oink.wav";
            u32 fileSize = 0;
            bool r = getFileSize(fullPath, &fileSize);
            ASSERT(r);
            FileContents audioFile = {};
            audioFile.size = fileSize;
            audioFile.contents = &PPUSHA(char, fileSize);
            r = readFile(fullPath, &audioFile);
            ASSERT(r);
            r &= decodeWAV(&audioFile, &track);
            ASSERT(r);
        }
        gameInit(track);

        keymap[GameAction_Up].key = 0x57;
        keymap[GameAction_Right].key = 0x44;
        keymap[GameAction_Down].key = 0x53;
        keymap[GameAction_Left].key = 0x41;
        keymap[GameAction_Kick].key = 0x20;

        IDirectInput8 * out = NULL;
        HRESULT hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, CAST(LPVOID*, &out), NULL);
        ASSERT(hr == DI_OK);
        ASSERT(out != NULL);
        hr = out->EnumDevices(DI8DEVCLASS_GAMECTRL, DIEnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
        ASSERT(hr == DI_OK);
        hr = out->CreateDevice(controller.winId, &controller.dev, NULL);
        ASSERT(hr == DI_OK);
        hr = controller.dev->EnumObjects(DIEnumDeviceObjectsCallback, NULL, DIDFT_ALL);
        ASSERT(hr == DI_OK);
        hr = controller.dev->EnumEffects(DIEnumEffectsCallback,NULL,DIEFT_ALL);
        ASSERT(hr == DI_OK);
        LOG(default, controller, "Controller: %s", controller.name);
        for(i32 i = 0; i < controller.buttonsCount; i++){
            LOG(default, controller, " - %s [%s]", controller.buttons[i].name, controllerObjToStr(controller.buttons[i].type));
        }
        DIPROPDWORD val = {};
        val.diph.dwSize = sizeof(DIPROPDWORD);
        val.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        val.diph.dwObj = 0;
        val.diph.dwHow = DIPH_DEVICE;
        val.dwData = MEGABYTE(1);
        hr = controller.dev->SetProperty(DIPROP_BUFFERSIZE, CAST(DIPROPHEADER*, &val));
        ASSERT(hr == DI_OK);

        
        DIOBJECTDATAFORMAT objs[1] = {};
        objs[0].pguid = &GUID_XAxis; 
        objs[0].dwOfs = 0; 
        objs[0].dwType = DIDFT_AXIS | DIDFT_ANYINSTANCE;
        /*
        objs[1].pguid = &GUID_YAxis;
        objs[1].dwOfs = 4; 
        objs[1].dwType = DIDFT_AXIS | DIDFT_ANYINSTANCE;
        */

        DIDATAFORMAT objects = {};
        objects.dwSize = sizeof(DIDATAFORMAT);
        objects.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
        objects.dwFlags = DIDF_ABSAXIS;
        objects.dwDataSize = 8;
        objects.dwNumObjs = ARRAYSIZE(objs);
        objects.rgodf = objs;
        hr = controller.dev->SetDataFormat(&objects);
        ASSERT(hr == DI_OK);

       
        hr = controller.dev->Acquire();
        ASSERT(hr == DI_OK);
        for(;;)
        {
            DIDEVICEOBJECTDATA data[10];
            DWORD datasize = ARRAYSIZE(data);
            hr = controller.dev->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), data, &datasize, 0);
            ASSERT(hr == DI_OK);
            if (datasize > 0){

            LOG(default, controller, "DATA START");
            for (DWORD i = 0; i < datasize; i++){
                LOG(default, controller, "%d %d %d", i, data[i].dwOfs, data[i].dwData);
            }
            LOG(default, controller, "DATA END");
            }
        }
        hr = controller.dev->Unacquire();
        ASSERT(hr == DI_OK);

        f64 currentTime = getProcessCurrentTime();
        platform->appRunning = true;
        platform->fps = 1;
        platform->frameIndex = 0;

        f64 accumulator = 0;
        Timer * fpsTimer = addTimer(1);
        u64 lastFpsFrameIndex = 0;
        bool wasShowProfile = platform->showProfile;
        //THE GAME LOOP
        Game previousState = *game;
        while (platform->appRunning) {
            if(!wasShowProfile && platform->showProfile){
                platform->framesRenderedSinceLastProfileClear = 0;
            }
            wasShowProfile = platform->showProfile;
            if(fpsTimer->ticked){
                platform->fps = CAST(f32, platform->frameIndex - lastFpsFrameIndex) / CAST(f32, (1.0f + fpsTimer->progressAbsolute));
                lastFpsFrameIndex = platform->frameIndex;
            }
            f64 newTime = getProcessCurrentTime();
            f64 frameTime = newTime - currentTime;
            accumulator += frameTime;
            currentTime = newTime;
            FLUSH;
            //END OF HOTLOADING
            
            //HANDLE INPUT
            auto prevMouse = game->entities[2].player.input.mouse;
            game->entities[2].player.input = {};
            game->entities[2].player.input.mouse = prevMouse;
            MSG msg;
            while(PeekMessage(&msg, window, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }


            
            gameHandleInput();
            while(accumulator >= FIXED_STEP){
                previousState = *game;
                gameFixedStep(FIXED_STEP);
                accumulator -= FIXED_STEP;
            }
            gameStep(frameTime);
            Game toRender = gameInterpolateSteps(&previousState, game, CAST(f32, accumulator/FIXED_STEP));
            
            if(platform->mouseOut){
                if(GetCursor() == NULL){
                    cursor = SetCursor(cursor);
                }
            }else{
                if(GetCursor() != NULL){
                    cursor = SetCursor(NULL);
                }
            }
            mix();
            render(&toRender, frameTime);
            r = SwapBuffers(dc) == TRUE;//&& DwmFlush() == S_OK;
            
            ASSERT(r);
            if(!r){
                platform->appRunning = false;
            }
            //END OF RENDER
            //
            
            // advance timers
            advanceTimers(frameTime);
            platform->frameIndex++;
            platform->framesRenderedSinceLastProfileClear++;
        }
        LOG(default, common, "Quitting main loop");
        //END OF MAIN LOOP

        controller.dev->Release();
        out->Release();

        //shaders and stuff
        glDeleteShader(gl->game.fragmentShader);
        glDeleteShader(gl->game.vertexShader);
        glDeleteProgram(gl->game.program);

        glDeleteShader(gl->hud.fragmentShader);
        glDeleteShader(gl->hud.vertexShader);
        glDeleteProgram(gl->hud.program);

        glDeleteShader(gl->wire.fragmentShader);
        glDeleteShader(gl->wire.vertexShader);
        glDeleteProgram(gl->wire.program);
        
        guiFinalize();
        //end of shaders and stuff
        
        //cleanup
        DestroyWindow(window);
        UnregisterClass("GameMainClass", hInstance);
        wglDeleteContext(gameGlContext);
        
        VirtualFree(memoryStart, 0, MEM_RELEASE);
        
    }else{
        // NO MEMORY
    }
    return 0;
}


/**
This is the entry point
*/
void __stdcall mainCRTStartup(){
    int argc = 0;
    LPWSTR * argv =  CommandLineToArgvW(GetCommandLineW(), &argc);
    hInstance = GetModuleHandle(NULL);
    int result = main(argv,argc);
    LocalFree(argv);
    ExitProcess(result);
}
