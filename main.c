#include <stdio.h>
#include <stdlib.h>
#include "raylib.h"

#define CAR_WIDTH 50
#define CAR_HEIGHT 50

#define MIN_ROAD_WIDTH (CAR_WIDTH * 4)          /* Min width of any randomly generated road */
#define MAX_ROAD_WIDTH (CAR_WIDTH * 6)          /* Max width of any randomly generated road */

#define MIN_GAP_WIDTH (CAR_WIDTH * 1.7)         /* Min width of the gap linking two consecutive roads */

#define ROAD_COUNT_IN_SCREEN 3                  /* Number of roads in the screen, there are total (ROAD_COUNT_IN_SCREEN * 2) nodes in the roadList */

#define COLLISION_TOLERANCE 5                   /* Maximum number of pixels between the car and roads to trigger collision */

/* Colors */
#define ROAD_COLOR WHITE
#define CAR_COLOR WHITE

typedef struct Road
{
    Vector2 topLeft;
    Vector2 topRight;
    int height;
} Road;

typedef struct RoadNode         /* Nodes of a doubly linked list */
{
    Road *road;
    struct RoadNode *next;
    struct RoadNode *prev;
} RoadNode;

typedef struct Car
{
    Vector2 pos;                /* Top left position of the car */
} Car;


static const int screenWidth = 1280;
static const int screenHeight = 800;

static const int targetFPS = 60;

static const int roadHeight = screenHeight / ROAD_COUNT_IN_SCREEN;

static const float carSpeedVerticalBase = 240;
static float carSpeedVertical = carSpeedVerticalBase;
static const int carSpeedHorizontal = 300;

static Car car;

static int lastScore = 0;
static int highScore = 0;

static bool isPaused = false;                           /* false if the game is currently running and no collision occurs */


static void InitGame(RoadNode **roadlistPtr);           /* Called every time the game needs to restart */
static void UpdateGame(RoadNode **roadList);            /* Called every frame */
static void DrawFrame(void);                            /* Draws the current frame */
static void UpdateDrawFrame(RoadNode **roadList);       /* Unloads game and frees dynamic memory allocations, called at everytime game ends */
static void UnloadGame(RoadNode **roadList);            /* Updates the game and draws the updated/current frame */


static Road * CreateRoad(Vector2 topleft, Vector2 topRight);
static RoadNode * CreateRoadNode(Road *road);
static void DeleteRoadNode(RoadNode *roadNode);

static Road * CreateRandomRoad(Road *prevRoad);
static void CreateAddRandomizedRoadNodeToRoadList(RoadNode **roadListPtr);
static void RemoveHeadRoadNodeFromRoadList(RoadNode **roadListPtr);

static void UpdateRoadPositions(RoadNode **roadList);

static void DrawRoad(Road *newRoad, Road *prevRoad);
static void DrawAllRoads(RoadNode *rodeList);

static void DrawScore(void);
static void DrawPressButtonScreen(void);
static void DrawPauseScreen(void);

static void RestartGame(RoadNode **roadListPtr);

static void UpdateCarPosition(void);

static void CheckCarCollision(RoadNode *rodeList);

static void PrintRoadList(RoadNode *roadList);          /* For debugging purposes */

int main(void)
{
    RoadNode *roadList = NULL;

    InitWindow(screenWidth, screenHeight, "Car Game");
    SetTargetFPS(targetFPS);

    InitGame(&roadList);

    while (!WindowShouldClose())
    {
        UpdateDrawFrame(&roadList);
    }

    UnloadGame(&roadList);

    CloseWindow();

    return 0;
}

void InitGame(RoadNode **roadlistPtr) 
{
    int i;
    int currentScore;

    for (i = 0; i < ROAD_COUNT_IN_SCREEN * 2; i++) /* TO DO: MAKE DYNAMIC */
        CreateAddRandomizedRoadNodeToRoadList(roadlistPtr);

    car.pos = (Vector2) {screenWidth / 2 - CAR_WIDTH / 2, screenHeight - CAR_HEIGHT};

    carSpeedVertical = carSpeedVerticalBase;

    highScore = (lastScore > highScore) ? lastScore : highScore;

    lastScore = GetTime();
    isPaused = false;
}

void UpdateGame(RoadNode **roadlist)
{
    if (isPaused)
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            RestartGame(roadlist);
        }
    }
    else
    {
        CheckCarCollision(*roadlist);

        /* Update Positions */
        UpdateCarPosition();
        UpdateRoadPositions(roadlist);

        printf("%f\n", carSpeedVertical);
        /* Limiting the max game speed */
        if (carSpeedVertical <= 540)
            carSpeedVertical += (0.06);
    }
}

void DrawGame(RoadNode *roadlist)
{
    BeginDrawing();

    ClearBackground(BLACK);

    DrawRectangle(car.pos.x, car.pos.y, CAR_WIDTH, CAR_HEIGHT, CAR_COLOR);
    DrawAllRoads(roadlist);
    
    if (isPaused)
    {
        DrawPauseScreen();
    }
    else
    {
        DrawScore();
    }

    EndDrawing();
}

