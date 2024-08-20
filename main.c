#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define MAX_NUM_ENTITIES 500

typedef enum EntityState {
    ENTITY_STATE_INACTIVE,
    ENTITY_STATE_ACTIVE,
} EntityState;

typedef struct Entity {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 direction;
    EntityState state;
    float radius;
} Entity;

typedef struct Flock {
    Entity entities[MAX_NUM_ENTITIES];
    Vector2 seekTarget;
    int shouldSeekTarget;
    float maxSpeed;
    float maxAlignForce;
    float maxCohesionForce;
    float maxSeparationForce;
    float maxSeekForce;
    float awarenessRadius;
    float separationRadius;
    int renderHelpers;
} Flock;

void InitFlock(Flock *flock, Vector2 center, EntityState state) {
    for (int i = 0; i < MAX_NUM_ENTITIES; i++) {
        Entity *entity = &flock->entities[i];
        entity->radius = 5;
        entity->position.x = center.x + GetRandomValue(-entity->radius * 10, entity->radius * 10);
        entity->position.y = center.y + GetRandomValue(-entity->radius * 10, entity->radius * 10);
        entity->acceleration.x = 0;
        entity->acceleration.y = 0;
        entity->direction.x = 0;
        entity->direction.y = 0;
        entity->state = state;
        entity->velocity.x = GetRandomValue(-flock->maxSpeed, flock->maxSpeed);
        entity->velocity.y = GetRandomValue(-flock->maxSpeed, flock->maxSpeed);
    }
}

void UpdateFlock(Flock *flock) {
    if (IsKeyPressed(KEY_H)) {
        flock->renderHelpers = flock->renderHelpers ? 0 : 1;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        flock->seekTarget = GetMousePosition();
        flock->shouldSeekTarget = 1;
    } else {
        flock->shouldSeekTarget = 0;
    }

    Vector2 align = {0, 0};
    Vector2 cohesion = {0, 0};
    Vector2 separation = {0, 0};
    Vector2 seek = {0, 0};
    Vector2 wander = {0, 0};

    for (int i = 0; i < MAX_NUM_ENTITIES; i++) {
        Entity *entity = &flock->entities[i];

        if (entity->state == ENTITY_STATE_INACTIVE) continue;

        int awarenessNeighboursCount = 0;
        int toCloseNeighboursCount = 0;

        for (int j = 0; j < MAX_NUM_ENTITIES; j++) {
            if (j == i || flock->entities[j].state == ENTITY_STATE_INACTIVE) continue;

            Entity *other = &flock->entities[j];
            float distanceToOther = Vector2Distance(entity->position, other->position);

            if (distanceToOther > 0 && distanceToOther <= flock->awarenessRadius) {
                align = Vector2Add(align, other->velocity);
                cohesion = Vector2Add(cohesion, other->position);
                awarenessNeighboursCount++;
            }

            if (distanceToOther >0 && distanceToOther <= flock->separationRadius) {
                Vector2 diff = Vector2Subtract(entity->position, other->position);
                diff = Vector2Normalize(diff);
                diff.x /= distanceToOther;
                diff.y /= distanceToOther;
                separation = Vector2Add(separation, diff);
                toCloseNeighboursCount++;
            }
        }

        if (awarenessNeighboursCount > 0) {
            align.x /= awarenessNeighboursCount;
            align.y /= awarenessNeighboursCount;
            align = Vector2ClampValue(align, -flock->maxSpeed, flock->maxSpeed);
            align = Vector2Subtract(align, entity->velocity);
            align = Vector2Normalize(align);
            align = Vector2Scale(align, flock->maxAlignForce);

            cohesion.x /= awarenessNeighboursCount;
            cohesion.y /= awarenessNeighboursCount;
            cohesion = Vector2Subtract(cohesion, entity->position);
            cohesion = Vector2ClampValue(cohesion, -flock->maxSpeed, flock->maxSpeed);
            cohesion = Vector2Subtract(cohesion, entity->velocity);
            cohesion = Vector2Normalize(cohesion);
            cohesion = Vector2Scale(cohesion, flock->maxCohesionForce);
        }

        if (toCloseNeighboursCount > 0) {
            separation.x /= toCloseNeighboursCount;
            separation.y /= toCloseNeighboursCount;
            separation = Vector2Normalize(separation);
            separation = Vector2Scale(separation, flock->maxSpeed);
            separation = Vector2Subtract(separation, entity->velocity);
            separation = Vector2Normalize(separation);
            separation = Vector2Scale(separation, flock->maxSeparationForce);
        }

        if (flock->shouldSeekTarget) {
            float distanceToMouse = Vector2Distance(entity->position, flock->seekTarget);
            if (distanceToMouse > entity->radius) {
                seek = Vector2Subtract(flock->seekTarget, entity->position);
                seek = Vector2Normalize(seek);
                seek = Vector2Scale(seek, flock->maxSpeed);
                seek = Vector2Subtract(seek, entity->velocity);
                seek = Vector2Normalize(seek);
                seek = Vector2Scale(seek, flock->maxSeekForce);
            }
        }

        entity->acceleration.x += align.x;
        entity->acceleration.y += align.y;

        entity->acceleration.x += cohesion.x;
        entity->acceleration.y += cohesion.y;

        entity->acceleration.x += separation.x;
        entity->acceleration.y += separation.y;

        entity->acceleration.x += seek.x;
        entity->acceleration.y += seek.y;

        entity->acceleration.x += wander.x;
        entity->acceleration.y += wander.y;

        if (entity->position.x + entity->radius >= GetScreenWidth() || entity->position.x - entity->radius <= 0) {
            entity->velocity.x *= -1;
        }

        if (entity->position.y + entity->radius >= GetScreenHeight() || entity->position.y - entity->radius <= 0) {
            entity->velocity.y *= -1;
        }

        entity->velocity.x += entity->acceleration.x;
        entity->velocity.y += entity->acceleration.y;
        entity->direction = Vector2Normalize(entity->velocity);
        entity->position.x += entity->velocity.x * GetFrameTime();
        entity->position.y += entity->velocity.y * GetFrameTime();
        entity->acceleration.x = 0;
        entity->acceleration.y = 0;
    }
}

