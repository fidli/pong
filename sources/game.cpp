#define AI_STEP 0.25
#define INSTANT_DURATION 0.001
#include "util_collision.cpp"


enum GameAction{
    GameAction_Invalid,
    GameAction_Up,
    GameAction_Right,
    GameAction_Down,
    GameAction_Left,
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

struct Entity {

    f32 mass;

    v2 vel;
    CollisionRect body;

    Animation * animation;
    v4 overlayColor;

    union {
        struct {
            u8 score;
            u32 action;
            PlayerInput input;
            v2 dir;
            v2 wantDir;
            f32 rotationYRad;
            f32 angularVel;
        } player;
        struct {
            v2 dir;
        } ball;
    };
    
};

struct Game {
    f64 aiAccumulator;
    GameAction aiSteps[i32(AI_STEP/FIXED_STEP) + 1];
    i32 aiStepCount;
    i32 aiCurrentStep;

    AudioTrack track;

    CollisionRect boundaries[6];
    Entity entities[4];
};

extern Game * game;

void gameInit(AudioTrack track){

    game->entities[0].body.size = V2(170.0f, 90.0f);
    game->entities[0].body.pos = V2(0.0f, 0.0f);

    game->entities[2].body.size = V2(6.0f, 6.0f);
    game->entities[2].body.pos = game->entities[0].body.pos + V2(-game->entities[0].body.size.x/2 + game->entities[2].body.size.x/2.0f, 0);
    game->entities[2].mass = 80.0f;
    game->entities[2].player.dir = V2(1.0f, 0);
    game->entities[2].player.wantDir = game->entities[2].player.dir;
    game->entities[2].player.rotationYRad = degToRad(180);
    
    

    game->entities[3].body.size = V2(6.0f, 6.0f);
    game->entities[3].body.pos = game->entities[0].body.pos + V2(game->entities[0].body.size.x/2, 0) - game->entities[3].body.size/2.0f;
    game->entities[3].mass = 80.0f;
    game->entities[3].player.dir = V2(-1.0f, 0);
    game->entities[3].player.rotationYRad = degToRad(0);
    game->entities[3].player.wantDir = game->entities[3].player.dir;
    game->entities[2].animation = game->entities[3].animation = findAnimation("pig-idle");

    game->entities[1].body.size = V2(1.0f, 1.0f);
    game->entities[1].body.pos = game->entities[2].body.pos + game->entities[2].player.dir * ((length(game->entities[2].body.size) + length(game->entities[1].body.size))/2.0f);
    game->entities[1].ball.dir = game->entities[2].player.dir;
    game->entities[1].vel = V2(0, 0);
    game->entities[1].mass = 5.0f;

    game->boundaries[0].size = V2(game->entities[0].body.size.x, 1.0f);
    game->boundaries[0].pos = game->entities[0].body.pos + V2(0, game->entities[0].body.size.y/2 + game->boundaries[0].size.y/2.0f);

    game->boundaries[1].size = V2(1.0f, game->entities[0].body.size.y + game->boundaries[0].size.y*2);
    game->boundaries[1].pos = game->entities[0].body.pos + V2(-game->entities[0].body.size.x/2 - game->boundaries[1].size.x/2.0f, 0);

    game->boundaries[2].size = V2(game->entities[0].body.size.x, 1.0f);
    game->boundaries[2].pos = game->entities[0].body.pos + V2(0, -game->entities[0].body.size.y/2 - game->boundaries[2].size.y/2.0f);

    game->boundaries[3].size = V2(1.0f, game->entities[0].body.size.y + game->boundaries[0].size.y*2);
    game->boundaries[3].pos = game->entities[0].body.pos + V2(game->entities[0].body.size.x/2 + game->boundaries[1].size.x/2.0f, 0);

    game->boundaries[4].size = V2(1.0f, game->entities[0].body.size.y + game->boundaries[0].size.y*2);
    game->boundaries[4].pos = game->entities[0].body.pos + V2(-game->entities[0].body.size.x/2 + game->entities[0].body.size.x/5 - game->boundaries[1].size.x/2.0f, 0);

    game->boundaries[5].size = V2(1.0f, game->entities[0].body.size.y + game->boundaries[0].size.y*2);
    game->boundaries[5].pos = game->entities[0].body.pos + V2(game->entities[0].body.size.x/2 - game->entities[0].body.size.x/5 + game->boundaries[1].size.x/2.0f, 0);

    game->entities[0].overlayColor = V4(0, 1, 0, 0.1f);

    game->track = track;
}

void gameHandleInput(){
    if(keys[keymap[GameAction_Up].key].down){
        game->entities[2].player.action |= 1 << GameAction_Up;
    }
    if(keys[keymap[GameAction_Right].key].down){
        game->entities[2].player.action |= 1 << GameAction_Right;
    }
    if(keys[keymap[GameAction_Down].key].down){
        game->entities[2].player.action |= 1 << GameAction_Down;
    }
    if(keys[keymap[GameAction_Left].key].down){
        game->entities[2].player.action |= 1 << GameAction_Left;
    }
    if(keys[keymap[GameAction_Kick].key].down){
        game->entities[2].player.action |= 1 << GameAction_Kick;
    }
}

void gameFixedStep(f64 dt){
    game->aiAccumulator += dt;
    if (game->aiAccumulator >= AI_STEP)
    {
        i32 steps = 0;
        if (game->entities[1].vel.x > 0){
            v2 newPos = game->entities[1].body.pos + game->entities[1].vel*float(AI_STEP);
            f32 distance = newPos.y - game->entities[3].body.pos.y;
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
                game->entities[3].vel.y = -25.0f;
                game->entities[3].animation = findAnimation("pig-run");
            }break; 
            case GameAction_Down:
            {
                game->entities[3].vel.y = 25.0f;
                game->entities[3].animation = findAnimation("pig-run");
            }break;
            case GameAction_Stop:
            {
                game->entities[3].vel = V2(0.0f, 0.0f);
                game->entities[3].animation = findAnimation("pig-idle");
            }break;
            default:
            {
               INV;
            }break;
        } 
        game->aiCurrentStep++;
    }

    
    f32 playerForceSize = 40000.0f;
    v2 playerDir = V2(0.0f, 0.0f);
    if (game->entities[2].player.action & (1 << GameAction_Up))
    {
           playerDir += V2(0.0f, 1.0f);
    }
    if (game->entities[2].player.action & (1 << GameAction_Right))
    {
           playerDir += V2(1.0f, 0.0f);
           game->entities[2].player.rotationYRad = degToRad(180);
    }
    if (game->entities[2].player.action & (1 << GameAction_Down))
    {
           playerDir += V2(0.0f, -1.0f);
    }
    if (game->entities[2].player.action & (1 << GameAction_Left))
    {
           playerDir += V2(-1.0f, 0.0f);
           game->entities[2].player.rotationYRad = degToRad(0);
    }
    v2 playerForces = {};
    if (length(playerDir) > 0){
        v2 dir = normalize(playerDir);
        game->entities[2].player.wantDir = dir;
        playerForces = dir * playerForceSize;
        game->entities[2].animation = findAnimation("pig-run");
    }

