#define AI_STEP 0.25
#define INSTANT_DURATION 0.001


enum GameAction{
    GameAction_Invalid,
    GameAction_Up,
    GameAction_Down,
    GameAction_Kick,
    GameAction_Stop,

    GameActionCount
};

struct KeyMapping{
    u8 key;
} keymap[GameActionCount];

struct PlayerInput {
    struct{
        dv2 pos;
    } mouse;
    bool down;
};

struct Body {

    f32 mass;

    v2 pos;
    v2 vel;
};

struct Game {
    PlayerInput input;

    u8 score1;
    u8 score2;

    Body player1;

    f32 player2;
    f32 vel2;

    f32 playerWidth;
    f32 playerHeight;

    f32 ballWidth;
    v2 ball;
    v2 ballDir;
    f32 ballVelocity;
    f32 ballAcc;
    bool kick;
    f64 ballDirAccumulator;

    f32 width;

    f64 aiAccumulator;
    GameAction aiSteps[i32(AI_STEP/FIXED_STEP) + 1];
    i32 aiStepCount;
    i32 aiCurrentStep;

    u32 player1Action;

    AudioTrack track;
};

extern Game * game;

void gameInit(){

    // pitch is 1.75 wide and 1 tall
    game->width = 1.75;
    game->player2 = 0.5f;

    game->ball = V2(0.5f*game->width, 0.5f);

    game->playerWidth = 0.01f*game->width;
    game->playerHeight = 0.1f;

    game->ballWidth = 0.025f;
    game->ballVelocity = 0.0f;
    game->ballDirAccumulator = 0.0;
    game->ballAcc = 0.0f;

    game->aiAccumulator = 0.0;
    game->vel2 = 0.0f;
    
    game->aiStepCount = 0;
    game->aiCurrentStep = 0;

    game->player1Action = 0;

    game->player1.pos = V2(0.0f, 0.5);
    game->player1.mass = 80.0f;

    game->ballDir = V2(1.75f, -2.0f);

    game->track = loadAudio();
    playAudio(&game->track);
    
}

void gameHandleInput(){
    if(keys[keymap[GameAction_Up].key].down){
        game->player1Action |= 1 << GameAction_Up;
    }
    if(keys[keymap[GameAction_Down].key].down){
        game->player1Action |= 1 << GameAction_Down;
    }
    if(keys[keymap[GameAction_Kick].key].down){
        game->player1Action |= 1 << GameAction_Kick;
    }
}