int main() {
    Flock flock;
    flock.seekTarget.x = 0;
    flock.seekTarget.y = 0;
    flock.shouldSeekTarget = 0;
    flock.renderHelpers = 0;
    flock.maxSpeed = 350;
    flock.maxAlignForce = 0.1;
    flock.maxCohesionForce = 0.1;
    flock.maxSeparationForce = 1;
    flock.maxSeekForce = 4;
    flock.awarenessRadius = 50;
    flock.separationRadius = 15;

    InitWindow(1024, 1024, "Boids");
    SetTargetFPS(144);

    Vector2 screenCenter = { GetScreenWidth() / 2, GetScreenHeight() / 2 };
    InitFlock(&flock, screenCenter, ENTITY_STATE_ACTIVE);

    while(!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            InitFlock(&flock, screenCenter, ENTITY_STATE_ACTIVE);
        }

        UpdateFlock(&flock);

        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < MAX_NUM_ENTITIES; i++) {
            Entity entity = flock.entities[i];

            if (entity.state == ENTITY_STATE_INACTIVE) continue;

            DrawCircleV(entity.position, entity.radius, YELLOW);
            DrawCircleLinesV(entity.position, entity.radius, RED);

            if (flock.renderHelpers) {
                DrawCircleLinesV(entity.position, flock.awarenessRadius, (Color){ 255, 255, 255, 50});

                DrawCircleLinesV(entity.position, flock.separationRadius, (Color){ 255, 0, 0, 100});

                Vector2 targetPos = flock.shouldSeekTarget ? flock.seekTarget : Vector2Add(entity.position, Vector2Scale(entity.direction, flock.awarenessRadius));
                DrawLineV(entity.position, targetPos, (Color){ 255, 255, 255, 50});
            }
        }

        if (flock.shouldSeekTarget) {
            DrawCircleV(flock.seekTarget, 5, WHITE);
        }

        char text[100];
        sprintf(text, "FPS %d Entities %d Speed %0.1f Align %0.2f Cohesion %0.2f Separation %0.2f Seek %0.2f", GetFPS(), MAX_NUM_ENTITIES, flock.maxSpeed, flock.maxAlignForce, flock.maxCohesionForce, flock.maxSeparationForce, flock.maxSeekForce);
        DrawText(text, 10, 10, 18, WHITE);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}