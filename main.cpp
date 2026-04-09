#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>

// 수학 상수
const float PI = 3.14159265359f;

// 벡터 구조체
struct Vector2 {
    float x, y;

    Vector2(float x = 0, float y = 0) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }

    float length() const { return std::sqrt(x * x + y * y); }
    Vector2 normalized() const {
        float len = length();
        return len > 0 ? Vector2(x / len, y / len) : Vector2(0, 0);
    }
};

// 벡터 길이 제한 함수
Vector2 clampVector(const Vector2& vec, float maxLength) {
    float len = vec.length();
    return len > maxLength ? vec.normalized() * maxLength : vec;
}

// AABB 구조체
struct AABB {
    Vector2 position;
    Vector2 size;

    AABB(Vector2 pos = Vector2(), Vector2 sz = Vector2()) : position(pos), size(sz) {}

    bool intersects(const AABB& other) const {
        return position.x < other.position.x + other.size.x &&
               position.x + size.x > other.position.x &&
               position.y < other.position.y + other.size.y &&
               position.y + size.y > other.position.y;
    }

    Vector2 getCenter() const { return position + size * 0.5f; }
};

// 플레이어 방향 열거형
enum Direction { UP, DOWN, LEFT, RIGHT };

// 플레이어 상태 열거형
enum PlayerState { IDLE, MOVE };

// 플레이어 구조체
struct Player {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 size;
    float friction;
    float maxSpeed;
    Direction direction;
    PlayerState state;

    // 애니메이션 관련
    SDL_Texture* texture;
    int frameWidth;
    int frameHeight;
    int currentFrame;
    float animationTimer;
    float animationSpeed; // 초당 프레임

    Player() : position(400, 300), velocity(0, 0), acceleration(0, 0),
               size(32, 32), friction(0.8f), maxSpeed(200.0f),
               direction(DOWN), state(IDLE),
               texture(nullptr), frameWidth(32), frameHeight(32),
               currentFrame(0), animationTimer(0.0f), animationSpeed(8.0f) {}

    void updateAnimation(float dt) {
        if (state == MOVE) {
            animationTimer += dt;
            if (animationTimer >= 1.0f / animationSpeed) {
                currentFrame = (currentFrame + 1) % 4; // 4프레임 애니메이션
                animationTimer = 0.0f;
            }
        } else {
            currentFrame = 0; // 멈춰있을 때는 첫 프레임
        }
    }

    SDL_Rect getSourceRect() const {
        int framesPerRow = 4; // 가로로 4프레임
        int row = static_cast<int>(direction); // 방향에 따라 행 선택
        int col = currentFrame;

        return SDL_Rect{
            col * frameWidth,
            row * frameHeight,
            frameWidth,
            frameHeight
        };
    }
};

// 바위 구조체 (강체)
struct Rock {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 size;
    float mass;
    float friction;

    Rock(Vector2 pos, Vector2 sz, float m = 10.0f, float f = 0.9f)
        : position(pos), velocity(0, 0), acceleration(0, 0), size(sz), mass(m), friction(f) {}

    void updatePhysics(float dt) {
        // 오일러 적분: v = v + a * dt
        velocity = velocity + acceleration * dt;

        // 마찰력 적용
        velocity = velocity * friction;

        // 최대 속도 제한 (바위는 느리게)
        velocity = clampVector(velocity, 50.0f);

        // 위치 업데이트
        position = position + velocity * dt;

        // 가속도 리셋
        acceleration = Vector2(0, 0);
    }

    void applyForce(Vector2 force) {
        acceleration = acceleration + force / mass;
    }
};

// 타일 구조체
struct Tile {
    Vector2 position;
    Vector2 size;
    bool isGround; // 땅인지 여부

    Tile(Vector2 pos, Vector2 sz, bool ground = true) : position(pos), size(sz), isGround(ground) {}
};

// 부품 구조체
struct Part {
    Vector2 position;
    Vector2 size;
    bool collected;

    Part(Vector2 pos, Vector2 sz) : position(pos), size(sz), collected(false) {}
};

// 맵 구조체
struct Map {
    int width, height; // 타일 단위
    int tileSize;
    std::vector<Tile> tiles;
    std::vector<Rock> walls; // 맵 경계 벽

    Map(int w, int h, int ts) : width(w), height(h), tileSize(ts) {
        generateMap();
    }