void UpdateDrawFrame(RoadNode **roadlist)
{   
    DrawGame(*roadlist);
    UpdateGame(roadlist);
}

void UnloadGame(RoadNode **roadlist)
{
    /* Free all road list */
    RoadNode *curr = *roadlist, *temp;
    while (curr)
    {
        temp = curr->next;
        DeleteRoadNode(curr);
        curr = temp;
    }

    /* Reset the roadList variable as it was pointing to a freed space */
    *roadlist = NULL;
}

Road * CreateRoad(Vector2 topleft, Vector2 topRight)
{
    Road *road = (Road *) malloc(sizeof(Road));
    if (road)
    {
        road->topLeft = topleft;
        road->topRight = topRight;
        road->height = roadHeight;
    }

    return road;
}

RoadNode * CreateRoadNode(Road *road)
{
    RoadNode * roadNode = (RoadNode *) malloc(sizeof(RoadNode));
    if (roadNode)
    {
        roadNode->road = road;
        roadNode->next = NULL;
        roadNode->prev = NULL;
    }

    return roadNode;
}

void DeleteRoadNode(RoadNode *roadNode)
{
    free(roadNode->road);
    free(roadNode);
}

Road * CreateRandomRoad(Road *prevRoad)
{
    int topLeftX, topLeftY;
    int topRightX, topRightY;

    Vector2 topLeft, topRight;

    Road *randomRoad;

    int prevHalfWidth = (prevRoad->topRight.x - prevRoad->topLeft.x) / 2;

    int randomizedRoadWidth = GetRandomValue(MIN_ROAD_WIDTH, MAX_ROAD_WIDTH);
    int shouldCreateRoadOnLeft = GetRandomValue(0,1);

    /* Clamping the road coordinates */

    if (prevRoad->topLeft.x <= MAX_ROAD_WIDTH - MIN_GAP_WIDTH) /* Not enough space to create a new road at worst case */
        shouldCreateRoadOnLeft = 0;
    else if (screenWidth - prevRoad->topRight.x <= MAX_ROAD_WIDTH - MIN_GAP_WIDTH)
        shouldCreateRoadOnLeft = 1;

    if (shouldCreateRoadOnLeft)
    {
        topRightX = GetRandomValue(prevRoad->topLeft.x + MIN_GAP_WIDTH, prevRoad->topLeft.x + prevHalfWidth);
        topLeftX = topRightX - randomizedRoadWidth;
    }
    else
    {
        topLeftX = GetRandomValue(prevRoad->topLeft.x + prevHalfWidth, prevRoad->topRight.x - MIN_GAP_WIDTH);
        topRightX = topLeftX + randomizedRoadWidth;
    }

    topLeftY = topRightY = prevRoad->topLeft.y - roadHeight;

    topLeft = (Vector2) {topLeftX, topLeftY};
    topRight = (Vector2) {topRightX, topRightY};

    randomRoad = CreateRoad(topLeft, topRight);

    return randomRoad;
}

void CreateAddRandomizedRoadNodeToRoadList(RoadNode **roadListPtr)
{
    RoadNode *roadList = *roadListPtr;
    
    int topLeftX, topLeftY;
    int topRightX, topRightY;

    Vector2 topLeft;
    Vector2 topRight;

    Road *roadToAdd;
    RoadNode *roadNodeToAdd;

    if (!roadList) /* If the list is empty */
    {
        int topLeftX = screenWidth / 2 - MIN_ROAD_WIDTH; 
        int topLeftY = screenHeight - roadHeight * 4;

        int topRightX = screenWidth / 2 + MIN_ROAD_WIDTH;
        int topRightY = screenHeight - roadHeight * 4;

        topLeft = (Vector2) {topLeftX, topLeftY};
        topRight = (Vector2) {topRightX, topRightY};

        roadToAdd = CreateRoad(topLeft, topRight);

        /* Make the first road longer so that the player can get ready */
        roadToAdd->height = roadHeight * 4;

        roadNodeToAdd = CreateRoadNode(roadToAdd);

        *roadListPtr = roadNodeToAdd;
    }
    else 
    {
        RoadNode *curr = roadList, *prev = NULL;

        while (curr)
        {
            prev = curr;
            curr = curr->next;
        }

        roadToAdd = CreateRandomRoad(prev->road);
        roadNodeToAdd = CreateRoadNode(roadToAdd);

        prev->next = roadNodeToAdd;
        
        roadNodeToAdd->prev = prev;
        roadNodeToAdd->next = NULL;
    }
}

void RemoveHeadRoadNodeFromRoadList(RoadNode **roadListPtr)
{
    RoadNode *newHeadNode = (*roadListPtr)->next;

    newHeadNode->prev = NULL;

    DeleteRoadNode(*roadListPtr);

    *roadListPtr = newHeadNode;
}

