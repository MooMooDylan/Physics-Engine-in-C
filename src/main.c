#include "raylib.h"
#include "raymath.h"
#include <stdio.h>

#define WINDOWWIDTH 800
#define WINDOWHEIGHT 488
#define START_OUTER_BOX_BUFFER 10

#define WORLD_FRICTION 0.2
#define WORLD_GRAVITY 2

#define DEFAULT_RESTESTUTION 0.8

typedef struct Wall2D {
    Vector2 start;
    Vector2 end;
}Wall2D;

typedef struct KineticCircle2D {
    Vector2 pos;
    Vector2 vel;
    float radius;

    float restitution;
} KineticCircle2D;

Vector2 GetLineMidpoint(Wall2D line) {
    Vector2 mid = {(line.start.x + line.end.x) / 2, (line.start.y + line.end.y) / 2};
    return mid;
}

Vector2 GetLineNormal(Wall2D line) {
    Vector2 dir = Vector2Subtract(line.start, line.end);
    Vector2 normal = { -dir.y, dir.x };
    return Vector2Normalize(normal);
}

void UpdatePosition(KineticCircle2D *ball, float delta) {
    ball->pos.x = ball->pos.x + ball->vel.x * delta;
    ball->pos.y = ball->pos.y + ball->vel.y * delta;
}

void ApplyGravity(KineticCircle2D *ball, float gravity) {
    ball->vel.y = ball->vel.y + gravity;
}

void WallCollision(Wall2D line, KineticCircle2D *ball, float friction) {
    Vector2 lineNormal = GetLineNormal(line);

    float dist;
    Vector2 toBall;

    toBall = Vector2Subtract(ball->pos, line.start);
    if (Vector2DotProduct(lineNormal, toBall) < 0) {
        lineNormal = Vector2Negate(lineNormal); //Correct direction to face object
    }

    dist = Vector2DotProduct(toBall, lineNormal);

    if (dist < ball->radius) {

        float d = Vector2DotProduct(ball->vel, lineNormal);

        if (d >= 0) return;

        Vector2 vn = Vector2Scale(lineNormal, d);
        Vector2 vt = Vector2Subtract(ball->vel, vn);

        vt = Vector2Scale(vt, 1.0f - friction);
        vn = Vector2Scale(vn, -ball->restitution);

        ball->vel = Vector2Add(vt, vn);

        float dist = Vector2DotProduct(toBall, lineNormal);

        ball->pos.x += lineNormal.x * -(dist - ball->radius);
        ball->pos.y += lineNormal.y * -(dist - ball->radius);
    
    } else { return; }
}

void OuterBoxCollision(Rectangle outerBox, KineticCircle2D *ball, float friction) {
    //Floor
    if (ball->pos.y + ball->radius >= outerBox.height+outerBox.y) {
        if (ball->vel.y > 0) {
            ball->vel.y = -ball->vel.y * ball->restitution;
            ball->vel.x = ball->vel.x * (1.0f - friction);
            ball->pos.y = outerBox.height+outerBox.y - ball->radius;
        }
    }
    //Ceiling
    if (ball->pos.y - ball->radius <= outerBox.y) {
        if (ball->vel.y < 0) {
            ball->vel.y = -ball->vel.y * ball->restitution;
            ball->vel.x = ball->vel.x * (1.0f - friction);
            ball->pos.y = outerBox.y + ball->radius;
        }
    }
    //Right wall
    if (ball->pos.x + ball->radius >= outerBox.width+outerBox.x) {
        if (ball->vel.x > 0) {
            ball->vel.x = -ball->vel.x * ball->restitution;
            ball->vel.y = ball->vel.y * (1.0f - friction);
            ball->pos.x = outerBox.width+outerBox.x - ball->radius;
        }
    }
    //Left wall
    if (ball->pos.x - ball->radius <= outerBox.x) {
        if (ball->vel.x < 0) {
            ball->vel.x = -ball->vel.x * ball->restitution;
            ball->vel.y = ball->vel.y * (1.0f - friction);
            ball->pos.x = outerBox.x + ball->radius;
        }
    }
}

int main(void) {
    Rectangle outerBox;
    outerBox.x = START_OUTER_BOX_BUFFER;
    outerBox.y = START_OUTER_BOX_BUFFER;
    outerBox.height = WINDOWHEIGHT - START_OUTER_BOX_BUFFER * 2;
    outerBox.width = WINDOWWIDTH - START_OUTER_BOX_BUFFER * 2;

    Wall2D line = { {300, 400}, {250, 175} };
    
    KineticCircle2D ball = { {WINDOWWIDTH / 2, WINDOWHEIGHT / 2}, {-80, 20}, 10, DEFAULT_RESTESTUTION};

    float deltatime;

    InitWindow(WINDOWWIDTH, WINDOWHEIGHT, "Physics Engine");

    while (!WindowShouldClose()) {
        deltatime = GetFrameTime();

        BeginDrawing();

            ClearBackground(BLACK);

            //Draw outer box
            DrawRectangleRec(outerBox, SKYBLUE);
            DrawCircle(outerBox.x, outerBox.y, 5, BLUE);
            DrawCircle(outerBox.x + outerBox.width, outerBox.y, 5, BLUE);
            DrawCircle(outerBox.x + outerBox.width, outerBox.y + outerBox.height, 5, BLUE);
            DrawCircle(outerBox.x, outerBox.y + outerBox.height, 5, BLUE);

            DrawLine(line.start.x, line.start.y, line.end.x, line.end.y, WHITE);

            DrawCircle(ball.pos.x, ball.pos.y, ball.radius, WHITE);

            DrawFPS(0,0);

        EndDrawing();

        UpdatePosition(&ball, deltatime);

        OuterBoxCollision(outerBox, &ball, WORLD_FRICTION);

        //Line Collision
        WallCollision(line, &ball, WORLD_FRICTION);
        
        ApplyGravity(&ball, WORLD_GRAVITY);
    }

    CloseWindow();

    return 0;
}