    void generateMap() {
        // 타일 생성 (격자무늬 땅)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Vector2 pos(x * tileSize, y * tileSize);
                Vector2 size(tileSize, tileSize);
                tiles.push_back(Tile(pos, size, true));
            }
        }

        // 맵 경계 벽 생성
        int mapWidth = width * tileSize;
        int mapHeight = height * tileSize;

        // 위쪽 벽
        walls.push_back(Rock(Vector2(0, -tileSize), Vector2(mapWidth, tileSize)));
        // 아래쪽 벽
        walls.push_back(Rock(Vector2(0, mapHeight), Vector2(mapWidth, tileSize)));
        // 왼쪽 벽
        walls.push_back(Rock(Vector2(-tileSize, 0), Vector2(tileSize, mapHeight)));
        // 오른쪽 벽
        walls.push_back(Rock(Vector2(mapWidth, 0), Vector2(tileSize, mapHeight)));
    }

    Vector2 getSize() const { return Vector2(width * tileSize, height * tileSize); }
};

// 카메라 구조체
struct Camera {
    Vector2 position;
    Vector2 target;
    float lerpSpeed;

    Camera() : position(0, 0), target(0, 0), lerpSpeed(0.1f) {}

    void followSmooth(const Vector2& targetPos, float dt) {
        target = targetPos;
        Vector2 diff = target - position;
        position = position + diff * lerpSpeed;
    }
};

// 전역 변수
Player player;
std::vector<Rock> rocks;
std::vector<Part> parts;
Camera camera;
Map map(25, 19, 32); // 25x19 타일, 각 32x32 픽셀
Uint64 lastTime = 0;

// 유틸리티 함수들
float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

Vector2 lerp(const Vector2& a, const Vector2& b, float t) {
    return Vector2(lerp(a.x, b.x, t), lerp(a.y, b.y, t));
}

