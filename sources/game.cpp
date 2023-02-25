#define AI_STEP 0.25
#define INSTANT_DURATION 0.001
#include "util_collision.cpp"


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

struct Entity {

    f32 mass;

    v2 vel;
    v2 dir;

    CollisionRect body;
};

struct Game {
    PlayerInput input;

    Entity * field;
    Entity * player1;
    Entity * player2;
    Entity * ball;

    u8 score1;
    u8 score2;


    bool kick;
    f64 ballDirAccumulator;


    f64 aiAccumulator;
    GameAction aiSteps[i32(AI_STEP/FIXED_STEP) + 1];
    i32 aiStepCount;
    i32 aiCurrentStep;

    u32 player1Action;

    AudioTrack track;

    Entity entities[4];
    CollisionRect boundaries[4];
};

extern Game * game;

void gameInit(AudioTrack track){

    game->field = &game->entities[0];
    game->player1 = &game->entities[1];
    game->player2 = &game->entities[2];
    game->ball = &game->entities[3];

    game->field->body.size = V2(120.0f, 90.0f);
    game->field->body.pos = V2(0.0f, 0.0f);

    game->player1->body.size = V2(1.0f, 6.0f);
    game->player1->body.pos = game->field->body.pos + V2(-game->field->body.size.x/2 + game->player1->body.size.x/2.0f, 0);
    game->player1->mass = 80.0f;

    game->player2->body.size = V2(1.0f, 6.0f);
    game->player2->body.pos = game->field->body.pos + V2(game->field->body.size.x/2, 0) - game->player2->body.size/2.0f;
    game->player2->mass = 80.0f;

    game->ball->body.size = V2(1.0f, 1.0f);
    game->ball->body.pos = -game->ball->body.size/2.0f;
    game->ball->dir = V2(1.75f, -2.0f);

    game->boundaries[0].size = V2(game->field->body.size.x, 1.0f);
    game->boundaries[0].pos = game->field->body.pos + V2(0, game->field->body.size.y/2 + game->boundaries[0].size.y/2.0f);

    game->boundaries[1].size = V2(1.0f, game->field->body.size.y + game->boundaries[0].size.y*2);
    game->boundaries[1].pos = game->field->body.pos + V2(-game->field->body.size.x/2 - game->boundaries[1].size.x/2.0f, 0);

    game->boundaries[2].size = V2(game->field->body.size.x, 1.0f);
    game->boundaries[2].pos = game->field->body.pos + V2(0, -game->field->body.size.y/2 - game->boundaries[2].size.y/2.0f);

    game->boundaries[3].size = V2(1.0f, game->field->body.size.y + game->boundaries[0].size.y*2);
    game->boundaries[3].pos = game->field->body.pos + V2(game->field->body.size.x/2 + game->boundaries[1].size.x/2.0f, 0);

    game->ballDirAccumulator = 0.0;

    game->aiAccumulator = 0.0;
    
    game->aiStepCount = 0;
    game->aiCurrentStep = 0;

    game->player1Action = 0;



    game->track = track;
    
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
        if (game->ball->vel.x > 0){
            v2 newPos = game->ball->body.pos + game->ball->vel*float(AI_STEP);
            f32 distance = newPos.y - game->player2->body.pos.y;
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
                game->player2->vel.y = -25.0f;
            }break; 
            case GameAction_Down:
            {
                game->player2->vel.y = 25.0f;
            }break;
            case GameAction_Stop:
            {
                game->player2->vel = V2(0.0f, 0.0f);
            }break;
            default:
            {
               INV;
            }break;
        } 
        game->aiCurrentStep++;
    }

    game->ballDirAccumulator += dt;
    if(length(game->ball->vel) == 0)
    {
        f32 coef = CAST(f32, ABS(fmod64(game->ballDirAccumulator, 2.0f)-1.0f));
        v2 start = V2(1.75f, -2.0f);
        v2 end = V2(1.75f, 2.0f);
        ASSERT(coef >= 0 && coef <= 1.0f);
        game->ball->dir = slerp(&end, &start, coef);
    }
    
    v2 playerForces = {0};
    if (game->player1Action & (1 << GameAction_Up))
    {
           playerForces += V2(0.0f, 6000.0f); 
    }
    if (game->player1Action & (1 << GameAction_Down))
    {
           playerForces += V2(0.0f, -6000.0f); 
    }

    f32 g = 10.0; // m / s^2
    f32 grassFrictionCoef = 0.5f;
    if (game->player1->vel.x != 0 || game->player1->vel.y != 0){
        f32 FnPlayer = game->player1->mass * g;
        f32 FfrictionPlayer = grassFrictionCoef * FnPlayer;
        playerForces += -1*normalize(game->player1->vel)*FfrictionPlayer;
    }

    v2 oldVelocity = game->player1->vel;
    v2 acc = playerForces / game->player1->mass;
    game->player1->vel += acc * CAST(f32, dt);
    if (dot(oldVelocity, game->player1->vel) < 0 || length(game->player1->vel) < 0.005f){
        game->player1->vel = V2(0, 0);
    }
    game->player1->body.pos += game->player1->vel * CAST(f32, dt);
    game->player2->body.pos += game->player2->vel * CAST(f32, dt);

    if (collide(game->player1->body, game->boundaries[0])){
        game->player1->body.pos += collidePop(game->player1->body, game->boundaries[0], -game->player1->vel);
        game->player1->vel = V2(0, 0);
    }else if (collide(game->player1->body, game->boundaries[2])){
        game->player1->body.pos += collidePop(game->player1->body, game->boundaries[2], -game->player1->vel);
        game->player1->vel = V2(0, 0);
    }
    if (collide(game->player2->body, game->boundaries[0])){
        game->player2->body.pos += collidePop(game->player2->body, game->boundaries[0], -game->player2->vel);
        game->player2->vel = V2(0, 0);
    }else if (collide(game->player2->body, game->boundaries[2])){
        game->player2->body.pos += collidePop(game->player2->body, game->boundaries[2], -game->player2->vel);
        game->player2->vel = V2(0, 0);
    }

    f32 ballWeight = 1.0; // kG
    f32 FnBall = ballWeight * g;
    f32 FfrictionBall = 0 * 0.08f * FnBall;

    f32 ballAcc = 0.0f;
    if ((game->player1Action & (1 << GameAction_Kick)) && length(game->ball->vel) == 0.0f){
        f32 force = 50000.0f;
        ballAcc = force  / ballWeight;
        game->ballDirAccumulator = 0;
    }

    ballAcc -= FfrictionBall / ballWeight;
    if ((game->player1Action & (1 << GameAction_Kick)) && length(game->ball->vel) == 0.0f){
        game->ball->vel = ballAcc * CAST(f32, INSTANT_DURATION) * normalize(game->ball->dir);
    }else{
        v2 newVel = game->ball->vel + ballAcc * CAST(f32, dt) * normalize(game->ball->dir);
        if(dot(newVel, game->ball->vel) > 0){
            game->ball->vel = newVel;
        }
        else{
            game->ball->vel = V2(0, 0);
        }
    }

    
    if (length(game->ball->vel) > 0){
        v2 currentBallDir = normalize(game->ball->vel);
        Entity currentBall = *game->ball;

        f32 remainAdvance = length(game->ball->vel)*CAST(f32, dt);
        i32 bounces = 5;
        CollisionRect * bouncers[4] = {&game->boundaries[0], &game->boundaries[2], &game->player1->body, &game->player2->body};
        do{
            Entity newBall = currentBall;
            newBall.body.pos += currentBallDir*remainAdvance;
            bool collision = false;
            for(i32 i = 0; i < ARRAYSIZE(bouncers) && !collision; i++){
                if (collide(newBall.body, *bouncers[i])){
                    collision = true;
                    v2 pop = collidePop(newBall.body, *bouncers[i], -(currentBallDir*remainAdvance));
                    currentBallDir = collideReflect(newBall.body, *bouncers[i], currentBallDir);
                    currentBall.body.pos += pop;
                    remainAdvance = length(pop);
                    bounces--;
                    playAudio(&game->track);
                }
            }
            if (!collision){
                remainAdvance = 0;
                currentBall = newBall;
                if(collide(newBall.body, game->boundaries[1])){
                    game->score2 += 1;
                    game->ball->vel = V2(0, 0);
                    game->ball->body.pos = V2(0, 0);
                    game->player2->vel = V2(0, 0);
                    return;
                }
                else if(collide(newBall.body, game->boundaries[3])){
                    game->score1 += 1;
                    game->ball->vel = V2(0, 0);
                    game->ball->body.pos = V2(0, 0);
                    game->player2->vel = V2(0, 0);
                    return;
                }
            }
        } while (remainAdvance >= 0.0000005f && bounces > 0);
        ASSERT(remainAdvance <= 0.0000005f);
        *game->ball = currentBall;
        game->ball->vel = currentBallDir * length(game->ball->vel);
    }
}

Game gameInterpolateSteps(Game * from, Game * to, f32 t)
{
    Game result = *to;
    
    result.player1->body.pos = lerp(&from->player1->body.pos, &to->player1->body.pos, t);
    result.player2->body.pos = lerp(&from->player2->body.pos, &to->player2->body.pos, t);
    result.ball->body.pos = lerp(&from->ball->body.pos, &to->ball->body.pos, t);
    result.ball->dir = slerp(&from->ball->dir, &to->ball->dir, t);

    return result;
}

void gameStep(f64 dt){
    (void)dt;
    game->player1Action = 0;
}

void gameExit(){
    platform->appRunning = false;
}