void gameFixedStep(f64 dt){
    game->aiAccumulator += dt;
    if (game->aiAccumulator >= AI_STEP)
    {
        i32 steps = 0;
        if (game->ballDir.x > 0){
            v2 newPos = game->ball + normalize(game->ballDir)*game->ballVelocity*float(AI_STEP);
            f32 distance = newPos.y - game->player2;
            f32 aiVel = 1.0f;
            steps = MIN(ABS((i32)((distance / aiVel)/dt)), i32(AI_STEP/FIXED_STEP));
            GameAction movement;
            if ( distance < 0)
            {
                movement = GameAction_Up;
            }else{
                movement = GameAction_Down;
            }
            for(i32 i = 0; i < steps; i++){
                game->aiSteps[i] = movement;
            }
        }
        game->aiSteps[steps] = GameAction_Stop;
        game->aiStepCount = steps + 1;
        game->aiCurrentStep = 0;

        game->aiAccumulator = fmod64(game->aiAccumulator, AI_STEP);
    }

    if ( game->aiCurrentStep < game->aiStepCount)
    {
        switch(game->aiSteps[game->aiCurrentStep]){
            case GameAction_Up:
            {
                game->vel2 = -1.0f;
            }break; 
            case GameAction_Down:
            {
                game->vel2 = 1.0f;
            }break;
            case GameAction_Stop:
            {
                game->vel2 = 0.0f;
            }break;
            default:
            {
               INV;
            }break;
        } 
        game->aiCurrentStep++;
    }

    game->ballDirAccumulator += dt;
    if(game->ballVelocity == 0)
    {
        f32 coef = CAST(f32, ABS(fmod64(game->ballDirAccumulator, 2.0f)-1.0f));
        v2 start = V2(1.75f, -2.0f);
        v2 end = V2(1.75f, 2.0f);
        ASSERT(coef >= 0 && coef <= 1.0f);
        game->ballDir = slerp(&end, &start, coef);
    }
    
    v2 playerForces = {0};
    if (game->player1Action & (1 << GameAction_Up))
    {
           playerForces += V2(0.0f, -600.0f); 
    }
    if (game->player1Action & (1 << GameAction_Down))
    {
           playerForces += V2(0.0f, 600.0f); 
    }

    f32 g = 10.0; // m / s^2
    f32 grassFrictionCoef = 0.5f;
    if (game->player1.vel.x != 0 || game->player1.vel.y != 0){
        f32 FnPlayer = game->player1.mass * g;
        f32 FfrictionPlayer = grassFrictionCoef * FnPlayer;
        playerForces += -1*normalize(game->player1.vel)*FfrictionPlayer;
    }

    v2 oldVelocity = game->player1.vel;
    v2 acc = playerForces / game->player1.mass;
    game->player1.vel += acc * CAST(f32, dt);
    if (dot(oldVelocity, game->player1.vel) < 0 || length(game->player1.vel) < 0.005f){
        game->player1.vel = V2(0, 0);
    }
    game->player1.pos += game->player1.vel * CAST(f32, dt);
    // collisions
    game->player1.pos.y = clamp(game->player1.pos.y, game->playerHeight/2.0f, 1.0f - game->playerHeight/2.0f);



    if(game->vel2 != 0.0f){
        game->player2 = clamp(CAST(f32, game->player2 + dt*game->vel2), game->playerHeight/2.0f, 1.0f - game->playerHeight/2.0f);
    }

    f32 ballWeight = 1.0; // kG
    f32 FnBall = ballWeight * g;
    f32 FfrictionBall = 0 * 0.08f * FnBall;

    game->ballAcc = 0.0f;
    if ((game->player1Action & (1 << GameAction_Kick)) && game->ballVelocity == 0.0f){
        f32 force = 1000.0f;
        game->ballAcc = force  / ballWeight;
        game->ballDirAccumulator = 0;
    }

    game->ballAcc -= FfrictionBall / ballWeight;
    if ((game->player1Action & (1 << GameAction_Kick)) && game->ballVelocity == 0.0f){
        game->ballVelocity += game->ballAcc * CAST(f32, INSTANT_DURATION);
    }else{
        game->ballVelocity += game->ballAcc * CAST(f32, dt);
    }
    game->ballVelocity = MAX(0, game->ballVelocity);
    

    f32 remain = game->ballVelocity*CAST(f32, dt);
    i32 bounces = 5;

    v2 currentBallDir = game->ballDir;
    v2 currentBall = game->ball;
    do{
        v2 step = normalize(currentBallDir);
        v2 newBall = currentBall + step*remain;
        if (newBall.x > 1.75f || newBall.x <= 0 || (newBall.x > 1.75f && newBall.y >= (game->player2 - game->playerHeight/2) && newBall.y <= (game->player2 + game->playerHeight/2))
            || (newBall.x < 0 && newBall.y >= (game->player1.pos.y - game->playerHeight/2) && newBall.y <= (game->player1.pos.y + game->playerHeight/2))
                    || newBall.y > 1.0f || newBall.y < 0){
            v2 validPart = V2(clamp(newBall.x, 0.0f, game->width), clamp(newBall.y, 0.0f, 1.0f)) - currentBall;
            currentBall += validPart;
            f32 validPartLen = length(validPart);
            remain -= validPartLen;

            v2 reflectedPart = step;
            if (newBall.x > 1.75f || newBall.x < 0.0f)
            {
                reflectedPart.y *= -1.0f;
            }else 
            if (newBall.y > 1.0f || newBall.y < 0.0f)
            {
                reflectedPart.x *= -1.0f;
            } 
            reflectedPart *= -1.0f;

            currentBallDir = reflectedPart;
            //bounces--;
            //playAudio(&game->track);
        } else if(newBall.x > 1.75f){
            game->score1 += 1;
            game->ballVelocity = 0;
            game->ball = V2(0.5f*game->width, 0.5f);
            game->vel2 = 0;
            return;
        } else if(newBall.x < 0){
            game->score2 += 1;
            game->ballVelocity = 0;
            game->ball = V2(0.5f*game->width, 0.5f);
            game->vel2 = 0;
            return;
        } else{
            currentBall = newBall;
            remain = 0;
        }
    } while (remain >= 0.0000005f && bounces > 0);
    ASSERT(remain <= 0.0000005f);
    game->ball = currentBall;
    game->ballDir = currentBallDir;
}

Game gameInterpolateSteps(Game * from, Game * to, f32 t)
{
    Game result = *to;
    
    result.player1.pos = lerp(&from->player1.pos, &to->player1.pos, t);
    result.player2 = lerp(from->player2, to->player2, t);
    result.ball = lerp(&from->ball, &to->ball, t);
    result.ballDir = slerp(&from->ballDir, &to->ballDir, t);

    return result;
}

void gameStep(f64 dt){
    (void)dt;
    game->player1Action = 0;
}

void gameExit(){
    platform->appRunning = false;
}