// 물리 업데이트 함수
void updatePhysics(float dt) {
    // 입력 처리 (WASD) 및 방향/상태 업데이트
    Vector2 inputDir(0, 0);
    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]) { inputDir.y -= 1; player.direction = UP; }
    if (keys[SDL_SCANCODE_S]) { inputDir.y += 1; player.direction = DOWN; }
    if (keys[SDL_SCANCODE_A]) { inputDir.x -= 1; player.direction = LEFT; }
    if (keys[SDL_SCANCODE_D]) { inputDir.x += 1; player.direction = RIGHT; }

    // 상태 업데이트
    player.state = (inputDir.length() > 0) ? MOVE : IDLE;

    // 대각선 이동 정규화
    if (inputDir.length() > 0) {
        inputDir = inputDir.normalized();
    }

    // 가속도 설정 (오일러 적분용)
    player.acceleration = inputDir * 500.0f; // 가속도 값

    // 오일러 적분: v = v + a * dt
    player.velocity = player.velocity + player.acceleration * dt;

    // 마찰력 적용
    player.velocity = player.velocity * player.friction;

    // 최대 속도 제한
    player.velocity = clampVector(player.velocity, player.maxSpeed);

    // 위치 업데이트: x = x + v * dt
    Vector2 newPosition = player.position + player.velocity * dt;

    // 바위 물리 업데이트
    for (auto& rock : rocks) {
        rock.updatePhysics(dt);
    }

    // 충돌 감지 및 해결
    AABB playerAABB(player.position, player.size);

    // 바위와의 충돌 (강체 동역학)
    for (auto& rock : rocks) {
        AABB rockAABB(rock.position, rock.size);

        // 플레이어의 새 위치로 AABB 생성
        AABB newPlayerAABB(newPosition, player.size);

        if (newPlayerAABB.intersects(rockAABB)) {
            // 플레이어가 바위를 미는 방향 계산
            Vector2 pushDir = (newPosition - player.position).normalized();

            // 바위에 힘 적용 (플레이어 속도에 비례)
            Vector2 pushForce = pushDir * player.velocity.length() * 100.0f;
            rock.applyForce(pushForce);

            // 충돌 해결: 겹침 방향으로 밀어내기
            Vector2 playerCenter = newPlayerAABB.getCenter();
            Vector2 rockCenter = rockAABB.getCenter();

            Vector2 diff = playerCenter - rockCenter;
            float overlapX = (newPlayerAABB.size.x + rockAABB.size.x) * 0.5f - std::abs(diff.x);
            float overlapY = (newPlayerAABB.size.y + rockAABB.size.y) * 0.5f - std::abs(diff.y);

            if (overlapX < overlapY) {
                // X축으로 밀어내기
                newPosition.x += diff.x > 0 ? overlapX : -overlapX;
                player.velocity.x = 0; // 속도 리셋
            } else {
                // Y축으로 밀어내기
                newPosition.y += diff.y > 0 ? overlapY : -overlapY;
                player.velocity.y = 0; // 속도 리셋
            }
        }
    }

    // 맵 경계 벽과의 충돌
    for (auto& wall : map.walls) {
        AABB wallAABB(wall.position, wall.size);
        AABB newPlayerAABB(newPosition, player.size);

        if (newPlayerAABB.intersects(wallAABB)) {
            Vector2 playerCenter = newPlayerAABB.getCenter();
            Vector2 wallCenter = wallAABB.getCenter();

            Vector2 diff = playerCenter - wallCenter;
            float overlapX = (newPlayerAABB.size.x + wallAABB.size.x) * 0.5f - std::abs(diff.x);
            float overlapY = (newPlayerAABB.size.y + wallAABB.size.y) * 0.5f - std::abs(diff.y);

            if (overlapX < overlapY) {
                newPosition.x += diff.x > 0 ? overlapX : -overlapX;
                player.velocity.x = 0;
            } else {
                newPosition.y += diff.y > 0 ? overlapY : -overlapY;
                player.velocity.y = 0;
            }
        }
    }

    // 바위와 맵 경계 충돌
    for (auto& rock : rocks) {
        Vector2 rockNewPos = rock.position + rock.velocity * dt;

        for (auto& wall : map.walls) {
            AABB wallAABB(wall.position, wall.size);
            AABB rockAABB(rockNewPos, rock.size);

            if (rockAABB.intersects(wallAABB)) {
                // 바위가 벽에 부딪히면 속도 반전
                Vector2 wallCenter = wallAABB.getCenter();
                Vector2 rockCenter = rockAABB.getCenter();

                Vector2 diff = rockCenter - wallCenter;
                if (std::abs(diff.x) > std::abs(diff.y)) {
                    rock.velocity.x = -rock.velocity.x * 0.5f; // 반전 및 감쇠
                } else {
                    rock.velocity.y = -rock.velocity.y * 0.5f;
                }
                break; // 한 벽에만 충돌 처리
            }
        }
    }

    // 부품 수집
    for (auto& part : parts) {
        if (!part.collected) {
            AABB partAABB(part.position, part.size);
            AABB newPlayerAABB(newPosition, player.size);

            if (newPlayerAABB.intersects(partAABB)) {
                part.collected = true;
                std::cout << "부품 수집됨! 위치: (" << part.position.x << ", " << part.position.y << ")" << std::endl;
            }
        }
    }

    player.position = newPosition;

    // 카메라 업데이트 (감성적 추적 + 클램프)
    camera.followSmooth(player.position - Vector2(400, 300), dt); // 화면 중앙에 플레이어

    // 카메라 클램프 (맵 경계 내로 제한)
    Vector2 mapSize = map.getSize();
    camera.position.x = std::max(0.0f, std::min(camera.position.x, mapSize.x - 800.0f));
    camera.position.y = std::max(0.0f, std::min(camera.position.y, mapSize.y - 600.0f));
}

