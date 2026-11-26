#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>

#define WINDOWWIDTH 800
#define WINDOWHEIGHT 488
#define START_OUTER_BOX_BUFFER 10

#define MAX_BALLS 20
#define MAX_LINES 30

#define WORLD_FRICTION 0
#define WORLD_GRAVITY 0//90
#define CORRECTION_BUFFER 0.1f

#define DEFAULT_RESTESTUTION 0.8

void 你好() {
    printf("你好\n");
}

//================Struct Definitions================
typedef struct Wall2D {
    Vector2 start;
    Vector2 end;
}Wall2D;

typedef struct KineticCircle2D {
    Vector2 pos;
    Vector2 vel;
    float radius;

    float restitution;

    bool selected;
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

void ApplyGravity(KineticCircle2D *ball, float gravity, float delta) {
    ball->vel.y = ball->vel.y + gravity * delta;
}

void BounceCircleOffNormal(KineticCircle2D *ball, Vector2 n) {
    float d = Vector2DotProduct(ball->vel, n);

    // Only respond if moving into the wall
    if (d >= 0) return;

    Vector2 vn = Vector2Scale(n, d);              // normal component
    Vector2 vt = Vector2Subtract(ball->vel, vn);  // tangent component

    vt = Vector2Scale(vt, 1.0f - WORLD_FRICTION);       // friction slows sliding
    vn = Vector2Scale(vn, -ball->restitution);          // flip and scale bounce

    ball->vel = Vector2Add(vt, vn);
}

void PointBounce(KineticCircle2D *ball, Vector2 point) {
    Vector2 diff = Vector2Subtract(ball->pos, point);
    float dist = Vector2Length(diff);

    if (dist >= ball->radius) { return; }

    Vector2 n = Vector2Scale(Vector2Subtract(ball->pos, point), 1.0f / dist);

    float pentration = ball->radius - Vector2Distance(ball->pos, point);

    ball->pos = Vector2Add(ball->pos, Vector2Scale(n, pentration));

    BounceCircleOffNormal(ball, n);

    return;
}

void WallCollision(Wall2D line, KineticCircle2D *ball, float friction) {
    Vector2 lineNormal = GetLineNormal(line);

    float dist;
    Vector2 toBall;

    Vector2 v1 = {ball->pos.x-line.start.x, ball->pos.y-line.start.y};
    Vector2 v2 = {-lineNormal.y, lineNormal.x};
    float distAlongLine = Vector2DotProduct(v1, v2);

    toBall = Vector2Subtract(ball->pos, line.start);
    if (Vector2DotProduct(lineNormal, toBall) < 0) {
        lineNormal = Vector2Negate(lineNormal); //Correct direction to face object
    }

    dist = Vector2DotProduct(toBall, lineNormal);

    if (dist < ball->radius) {

        if (distAlongLine < 0) { //Relative Left of Line
            PointBounce(ball, line.start);
            return;
        }
        if (distAlongLine > Vector2Distance(line.start, line.end)) { //Relative Left of Line
            PointBounce(ball, line.end);
            return;
        }

        BounceCircleOffNormal(ball, lineNormal); //On line middle
        return;
    }
}

void BallCollision(KineticCircle2D *ball1, KineticCircle2D *ball2, float friction) {
    float dist = Vector2Distance(ball1->pos, ball2->pos);

    if (dist > (ball1->radius + ball2->radius)) { return; }
    printf("Collision: %p -> %p\n", ball1, ball2);

    Vector2 normal;
    normal.x = (ball1->pos.x - ball2->pos.x) / dist;
    normal.y = (ball1->pos.y - ball2->pos.y) / dist;

    //Eliminate Overlap
    float overlap = dist - ball1->radius - ball2->radius;
    if (overlap >= CORRECTION_BUFFER) {
        ball1->pos = Vector2Add(ball1->pos, Vector2Scale(normal, -overlap));
        ball2->pos = Vector2Add(ball2->pos, Vector2Scale(normal, overlap));
    }

    float vn = Vector2DotProduct(ball1->vel, normal);
    Vector2 temp = {normal.y, -normal.x};
    float vt = Vector2DotProduct(ball1->vel, temp);

    //Sliding force
    Vector2 force;
    force.x = vt/2 * normal.y * friction;
    force.y = vt/2 * -normal.x * friction;

    //Elastic + Inelastic Collision
    force.x += vn/2 * normal.x * (1 + (ball1->restitution + ball2->restitution)/2);
    force.y += vn/2 * normal.y * (1 + (ball1->restitution + ball2->restitution)/2);

    ball1->vel = Vector2Subtract(ball1->vel, force);
    ball2->vel = Vector2Add(ball2->vel, force);

    return;
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
    //=========================Initialize Variables=========================
    你好();
    Rectangle outerBox;
    outerBox.x = START_OUTER_BOX_BUFFER;
    outerBox.y = START_OUTER_BOX_BUFFER;
    outerBox.height = WINDOWHEIGHT - START_OUTER_BOX_BUFFER * 2;
    outerBox.width = WINDOWWIDTH - START_OUTER_BOX_BUFFER * 2;
    printf("Outer box init\n");
    
    Wall2D lines[MAX_LINES];
    int linesSpawned = 0;
    bool spawningLine = false;
    Vector2 newLineStart;

    Wall2D defaultLine = { {300, 400}, {250, 175} };
    printf("Wall init\n");
    
    KineticCircle2D defaultBall = { {WINDOWWIDTH / 2, WINDOWHEIGHT / 2}, {0, 0}, 10, DEFAULT_RESTESTUTION};

    KineticCircle2D balls[MAX_BALLS];
    int ballsSpawned = 0;
    printf("Ball init\n");

    float deltatime;

    InitWindow(WINDOWWIDTH, WINDOWHEIGHT, "Physics Engine");

    Vector2 mousePosition;
    printf("Main loop begin\n");

    int alreadyCollided[MAX_BALLS];

    //=====================================Main Loop=====================================
    while (!WindowShouldClose()) {
        deltatime = GetFrameTime();
        mousePosition = GetMousePosition();

        //=================Input=================
            // Move balls
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                for (int i = 0; i <= ballsSpawned; i++) {
                    if (!balls[i].selected && Vector2Distance(balls[i].pos, mousePosition) <= balls[i].radius){
                    balls[i].selected = true;
                }
                }
            } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                for (int i = 0; i <= ballsSpawned; i++) {

                    if (balls[i].selected) {
                        balls[i].selected = false;
                        balls[i].vel = Vector2Add(Vector2Subtract(mousePosition, balls[i].pos), balls[i].vel);
                    }
                }
                
            }

            //Spawn balls
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                if (ballsSpawned < MAX_BALLS - 1) {
                    balls[ballsSpawned + 1] = defaultBall;
                    balls[ballsSpawned + 1].pos = mousePosition;
                    ballsSpawned++;
                    printf("Spawn ball %d\n", ballsSpawned);
                }
            }

            //Spawn Lines
            if (IsKeyPressed(KEY_SPACE)) {
                if (!spawningLine && linesSpawned <= MAX_LINES) {
                    printf("spawning line\n");
                    newLineStart = mousePosition;
                    spawningLine = true;
                }
            }
            if (IsKeyReleased(KEY_SPACE)) {
                if (spawningLine) {
                    lines[linesSpawned + 1] = defaultLine;
                    lines[linesSpawned + 1].start = newLineStart;
                    lines[linesSpawned + 1].end = mousePosition;
                    linesSpawned++;
                    spawningLine = false;
                }
            }

            //Despawn all
            if (IsKeyPressed(KEY_C)) {
                ballsSpawned = 0;
                linesSpawned = 0;
            }

        //====================Rendering====================
        BeginDrawing();

            ClearBackground(BLACK);

            //Draw outer box
            DrawRectangleRec(outerBox, SKYBLUE);
            DrawCircle(outerBox.x, outerBox.y, 5, BLUE);
            DrawCircle(outerBox.x + outerBox.width, outerBox.y, 5, BLUE);
            DrawCircle(outerBox.x + outerBox.width, outerBox.y + outerBox.height, 5, BLUE);
            DrawCircle(outerBox.x, outerBox.y + outerBox.height, 5, BLUE);
            
            //Draw lines
            for (int i = 0; i <= linesSpawned; i++) {
                DrawLineV(lines[i].start, lines[i].end, WHITE);
            }
            if (spawningLine) {
                DrawLineV(newLineStart, mousePosition, RED);
            }

            //Draw balls
            for (int i = 0; i <= ballsSpawned; i++){
                if (!balls[i].selected) {
                    DrawCircle(balls[i].pos.x, balls[i].pos.y, balls[i].radius, (Color){255/ (i + 1), 255/ (i + 1), 255/ (i + 1), 255});
                } else {
                    DrawCircle(balls[i].pos.x, balls[i].pos.y, balls[i].radius, RED);
                    DrawLineV(GetMousePosition(), balls[i].pos, RED);
                }
            }

            DrawFPS(0,0);

        EndDrawing();

        //=======================Apply physics========================

        memset(alreadyCollided, -1, sizeof(alreadyCollided));
            for (int i = 0; i <= ballsSpawned; i++) {

            OuterBoxCollision(outerBox, &balls[i], WORLD_FRICTION);

            //Line Collision
            for (int j = 0; j <= linesSpawned; j++) {
                WallCollision(lines[j], &balls[i], WORLD_FRICTION);
            }

            //Ball Collision
            for (int j = 0; j <= ballsSpawned - 1; j++) {
                if (&balls[i] == &balls[j]) { continue; }
                BallCollision(&balls[i], &balls[j], WORLD_FRICTION);
                
            }

            UpdatePosition(&balls[i], deltatime);
        
            ApplyGravity(&balls[i], WORLD_GRAVITY, deltatime);

            }
    }

    CloseWindow();

    return 0;
}