void UpdateRoadPositions(RoadNode **roadList) /* Will be called every frame */
{
    RoadNode *curr = *roadList;

    if (curr->road->topLeft.y >= screenHeight) /* The road is not on display anymore, remove it. */
    {
        RemoveHeadRoadNodeFromRoadList(roadList);
        CreateAddRandomizedRoadNodeToRoadList(roadList);
    }

    /* Reset the current node */
    curr = *roadList;

    while (curr)
    {
        curr->road->topLeft.y += carSpeedVertical * GetFrameTime();
        curr->road->topRight.y += carSpeedVertical * GetFrameTime();

        curr = curr->next;
    }
}

void DrawRoad(Road *newRoad, Road *prevRoad)
{
    Vector2 newRoadBottomLeft = (Vector2) {newRoad->topLeft.x, newRoad->topLeft.y + newRoad->height};
    Vector2 newRoadBottomRight = (Vector2) {newRoad->topRight.x, newRoad->topRight.y + newRoad->height};

    /* Using functions ending with 'V' to pass vectors directly */

    /* Drawing the vertical edges of the new road*/
    DrawLineV(newRoad->topLeft, newRoadBottomLeft, ROAD_COLOR);
    DrawLineV(newRoad->topRight, newRoadBottomRight, ROAD_COLOR);

    /* Drawing the horizontal edges connecting two consecutive roads */
    if (prevRoad)
    {
        DrawLineV(newRoadBottomLeft, prevRoad->topLeft, ROAD_COLOR);
        DrawLineV(newRoadBottomRight, prevRoad->topRight, ROAD_COLOR);
    }
}

void DrawAllRoads(RoadNode *roadList)
{
    RoadNode *curr = roadList;

    while (curr)
    {
        if (curr->prev)
            DrawRoad(curr->road, curr->prev->road);
        else
            DrawRoad(curr->road, NULL);

        curr = curr->next;
    }
}

void DrawScore()
{
    char buffer[40];
    int textWidth;

    snprintf(buffer, 40, "Score : %d", (int) GetTime() - lastScore);

    DrawText(buffer, 10, 0, 30, WHITE);

    snprintf(buffer, 40, "High Score : %d", highScore);

    DrawText(buffer, 10, 40, 20, WHITE);
}

void DrawPressButtonScreen(void)
{
    int textWidth;

    textWidth = MeasureText("Press 'Space' to Play Again.", 20);
    DrawText("Press 'Space' to Play Again.", GetScreenWidth() / 2 - textWidth / 2, GetScreenHeight() / 2 - textWidth / 2 + 150, 20, WHITE);    
}

void DrawPauseScreen(void)
{
    char buffer[40];
    int textWidth;

    snprintf(buffer, 40, "Score : %d", lastScore);
    textWidth = MeasureText(buffer, 30);

    DrawText(buffer, GetScreenWidth() / 2 - textWidth / 2, GetScreenHeight() / 2 - textWidth / 2 , 30, WHITE);
    DrawPressButtonScreen();
}

void UpdateCarPosition(void)
{
    if (IsKeyDown(KEY_A))
        car.pos.x -= carSpeedHorizontal * GetFrameTime();
    else if (IsKeyDown(KEY_D))
        car.pos.x += carSpeedHorizontal * GetFrameTime();
}

void CheckCarCollision(RoadNode *rodeList)
{
    Road *currRoad = rodeList->road;
    Road *nextRoad = rodeList->next->road;

    /* Checking collision with the vertical lines of the current/bottom road */

    if (car.pos.x <= currRoad->topLeft.x - COLLISION_TOLERANCE || car.pos.x + CAR_WIDTH >= currRoad->topRight.x + COLLISION_TOLERANCE)
    {   
        isPaused = true;
        lastScore = GetTime() - lastScore;
        return;
    }

    /* Checking collision with horizontal lines of the upper road */

    if (currRoad->topLeft.y - COLLISION_TOLERANCE >=  screenHeight - CAR_HEIGHT && 
    (car.pos.x <= nextRoad->topLeft.x - COLLISION_TOLERANCE || car.pos.x + CAR_WIDTH >= nextRoad->topRight.x + COLLISION_TOLERANCE))
    {
        isPaused = true;
        lastScore = GetTime() - lastScore;
        return;
    }
}

void PrintRoadList(RoadNode *roadList) 
{
    printf("\nRoad List: ");
    RoadNode *curr = roadList;
    while (curr) {
        printf(" -> (%f, %f)", curr->road->topLeft.x, curr->road->topLeft.y);
        curr = curr->next;
    }
    printf("\n");
}

void RestartGame(RoadNode **roadListPtr)
{
    /* Free all road list */
    UnloadGame(roadListPtr);

    InitGame(roadListPtr);
}



