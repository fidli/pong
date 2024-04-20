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

enum EntityType
{
    EntityType_Invalid,

    EntityType_StaticBlock,
    EntityType_Player,
    EntityType_Ball,

    EntityTypeCount
};

struct GLModel
{
    v2 scale;
    v2 pos;

    GLint glType;
    GLint glOffset;
    GLint glCount;
};

struct Entity {

    EntityType type;
    f32 mass;

    v2 size;
    v2 pos;
    v2 vel;

    Animation * animation;
    v4 overlayColor;
    ConvexHull body;

    GLModel model;
    GLModel wire;

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
            v2 acc;
        } ball;
    };
    
};

struct Game {
    f64 aiAccumulator;
    GameAction aiSteps[i32(AI_STEP/FIXED_STEP) + 1];
    i32 aiStepCount;
    i32 aiCurrentStep;

    AudioTrack track;

    Entity boundaries[6];
    Entity entities[4];
};

extern Game * game;

void makeQuad(GLModel* target, v2 scale);
void makeLinebox(GLModel* target, v2 scale);
void makeLinecircle(GLModel* target, v2 scale);

void gameInit(AudioTrack track){

    Entity* field = &game->entities[0];
    field->type = EntityType_StaticBlock;
    Entity* ball = &game->entities[1];
    ball->type = EntityType_Ball;
    Entity* player1 = &game->entities[2];
    player1->type = EntityType_Player;
    Entity* player2 = &game->entities[3];
    player2->type = EntityType_Player;

    {
        field->size = V2(170.0f, 90.0f);
        field->pos = V2(0.0f, 0.0f);

        makeQuad(&field->model, field->size);
        makeLinebox(&field->wire, field->size);
        field->overlayColor = V4(0, 1, 0, 0.1f);

        field->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&field->body, V2(0,0), field->size);
    }

    {
        player1->size = V2(6.0f, 6.0f);
        player1->pos = field->pos + V2(-field->size.x/2.0f + player1->size.x/2.0f + 1.0f, 0);
        player1->player.dir = V2(1.0f, 0);
        player1->player.wantDir = player1->player.dir;
        player1->player.rotationYRad = degToRad(180);
        player1->mass = 80.0f;

        makeQuad(&player1->model, player1->size);
        makeLinecircle(&player1->wire, player1->size);

        player1->body.points = &PPUSHA(v2, 41);
        // HERE
        makeCircleConvexHull(&player1->body, V2(0,0), player1->size.x/2.0f);
        //makeRectConvexHull(&player1->body, V2(0,0), player1->size);
    }

    {
        player2->size = V2(6.0f, 6.0f);
        player2->pos = field->pos + V2(field->size.x/2  - player2->size.x/2.0f - 1.0f, 0.0f);
        player2->player.dir = V2(-1.0f, 0);
        player2->player.wantDir = player2->player.dir;
        player2->player.rotationYRad = 0;//degToRad(0);
        player2->mass = 80.0f;

        makeQuad(&player2->model, player2->size);
        makeLinecircle(&player2->wire, player2->size);

        player2->body.points = &PPUSHA(v2, 41);
        makeCircleConvexHull(&player2->body, V2(0,0), player2->size.x/2.0f);
    }

    player1->animation = player2->animation = findAnimation("pig-idle");
    

    {

        ball->size = V2(1.0f, 1.0f);
        ball->pos = player1->pos + player1->player.dir * ((player1->size.x + ball->size.x)/2.0f);
        ball->ball.dir = player1->player.dir;
        ball->vel = V2(0, 0);
        ball->mass = 5.0f;

        makeLinecircle(&ball->model, ball->size);
        makeLinecircle(&ball->wire, ball->size);

        ball->body.points = &PPUSHA(v2, 41);
        makeCircleConvexHull(&ball->body, V2(0,0), ball->size.x/2.0f);
    }

    {
        Entity* entity = &game->boundaries[0];
        entity->size = V2(field->size.x, 10.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = field->pos + V2(0, field->size.y/2 +entity->size.y/2.0f);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
    // HERE
    /*
        entity->size = V2(10.0f, 10.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = player1->pos + V2(10.0f, 0.0f);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
        */
    }

    {
        Entity* entity = &game->boundaries[1];
        entity->size = V2(10.0f, field->size.y + 20.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = field->pos + V2(-field->size.x/2 -entity->size.x/2.0f, 0);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
    }
    {
        Entity* entity = &game->boundaries[2];
        entity->size = V2(field->size.x, 10.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = field->pos + V2(0, -field->size.y/2 -entity->size.y/2.0f);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
    }
    {
        Entity* entity = &game->boundaries[3];
        entity->size = V2(10.0f, field->size.y + 20.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = field->pos + V2(field->size.x/2 +entity->size.x/2.0f, 0);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
    }
    {
        Entity* entity = &game->boundaries[4];
        entity->size = V2(2.0f, field->size.y + 20.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = field->pos + V2(-field->size.x/2 + field->size.x/5 - 5.0f, 0);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
    }
    {
        Entity* entity = &game->boundaries[5];
        entity->size = V2(2.0f, field->size.y + 20.0f);
        entity->type = EntityType_StaticBlock;
        entity->pos = field->pos + V2(field->size.x/2.0f - field->size.x/5 + 5.0f, 0);
        makeQuad(&entity->model, entity->size);
        makeLinebox(&entity->wire, entity->size);

        entity->body.points = &PPUSHA(v2, 4);
        makeRectConvexHull(&entity->body, V2(0,0), entity->size);
    }


    game->track = track;
}

void gameHandleInput(){
    Entity* field = &game->entities[0];
    Entity* ball = &game->entities[1];
    Entity* player1 = &game->entities[2];
    Entity* player2 = &game->entities[3];

    if(keys[keymap[GameAction_Up].key].down){
        player1->player.action |= 1 << GameAction_Up;
    }
    if(keys[keymap[GameAction_Right].key].down){
        player1->player.action |= 1 << GameAction_Right;
    }
    if(keys[keymap[GameAction_Down].key].down){
        player1->player.action |= 1 << GameAction_Down;
    }
    if(keys[keymap[GameAction_Left].key].down){
        player1->player.action |= 1 << GameAction_Left;
    }
    if(keys[keymap[GameAction_Kick].key].down){
        player1->player.action |= 1 << GameAction_Kick;
    }
}

void gameFixedStep(f64 dt){
    Entity* field = &game->entities[0];
    Entity* ball = &game->entities[1];
    Entity* player1 = &game->entities[2];
    Entity* player2 = &game->entities[3];
    Entity* players[] = {player1, player2};
    game->aiAccumulator += dt;
    if (game->aiAccumulator >= AI_STEP)
    {
        i32 steps = 0;
        if (ball->vel.x > 0){
            v2 newPos = ball->pos + ball->vel*float(AI_STEP);
            f32 distance = newPos.y - player2->pos.y;
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
                player2->vel.y = -25.0f;
                player2->animation = findAnimation("pig-run");
            }break; 
            case GameAction_Down:
            {
                player2->vel.y = 25.0f;
                player2->animation = findAnimation("pig-run");
            }break;
            case GameAction_Stop:
            {
                player2->vel = V2(0.0f, 0.0f);
                player2->animation = findAnimation("pig-idle");
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
    if (player1->player.action & (1 << GameAction_Up))
    {
           playerDir += V2(0.0f, 1.0f);
    }
    if (player1->player.action & (1 << GameAction_Right))
    {
           playerDir += V2(1.0f, 0.0f);
           player1->player.rotationYRad = degToRad(180);
    }
    if (player1->player.action & (1 << GameAction_Down))
    {
           playerDir += V2(0.0f, -1.0f);
    }
    if (player1->player.action & (1 << GameAction_Left))
    {
           playerDir += V2(-1.0f, 0.0f);
           player1->player.rotationYRad = degToRad(0);
    }
    if (state)
    {
        playerDir = V2(state->position.x, state->position.y);
        if (state->buttons[0].down)
        {
            player1->player.action |= 1 << GameAction_Kick;
        }
    }


    v2 playerForces = {};
    if (length(playerDir) > 0){
        v2 dir = normalize(playerDir);
        player1->player.wantDir = dir;
        playerForces = dir * playerForceSize;
        player1->animation = findAnimation("pig-run");
    }

    if (dot(player1->player.dir, player1->player.wantDir) != 1.0f)
    {
        f32 speedDir = cross(player1->player.dir, player1->player.wantDir).z;
        if (speedDir == 0.0f){
            speedDir = 1.0f;
        }else{
            speedDir = speedDir/ABS(speedDir);
        }
        f32 playerForceAngularSize = playerForceSize;
        f32 angularAcc = playerForceAngularSize / ball->mass;
        player1->player.angularVel += angularAcc*CAST(f32, dt);
        player1->player.angularVel = clamp(player1->player.angularVel, 0.0f, 4.0f*PI);
        f32 advance = fmod(player1->player.angularVel * CAST(f32, dt), 2.0f*PI);
        v2 newDir = rotate(player1->player.dir, speedDir*advance);
        f32 newSpeedDir = cross(newDir, player1->player.wantDir).z;
        if(newSpeedDir * speedDir <= 0.0f){
            player1->player.dir = player1->player.wantDir;
            player1->player.angularVel = 0;
        }else{
            player1->player.dir = newDir;
        }
    }
    

    f32 g = 10.0; // m / s^2
    f32 grassFrictionCoef = 10.0f;
    if (!isTiny(player1->vel)){
        f32 FnPlayer = player1->mass * g;
        f32 FfrictionPlayer = grassFrictionCoef * FnPlayer;
        playerForces += -1*normalize(player1->vel)*FfrictionPlayer;
    }

    v2 oldVelocity = player1->vel;
    v2 acc = playerForces / player1->mass;
    player1->vel += acc * CAST(f32, dt);
    if (dot(oldVelocity, player1->vel) < 0 || isTiny(player1->vel)){
        player1->vel = V2(0, 0);
        player1->animation = findAnimation("pig-idle");
    }
    f32 maxVel = 80.0f;
    if (length(player1->vel) > maxVel){
        player1->vel = normalize(player1->vel) * maxVel;
    }

    // Player collisions against boundaries
    // There is no player to player collision, it should not happen
    for(i32 pi = 0; pi < ARRAYSIZE(players); pi++)
    {
        Entity* player = players[pi];
        ConvexHull playerMovementHull;
        playerMovementHull.count = player->body.count*2;
        playerMovementHull.points = &PUSHA(v2, playerMovementHull.count);
        for(u32 p = 0; p < player->body.count; p++)
        {
            playerMovementHull.points[p] = player->body.points[p];
        }
        i32 bounces = 5;
        v2 advance = player->vel * CAST(f32, dt);

        while(bounces && !isTiny(advance) && !isTiny(player->vel))
        {
            for(u32 p = player->body.count; p < playerMovementHull.count; p++)
            {
                playerMovementHull.points[p] = player->body.points[p-player->body.count] + advance;
            }

            v2 normal = V2(0, 0);
            v2 pop = V2(0, 0);
            f32 popLength = 0;
            for(i32 bi = 0; bi < ARRAYSIZE(game->boundaries); bi++)
            {
                Entity* boundary = &game->boundaries[bi];
                if (collide(player->pos, &playerMovementHull, boundary->pos, &boundary->body)){
                    v2 thisNormal = V2(0, 0);
                    v2 thisPop = collidePop(player->pos, &playerMovementHull, boundary->pos, &boundary->body, -advance, &thisNormal);
                    ASSERT(dot(thisPop, -advance) > 0);
                    f32 thisPopLength = length(thisPop);
                    f32 thisAdvanceLength = length(advance);
                    if(thisPopLength > thisAdvanceLength)
                    {
                        // nothing can rewind our movement fully further
                        pop = -advance;
                        popLength = thisAdvanceLength;
                        normal = thisNormal;
                        break;
                    }
                    if (thisPopLength > popLength)
                    {
                        pop = thisPop;
                        popLength = thisPopLength;
                        normal = thisNormal;
                    }

                }
            }
            player->pos += advance + pop;
            ASSERT(popLength <= length(advance));
            //advance = MIN(length(advance), popLength) * normalize(player->vel);
            advance = popLength * normalize(player->vel);
            if (!isTiny(advance))
            {
                advance = slide(advance, normal);
            }
            bounces--;
        }
        POP;
    }

    if(isTiny(ball->vel))
    {
        ball->pos = player1->pos + player1->player.dir * (player1->size.x + ball->size.x);
    }

    f32 ballWeight = 1.0; // kG
    f32 FnBall = ballWeight * g;
    f32 FfrictionBall = 0 * 0.08f * FnBall;
    ball->ball.acc -= normalize(ball->ball.dir)*(FfrictionBall / ballWeight);
    if((player1->player.action & (1 << GameAction_Kick)) && isTiny(ball->vel))
    {
        f32 force = 50000.0f;
        ball->ball.dir = player1->player.dir;
        ball->ball.acc += (force  / ballWeight)*normalize(ball->ball.dir);
        ball->vel = (ball->ball.acc + acc) * CAST(f32, INSTANT_DURATION) + player1->vel;
    }else{
        v2 newVel = ball->vel + ball->ball.acc * CAST(f32, dt);
        if(dot(newVel, ball->vel) > 0){
            ball->vel = newVel;
            ball->ball.dir = normalize(newVel);
        }
        else{
            ball->vel = V2(0, 0);
        }
    }
    ball->ball.acc = V2(0, 0);
    
    {
        Entity currentBall = *ball;
        f32 remainAdvance = length(ball->vel)*CAST(f32, dt);

        ConvexHull ballMovementHull;
        ballMovementHull.count = ball->body.count*2;
        ballMovementHull.points = &PUSHA(v2, ballMovementHull.count);

        i32 bounces = 5;
        Entity * bouncers[2] = {&game->boundaries[0], &game->boundaries[2]};

        // 0) Make movement convex hull
        // 1) collide against player and pop, update hulls & positions
        // 2) collide against bouncers and pop, update hulls & positions
        // 3) if collide with player directly -> pop player & stop
        // 4) if movement hull collide with score trigger -> score & new round
        // 5) if no collision in 1-4, do movement and exit, else goto 0) with remaining movement or stop after 5 iterations
        while (!isTiny(remainAdvance) && bounces > 0){
            // 0)

            v2 wantedMovement = currentBall.ball.dir*remainAdvance;
            for(u32 p = 0; p < ball->body.count; p++)
            {
                ballMovementHull.points[p] = currentBall.body.points[p];
                ballMovementHull.points[p+ball->body.count] = ballMovementHull.points[p] + wantedMovement;
            }
            bool collision = false;
            // 1)
            {
                bool thisCollision = false;
                v2 normal = V2(0, 0);
                v2 pop = V2(0, 0);
                f32 popLength = 0;
                bool volley = true;
                for(i32 i = 0; i < ARRAYSIZE(players); i++){
                    Entity* player = players[i];
                    ConvexHull* playerBody = &player->body;
                    if (collide(currentBall.pos, &ballMovementHull, player->pos, playerBody)){
                        // HERE continue
                        thisCollision = true;

                        v2 thisNormal = V2(0, 0);
                        bool thisVolley = isTiny(player->vel) || dot(currentBall.ball.dir, player->vel) <= 0;
                        v2 thisPop = collidePop(currentBall.pos, &ballMovementHull, player->pos, playerBody, (1-(2*volley))*(wantedMovement), &thisNormal);

                        f32 thisPopLength = length(thisPop);
                        f32 thisAdvanceLength = length(wantedMovement);
                        if (thisVolley)
                        {
                            if(thisPopLength > thisAdvanceLength)
                            {
                                // nothing can rewind our movement fully further
                                pop = -wantedMovement;
                                popLength = thisAdvanceLength;
                                normal = thisNormal;
                                volley = thisVolley;
                                break;
                            }
                        }
                        if (thisPopLength > popLength)
                        {
                            pop = thisPop;
                            popLength = thisPopLength;
                            volley = thisVolley;
                            normal = thisNormal;
                        }
                    }
                }
                
                if (thisCollision)
                {
                    if (volley)
                    {
                        currentBall.ball.dir = reflect(currentBall.ball.dir, normal);
                        currentBall.pos += wantedMovement + pop;
                        remainAdvance = popLength;
                    }
                    else
                    {
                        // TODO player addded energy to the ball if not volley? 
                        currentBall.pos += pop;
                        remainAdvance = MAX(0.0f, remainAdvance - popLength);
                    }
                    for(i32 i = 0; i < ARRAYSIZE(players); i++){
                        Entity* player = players[i];
                        ConvexHull* playerBody = &player->body;
                        ASSERT(!collide(currentBall.pos, &currentBall.body, player->pos, playerBody));
                    }

                    collision = true;
                    // TODO consider bounces & audio only on whole loop?
                    bounces--;
                    // TODO consider audio only after loop, if we colided?
                    playAudio(&game->track);

                    wantedMovement = currentBall.ball.dir*remainAdvance;
                    for(u32 p = 0; p < ball->body.count; p++)
                    {
                        ballMovementHull.points[p] = currentBall.body.points[p];
                        ballMovementHull.points[p+ball->body.count] = ballMovementHull.points[p] + wantedMovement;
                    }
                }
            }

            // 2)
            if(!isTiny(remainAdvance)){
                bool thisCollision = false;
                v2 normal = V2(0, 0);
                v2 pop = V2(0, 0);
                f32 popLength = 0;
                for(i32 i = 0; i < ARRAYSIZE(bouncers); i++){
                    Entity* bouncer = bouncers[i];
                    if (collide(currentBall.pos, &ballMovementHull, bouncer->pos, &bouncer->body)){
                        thisCollision = true;
                        v2 thisNormal = V2(0, 0);
                        v2 thisPop = collidePop(currentBall.pos, &ballMovementHull, bouncer->pos, &bouncer->body, -(currentBall.ball.dir*remainAdvance), &thisNormal);
                        ASSERT(dot(thisPop, -wantedMovement) > 0);
                        f32 thisPopLength = length(thisPop);
                        f32 thisAdvanceLength = length(wantedMovement);
                        if(thisPopLength > thisAdvanceLength)
                        {
                            // nothing can rewind our movement fully further
                            pop = -wantedMovement;
                            popLength = thisAdvanceLength;
                            normal = thisNormal;
                            break;
                        }
                        if (thisPopLength > popLength)
                        {
                            pop = thisPop;
                            popLength = thisPopLength;
                            normal = thisNormal;
                        }
                    }
                }
                if (thisCollision)
                {
                    currentBall.ball.dir = reflect(currentBall.ball.dir, normal);
                    currentBall.pos += wantedMovement + pop;
                    remainAdvance = popLength;

                    collision = true;
                    // TODO consider bounces & audio only on whole loop?
                    bounces--;
                    // TODO consider audio only after loop, if we colided?
                    playAudio(&game->track);

                    wantedMovement = currentBall.ball.dir*remainAdvance;
                    for(u32 p = 0; p < ball->body.count; p++)
                    {
                        ballMovementHull.points[p] = currentBall.body.points[p];
                        ballMovementHull.points[p+ball->body.count] = ballMovementHull.points[p] + wantedMovement;
                    }
                }
            }

            // 3)
            for(i32 i = 0; i < ARRAYSIZE(players); i++){
                Entity* player = players[i];
                ConvexHull* playerBody = &player->body;
                if (collide(currentBall.pos, &currentBall.body, player->pos, playerBody)){
                    collision = true;
                    v2 normal = V2(0, 0);
                    v2 pop = collidePop(currentBall.pos, &currentBall.body, player->pos, &player->body, currentBall.pos - player->pos, &normal);
                    // players start at index 2
                    player->pos -= pop;
                    ASSERT(!collide(currentBall.pos, &currentBall.body, player->pos, playerBody));
                    currentBall.vel = V2(0, 0);
                    currentBall.ball.acc = V2(0, 0);
                    remainAdvance = 0;
                    bounces--;
                    break;
                }
            }

            // 4) 
            if (!collision){
                remainAdvance = 0;
                if (collide(currentBall.pos, &ballMovementHull, game->boundaries[1].pos, &game->boundaries[1].body)){
                    player2->player.score += 1;
                    ball->vel = V2(0, 0);
                    ball->pos = V2(0, 0);
                    player2->vel = V2(0, 0);
                    POP;
                    return;
                }
                else if (collide(currentBall.pos, &ballMovementHull, game->boundaries[3].pos, &game->boundaries[3].body)){
                    player1->player.score += 1;
                    ball->vel = V2(0, 0);
                    ball->pos = V2(0, 0);
                    player2->vel = V2(0, 0);
                    POP;
                    return;
                }
                
                // 5)
                currentBall.pos += wantedMovement;
            }
        }

    // HERE
    // original
    /*
    if (!isTiny(ball->vel)){
        f32 remainAdvance = length(ball->vel)*CAST(f32, dt);
        i32 bounces = 5;
        Entity * bouncers[2] = {&game->boundaries[0], &game->boundaries[2]};

        Entity currentBall = *ball;
        Entity lastBall = currentBall;
        // 0) Do movement
        // 1) collide against player and pop
        // 2) collide against bouncers and pop
        // 3) if collide with player -> pop player & stop
        // 4) goto 0) with remaining movement or stop after 5 iterations
        do{
            Entity newBall = currentBall;
            // 0)
            newBall.pos += currentBall.ball.dir*remainAdvance;
            // 1)
            bool collision = false;
            for(i32 i = 0; i < ARRAYSIZE(players); i++){
                Entity* player = players[i];
                ConvexHull* playerBody = &player->body;
                if (collide(newBall.pos, &newBall.body, player->pos, playerBody)){
                    collision = true;

                    v2 normal = V2(0, 0);
                    bool volley = isTiny(player->vel) || dot(currentBall.ball.dir, player->vel) <= 0;
                    v2 pop = collidePop(newBall.pos, &newBall.body, player->pos, playerBody, (1-(2*volley))*(currentBall.ball.dir*remainAdvance), &normal);
                    currentBall.pos = newBall.pos + pop;
                    if (volley)
                    {
                        currentBall.ball.dir = reflect(currentBall.ball.dir, normal);
                    }
                    ASSERT(!collide(currentBall.pos, &currentBall.body, player->pos, playerBody));
                    remainAdvance = clamp(length(pop), 0.0f, remainAdvance);
                    bounces--;
                    playAudio(&game->track);
                    newBall = currentBall;
                    break;
                }
            }

            // 2)
            for(i32 i = 0; i < ARRAYSIZE(bouncers) && !isTiny(remainAdvance); i++){
                Entity* bouncer = bouncers[i];
                if (collide(newBall.pos, &newBall.body, bouncer->pos, &bouncer->body)){
                    collision = true;
                    v2 normal = V2(0, 0);
                    v2 pop = collidePop(newBall.pos, &newBall.body, bouncer->pos, &bouncer->body, -(currentBall.ball.dir*remainAdvance), &normal);
                    currentBall.pos = newBall.pos + pop;
                    currentBall.ball.dir = reflect(currentBall.ball.dir, normal);
                    ASSERT(!collide(currentBall.pos, &currentBall.body, bouncer->pos, &bouncer->body));
                    remainAdvance = clamp(length(pop), 0.0f, remainAdvance);
                    bounces--;
                    playAudio(&game->track);
                    newBall = currentBall;
                    break;
                }
            }

            // 3)
            for(i32 i = 0; i < ARRAYSIZE(players); i++){
                Entity* player = players[i];
                ConvexHull* playerBody = &player->body;
                if (collide(newBall.pos, &newBall.body, player->pos, playerBody)){
                    collision = true;
                    v2 normal = V2(0, 0);
                    v2 pop = collidePop(newBall.pos, &newBall.body, player->pos, &player->body, currentBall.pos - player->pos, &normal);
                    currentBall = newBall;
                    // players start at index 2
                    player->pos -= pop;
                    ASSERT(!collide(currentBall.pos, &currentBall.body, player->pos, playerBody));
                    currentBall.vel = V2(0, 0);
                    currentBall.ball.acc = V2(0, 0);
                    remainAdvance = 0;
                    bounces--;
                    break;
                }
            }

            if (!collision){
                remainAdvance = 0;
                currentBall = newBall;
                if (collide(newBall.pos, &newBall.body, game->boundaries[1].pos, &game->boundaries[1].body)){
                    player2->player.score += 1;
                    ball->vel = V2(0, 0);
                    ball->pos = V2(0, 0);
                    player2->vel = V2(0, 0);
                    return;
                }
                else if (collide(newBall.pos, &newBall.body, game->boundaries[3].pos, &game->boundaries[3].body)){
                    player1->player.score += 1;
                    ball->vel = V2(0, 0);
                    ball->pos = V2(0, 0);
                    player2->vel = V2(0, 0);
                    return;
                }
            }

        } while (!isTiny(remainAdvance) && bounces > 0);
        */


        *ball = currentBall;
        ball->vel = currentBall.ball.dir * length(ball->vel);
        POP;
    }


}

Game gameInterpolateStepsForRendering(Game * from, Game * to, f32 t)
{
    Game result = *to;
    
    // 0 is field, no need to lerp/slepr
    for (i32 i = 0; i < ARRAYSIZE(game->entities); i++){
        Entity* entity = &game->entities[i];
        result.entities[i].pos = lerp(&from->entities[i].pos, &to->entities[i].pos, t);
        switch (entity->type)
        {
            case EntityType_Player:
            {
                result.entities[i].player.dir = slerp(&from->entities[i].player.dir, &to->entities[i].player.dir, t);
            }break;
            case EntityType_Ball:
            {
                result.entities[i].ball.dir = slerp(&from->entities[i].ball.dir, &to->entities[i].ball.dir, t);
            }break;
            case EntityType_StaticBlock:
            {
            }break;
            default:
            {
                INV;
            }break;
        }
    }

    return result;
}

void gameStep(f64 dt){
    Entity* field = &game->entities[0];
    Entity* ball = &game->entities[1];
    Entity* player1 = &game->entities[2];
    Entity* player2 = &game->entities[3];
    (void)dt;
    player1->player.action = 0;
}

void gameExit(){
    platform->appRunning = false;
}