    if (dot(game->entities[2].player.dir, game->entities[2].player.wantDir) != 1.0f)
    {
        f32 speedDir = cross(game->entities[2].player.dir, game->entities[2].player.wantDir).z;
        if (speedDir == 0.0f){
            speedDir = 1.0f;
        }else{
            speedDir = speedDir/ABS(speedDir);
        }
        f32 playerForceAngularSize = playerForceSize;
        f32 angularAcc = playerForceAngularSize / game->entities[1].mass;
        game->entities[2].player.angularVel += angularAcc*CAST(f32, dt);
        game->entities[2].player.angularVel = clamp(game->entities[2].player.angularVel, 0.0f, 4.0f*PI);
        f32 advance = fmod(game->entities[2].player.angularVel * CAST(f32, dt), 2.0f*PI);
        v2 newDir = rotate(game->entities[2].player.dir, speedDir*advance);
        f32 newSpeedDir = cross(newDir, game->entities[2].player.wantDir).z;
        if(newSpeedDir * speedDir <= 0.0f){
            game->entities[2].player.dir = game->entities[2].player.wantDir;
            game->entities[2].player.angularVel = 0;
        }else{
            game->entities[2].player.dir = newDir;
        }
    }
    

    f32 g = 10.0; // m / s^2
    f32 grassFrictionCoef = 10.0f;
    if (length(game->entities[2].vel) != 0){
        f32 FnPlayer = game->entities[2].mass * g;
        f32 FfrictionPlayer = grassFrictionCoef * FnPlayer;
        playerForces += -1*normalize(game->entities[2].vel)*FfrictionPlayer;
    }

    v2 oldVelocity = game->entities[2].vel;
    v2 acc = playerForces / game->entities[2].mass;
    game->entities[2].vel += acc * CAST(f32, dt);
    if (dot(oldVelocity, game->entities[2].vel) < 0 || length(game->entities[2].vel) < 0.005f){
        game->entities[2].vel = V2(0, 0);
        game->entities[2].animation = findAnimation("pig-idle");
    }
    f32 maxVel = 80.0f;
    if (length(game->entities[2].vel) > maxVel){
        game->entities[2].vel = normalize(game->entities[2].vel) * maxVel;
    }
    game->entities[2].body.pos += game->entities[2].vel * CAST(f32, dt);
    game->entities[3].body.pos += game->entities[3].vel * CAST(f32, dt);