// 렌더링 함수 (Y-Sorting 적용)
void render(SDL_Renderer* renderer) {
    // 배경 지우기 (어두운 회색)
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // Y-Sorting을 위한 렌더링 객체 리스트
    struct RenderObject {
        float yPos; // Y 좌표 (정렬용)
        std::function<void()> drawFunc; // 그리기 함수
    };

    std::vector<RenderObject> renderList;

    // 타일 추가 (Y-Sorting에 포함하지 않음 - 배경이므로 먼저 그림)
    for (const auto& tile : map.tiles) {
        if (tile.isGround) {
            int gridX = static_cast<int>(tile.position.x / map.tileSize);
            int gridY = static_cast<int>(tile.position.y / map.tileSize);
            bool isLight = (gridX + gridY) % 2 == 0;

            SDL_SetRenderDrawColor(renderer,
                isLight ? 80 : 60,
                isLight ? 80 : 60,
                isLight ? 80 : 60, 255);

            SDL_Rect tileRect = {
                static_cast<int>(tile.position.x - camera.position.x),
                static_cast<int>(tile.position.y - camera.position.y),
                static_cast<int>(tile.size.x),
                static_cast<int>(tile.size.y)
            };
            SDL_RenderFillRect(renderer, &tileRect);
        }
    }

    // 바위 추가 (Y-Sorting)
    for (const auto& rock : rocks) {
        float yBottom = rock.position.y + rock.size.y;
        renderList.push_back({
            yBottom,
            [&]() {
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                SDL_Rect rockRect = {
                    static_cast<int>(rock.position.x - camera.position.x),
                    static_cast<int>(rock.position.y - camera.position.y),
                    static_cast<int>(rock.size.x),
                    static_cast<int>(rock.size.y)
                };
                SDL_RenderFillRect(renderer, &rockRect);
            }
        });
    }

    // 부품 추가 (Y-Sorting, 수집되지 않은 것만)
    for (const auto& part : parts) {
        if (!part.collected) {
            float yBottom = part.position.y + part.size.y;
            renderList.push_back({
                yBottom,
                [&]() {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_Rect partRect = {
                        static_cast<int>(part.position.x - camera.position.x),
                        static_cast<int>(part.position.y - camera.position.y),
                        static_cast<int>(part.size.x),
                        static_cast<int>(part.size.y)
                    };
                    SDL_RenderFillRect(renderer, &partRect);
                }
            });
        }
    }

    // 플레이어 추가 (Y-Sorting)
    float playerYBottom = player.position.y + player.size.y;
    renderList.push_back({
        playerYBottom,
        [&]() {
            SDL_Rect destRect = {
                static_cast<int>(player.position.x - camera.position.x),
                static_cast<int>(player.position.y - camera.position.y),
                static_cast<int>(player.size.x),
                static_cast<int>(player.size.y)
            };
            SDL_RenderCopy(renderer, player.texture, &player.getSourceRect(), &destRect);
        }
    });

    // Y 좌표로 정렬 (낮은 Y부터 높은 Y 순서로)
    std::sort(renderList.begin(), renderList.end(),
        [](const RenderObject& a, const RenderObject& b) {
            return a.yPos < b.yPos;
        });

    // 정렬된 순서대로 렌더링
    for (const auto& obj : renderList) {
        obj.drawFunc();
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    // SDL 초기화
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL 초기화 실패: " << SDL_GetError() << std::endl;
        return 1;
    }

    // SDL_image 초기화
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image 초기화 실패: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 창 생성
    SDL_Window* window = SDL_CreateWindow(
        "StellaTrail",              // 창 제목
        SDL_WINDOWPOS_CENTERED,     // X 위치
        SDL_WINDOWPOS_CENTERED,     // Y 위치
        800,                        // 너비
        600,                        // 높이
        SDL_WINDOW_SHOWN            // 플래그
    );

    if (!window) {
        std::cerr << "창 생성 실패: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // 렌더러 생성
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "렌더러 생성 실패: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // 플레이어 텍스처 로드
    char* basePath = SDL_GetBasePath();
    std::string texturePath;
    if (basePath) {
        texturePath = std::string(basePath) + "../player.png";
        SDL_free(basePath);
    } else {
        texturePath = "player.png"; // fallback
    }

    player.texture = IMG_LoadTexture(renderer, texturePath.c_str());
    if (!player.texture) {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
        std::cerr << "IMG_Error: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // 게임 오브젝트 초기화
    rocks.push_back(Rock(Vector2(500, 300), Vector2(64, 64), 15.0f, 0.85f)); // 바위 (질량 15, 마찰력 0.85)
    rocks.push_back(Rock(Vector2(200, 400), Vector2(64, 64), 20.0f, 0.9f));  // 더 무거운 바위

    // 부품 초기화 (맵 곳곳에 배치)
    parts.push_back(Part(Vector2(150, 150), Vector2(24, 24)));
    parts.push_back(Part(Vector2(400, 200), Vector2(24, 24)));
    parts.push_back(Part(Vector2(650, 350), Vector2(24, 24)));
    parts.push_back(Part(Vector2(300, 500), Vector2(24, 24)));
    parts.push_back(Part(Vector2(700, 100), Vector2(24, 24)));

    // 시간 초기화
    lastTime = SDL_GetPerformanceCounter();

    // 게임 루프
    bool running = true;
    SDL_Event event;

    while (running) {
        // Delta time 계산 (프레임 독립성)
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(currentTime - lastTime) / static_cast<float>(SDL_GetPerformanceFrequency());
        lastTime = currentTime;

        // dt 상한선 설정 (너무 큰 값 방지)
        if (dt > 0.016f) dt = 0.016f; // 60FPS 기준

        // 이벤트 처리
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
            }
        }

        // 물리 업데이트
        updatePhysics(dt);

        // 플레이어 애니메이션 업데이트
        player.updateAnimation(dt);

        // 렌더링
        render(renderer);
    }

    // 정리
    SDL_DestroyTexture(player.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