    for(i32 pi = 2; pi <= 3; pi++)
    {
        i32 bounces = 1;
        bool retest = false;
        v2 intoDirection = game->entities[pi].vel;
        do{
            retest = false;
            for(i32 bi = 0; bi < ARRAYSIZE(game->boundaries) && !retest; bi++)
            {
                if (collide(game->entities[pi].body, game->boundaries[bi])){
                    intoDirection = collidePop(game->entities[pi].body, game->boundaries[bi], -intoDirection);
                    game->entities[pi].body.pos += intoDirection;
                    ASSERT(!collide(game->entities[pi].body, game->boundaries[bi]));
                    game->entities[pi].vel = V2(0, 0);
                    retest = true;
                }
            }
            bounces--;
        }while(bounces && retest);
    }
    if(length(game->entities[1].vel) == 0)
    {
        game->entities[1].body.pos = game->entities[2].body.pos + game->entities[2].player.dir * ((length(game->entities[2].body.size) + length(game->entities[1].body.size))/2.0f);
    }

    f32 ballWeight = 1.0; // kG
    f32 FnBall = ballWeight * g;
    f32 FfrictionBall = 0 * 0.08f * FnBall;

    f32 ballAcc = 0.0f;
    if ((game->entities[2].player.action & (1 << GameAction_Kick)) && length(game->entities[1].vel) == 0.0f){
        f32 force = 50000.0f;
        ballAcc = force  / ballWeight;
        game->entities[1].ball.dir = game->entities[2].player.dir;
    }

    ballAcc -= FfrictionBall / ballWeight;
    if ((game->entities[2].player.action & (1 << GameAction_Kick)) && length(game->entities[1].vel) == 0.0f){
        game->entities[1].vel = ((ballAcc * normalize(game->entities[1].ball.dir)) + acc) * CAST(f32, INSTANT_DURATION) + oldVelocity;
    }else{
        v2 newVel = game->entities[1].vel + ballAcc * CAST(f32, dt) * normalize(game->entities[1].ball.dir);
        if(dot(newVel, game->entities[1].vel) > 0){
            game->entities[1].vel = newVel;
        }
        else{
            game->entities[1].vel = V2(0, 0);
        }
    }

    
    static int b = 0;
    if (length(game->entities[1].vel) > 0){

        Entity currentBall = game->entities[1];
        f32 remainAdvance = length(game->entities[1].vel)*CAST(f32, dt);
        i32 bounces = 5;
        CollisionRect * bouncers[4] = {&game->boundaries[0], &game->boundaries[2], &game->entities[2].body, &game->entities[3].body};
        do{
            Entity newBall = currentBall;
            newBall.body.pos += currentBall.ball.dir*remainAdvance;
            bool collision = false;
            for(i32 i = 0; i < ARRAYSIZE(bouncers) && !collision; i++){
                if (collide(newBall.body, *bouncers[i])){
                    if (b == 1)
                            {
                    collide(newBall.body, *bouncers[i]);
                    }
                    collision = true;
                    v2 pop = collidePop(newBall.body, *bouncers[i], -(currentBall.ball.dir*remainAdvance));
                    currentBall.body.pos = newBall.body.pos + pop;
                    currentBall.ball.dir = collideReflect(currentBall.body, *bouncers[i], currentBall.ball.dir);
                    ASSERT(!collide(currentBall.body, *bouncers[i]));
                    remainAdvance = length(pop);
                    bounces--;
                    playAudio(&game->track);
                    b++;
                }
            }
            if (!collision){
                remainAdvance = 0;
                currentBall = newBall;
                if(collide(newBall.body, game->boundaries[1])){
                    game->entities[3].player.score += 1;
                    game->entities[1].vel = V2(0, 0);
                    game->entities[1].body.pos = V2(0, 0);
                    game->entities[3].vel = V2(0, 0);
                    return;
                }
                else if(collide(newBall.body, game->boundaries[3])){
                    game->entities[2].player.score += 1;
                    game->entities[1].vel = V2(0, 0);
                    game->entities[1].body.pos = V2(0, 0);
                    game->entities[3].vel = V2(0, 0);
                    return;
                }
            }
        } while (remainAdvance >= 0.0000005f && bounces > 0);
        ASSERT(remainAdvance <= 0.0000005f);
        game->entities[1] = currentBall;
        game->entities[1].vel = currentBall.ball.dir * length(game->entities[1].vel);
    }
}

Game gameInterpolateSteps(Game * from, Game * to, f32 t)
{
    Game result = *to;
    
    // 0 is field, no need to lerp/slepr
    for (i32 i = 1; i < ARRAYSIZE(game->entities); i++){
        result.entities[i].body.pos = lerp(&from->entities[i].body.pos, &to->entities[i].body.pos, t);
        if (i == 1)
        {
            result.entities[i].ball.dir = slerp(&from->entities[i].ball.dir, &to->entities[i].ball.dir, t);
        }
        else if (i == 2 || i == 3)
        {
            result.entities[i].player.dir = slerp(&from->entities[i].player.dir, &to->entities[i].player.dir, t);
        }
    }

    return result;
}

void gameStep(f64 dt){
    (void)dt;
    game->entities[2].player.action = 0;
}

void gameExit(){
    platform->appRunning = false;
}
