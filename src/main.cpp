#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <random>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <memory>
#include <winuser.h>
#include <mmsystem.h>
using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

HINSTANCE hInst;
HWND hwnd;
const wchar_t CLASS_NAME[] = L"Desktop Sonic";
int petX = 100;
int petY = 100;
int targetX = 100;
int targetY = 100;
bool isMoving = false;
int animationFrame = 0;
const int SPRITE_SCALE = 4; 
const int ORIGINAL_SPRITE_SIZE = 32;
const int WINDOW_SIZE = ORIGINAL_SPRITE_SIZE * SPRITE_SCALE; 
float velocityY = 0;
float velocityX = 0;
const float GRAVITY = 0.5f;
const float MOVE_SPEED = 3.0f;
const float FRICTION = 0.8f;
bool isOnGround = false;
RECT taskbarRect;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> distX;
std::uniform_int_distribution<int> distY;

const float ACCELERATION_SPEED = 0.046875f;
const float DECELERATION_SPEED = 0.5f;
const float FRICTION_SPEED = 0.046875f;
const float TOP_SPEED = 6.0f;
const float JUMP_FORCE = 6.5f;
const float GRAVITY_FORCE = 0.21875f;
const float AIR_ACCELERATION = 0.09375f;
const float MAX_Y_SPEED = 16.0f;

float groundSpeed = 0.0f;
float xSpeed = 0.0f;
float ySpeed = 0.0f;
bool facingLeft = false;
bool isJumping = false;

GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
Image* idleSprite = nullptr;
std::vector<Image*> walkSprites;
std::vector<Image*> runSprites;
std::vector<Image*> spinSprites;
int currentFrame = 0;
const int FRAME_DELAY = 4;
int frameCounter = 0;

const float ROLL_FRICTION_SPEED = 0.0234375f;
const float ROLL_DECELERATION_SPEED = 0.125f;
const float ROLL_MIN_SPEED = 0.5f;
const float ROLL_MAX_SPEED = 16.0f;

bool isRolling = false;

Image* lookUpSprite = nullptr;  
Image* crouchSprite = nullptr; 
bool isLookingUp = false;
bool isCrouching = false;

LARGE_INTEGER frequency;
LARGE_INTEGER lastTime;
const double TARGET_FPS = 60.0;
const double TIME_STEP = 1.0 / TARGET_FPS;
double accumulator = 0.0;

std::vector<Image*> balanceSprites;
bool isBalancing = false;
const float BALANCE_THRESHOLD = 10.0f;
const int BALANCE_FRAME_DELAY = 12;

std::vector<Image*> boredSprites;
bool isBored = false;
DWORD lastInputTime = GetTickCount();
const DWORD BORED_TIMEOUT = 15000; 
const int BORED_FRAME_DELAY = 8;  

bool isDragging = false;
POINT lastMousePos = {0, 0};
POINT lastValidMouseDelta = {0, 0};
const float THROW_FORCE_MULTIPLIER = 0.2f;  // Change to 0.5f and then throw him against the wall for funny.

std::vector<Image*> hurtSprites;
bool isHurt = false;

HWND jumpSound = NULL;
HWND rollSound = NULL;

void UpdateTaskbarRect() {
    HWND taskbar = FindWindow(L"Shell_TrayWnd", NULL);
    if (taskbar) {
        GetWindowRect(taskbar, &taskbarRect);
    }
}

bool LoadSprites() {
    idleSprite = Image::FromFile(L"assets/images/idle.png");
    if (!idleSprite || idleSprite->GetLastStatus() != Ok) {
        MessageBox(NULL, L"Failed to load idle.png", L"Error", MB_OK);
        return false;
    }

    for (int i = 1; i < 6; i++) {
        wchar_t path[256];
        swprintf_s(path, L"assets/images/walk%d.png", i);
        Image* frame = Image::FromFile(path);
        if (frame && frame->GetLastStatus() == Ok) {
            walkSprites.push_back(frame);
        }
    }

    for (int i = 1; i < 4; i++) {
        wchar_t path[256];
        swprintf_s(path, L"assets/images/run%d.png", i);
        Image* frame = Image::FromFile(path);
        if (frame && frame->GetLastStatus() == Ok) {
            runSprites.push_back(frame);
        }
    }

    for (int i = 1; i < 8; i++) { 
        wchar_t path[256];
        swprintf_s(path, L"assets/images/roll%d.png", i);
        Image* frame = Image::FromFile(path);
        if (frame && frame->GetLastStatus() == Ok) {
            spinSprites.push_back(frame);
        }
    }
    
    lookUpSprite = Image::FromFile(L"assets/images/up.png");
    if (!lookUpSprite || lookUpSprite->GetLastStatus() != Ok) {
        MessageBox(NULL, L"Failed to load up.png", L"Error", MB_OK);
        return false;
    }

    crouchSprite = Image::FromFile(L"assets/images/down.png");
    if (!crouchSprite || crouchSprite->GetLastStatus() != Ok) {
        MessageBox(NULL, L"Failed to load down.png", L"Error", MB_OK);
        return false;
    }

    for (int i = 1; i <= 2; i++) {
        wchar_t path[256];
        swprintf_s(path, L"assets/images/balancing%d.png", i);
        Image* frame = Image::FromFile(path);
        if (frame && frame->GetLastStatus() == Ok) {
            balanceSprites.push_back(frame);
        }
    }

    for (int i = 1; i <= 2; i++) {
        wchar_t path[256];
        swprintf_s(path, L"assets/images/boredloop%d.png", i);
        Image* frame = Image::FromFile(path);
        if (frame && frame->GetLastStatus() == Ok) {
            boredSprites.push_back(frame);
        }
    }

    for (int i = 1; i <= 2; i++) {
        wchar_t path[256];
        swprintf_s(path, L"assets/images/hurt%d.png", i);
        Image* frame = Image::FromFile(path);
        if (frame && frame->GetLastStatus() == Ok) {
            hurtSprites.push_back(frame);
        }
    }

    return true;
}

bool LoadSounds() {
    if (!PlaySound(NULL, NULL, 0)) {
        MessageBox(NULL, L"Failed to initialize audio", L"Error", MB_OK);
        return false;
    }
    return true;
}

void PlaySoundEffect(const wchar_t* soundFile) {
    PlaySound(soundFile, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void CleanupSprites() {
    if (idleSprite) {
        delete idleSprite;
        idleSprite = nullptr;
    }
    
    for (auto sprite : walkSprites) {
        delete sprite;
    }
    walkSprites.clear();
    
    for (auto sprite : runSprites) {
        delete sprite;
    }
    runSprites.clear();
    
    for (auto sprite : spinSprites) {
        delete sprite;
    }
    spinSprites.clear();
    
    if (lookUpSprite) {
        delete lookUpSprite;
        lookUpSprite = nullptr;
    }
    
    if (crouchSprite) {
        delete crouchSprite;
        crouchSprite = nullptr;
    }

    for (auto sprite : balanceSprites) {
        delete sprite;
    }
    balanceSprites.clear();

    for (auto sprite : boredSprites) {
        delete sprite;
    }
    boredSprites.clear();

    for (auto sprite : hurtSprites) {
        delete sprite;
    }
    hurtSprites.clear();
}

void PickNewTarget() {
    targetX = distX(gen);
    targetY = distY(gen);
    isMoving = true;
}

bool IsValidWindow(HWND window) {
    if (!IsWindowVisible(window) || window == hwnd) return false;
    
    DWORD style = GetWindowLong(window, GWL_STYLE);
    DWORD exStyle = GetWindowLong(window, GWL_EXSTYLE);
    
    if ((style & WS_MINIMIZE) || (exStyle & WS_EX_TOOLWINDOW)) return false;
    
    return true;
}

bool CheckCollision(int x, int y, bool* nearEdge = nullptr) {
    if (nearEdge) *nearEdge = false;
    const int CHECK_POINTS = 3;
    for (int i = 0; i < CHECK_POINTS; i++) {
        POINT checkPoint = { 
            x + (WINDOW_SIZE * i) / (CHECK_POINTS - 1),
            y + WINDOW_SIZE 
        };
        
        HWND windowAtPoint = WindowFromPoint(checkPoint);
        
        if (windowAtPoint && IsValidWindow(windowAtPoint)) {
            RECT windowRect;
            GetWindowRect(windowAtPoint, &windowRect);
            
            if (y + WINDOW_SIZE <= windowRect.top + 10 && 
                y + WINDOW_SIZE >= windowRect.top - 5 &&  
                x + WINDOW_SIZE > windowRect.left &&
                x < windowRect.right &&
                ySpeed >= 0) {
                
                if (nearEdge && isOnGround && abs(groundSpeed) < 0.5f) {
                    float distanceFromLeft = x - windowRect.left;
                    float distanceFromRight = windowRect.right - (x + WINDOW_SIZE);
                    
                    *nearEdge = (distanceFromLeft <= BALANCE_THRESHOLD * 2 || 
                               distanceFromRight <= BALANCE_THRESHOLD * 2);
                }
                return true;
            }
        }
    }
    
    if (y + WINDOW_SIZE >= GetSystemMetrics(SM_CYSCREEN)) {
        return true;
    }
    
    return false;
}

void UpdatePetPhysics() {
    if (isDragging) {
        return;
    }

    DWORD currentTime = GetTickCount();
    if (currentTime - lastInputTime >= BORED_TIMEOUT && 
        isOnGround && 
        abs(groundSpeed) < 0.1f && 
        !isLookingUp && 
        !isCrouching && 
        !isDragging &&
        !isBalancing) {
        isBored = true;
    } else {
        isBored = false;
    }

    if ((GetAsyncKeyState(VK_LEFT) & 0x8000) ||
        (GetAsyncKeyState(VK_RIGHT) & 0x8000) ||
        (GetAsyncKeyState(VK_UP) & 0x8000) ||
        (GetAsyncKeyState(VK_DOWN) & 0x8000) ||
        (GetAsyncKeyState('Z') & 0x8000)) {
        lastInputTime = GetTickCount();
        isBored = false;
    }

    bool nearEdge = false;
    if (!CheckCollision(petX, petY + 1, &nearEdge)) { 
        isOnGround = false;
    }
    isBalancing = nearEdge;
    
    if (isBalancing != nearEdge) {
        OutputDebugString(nearEdge ? L"Near Edge\n" : L"Not Near Edge\n");
    }

    if (isOnGround) {
        bool pressingLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000);
        bool pressingRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000);
        bool pressingDown = (GetAsyncKeyState(VK_DOWN) & 0x8000);
        bool pressingUp = (GetAsyncKeyState(VK_UP) & 0x8000);

        isLookingUp = false;
        isCrouching = false;

        if (abs(groundSpeed) < 0.1f) {
            if (pressingUp) {
                isLookingUp = true;
            } else if (pressingDown) {
                isCrouching = true;
            }
        }

        if (!isLookingUp && !isCrouching) {
            if (pressingDown && abs(groundSpeed) >= ROLL_MIN_SPEED) {
                if (!isRolling) {
                    PlaySoundEffect(L"assets/sounds/S1_BE.wav");
                }
                isRolling = true;
            } else if (abs(groundSpeed) < ROLL_MIN_SPEED) {
                isRolling = false;
            }

            if (isRolling) {
                if (pressingLeft && groundSpeed > 0) {
                    groundSpeed -= ROLL_DECELERATION_SPEED;
                    groundSpeed -= ROLL_FRICTION_SPEED;
                    if (groundSpeed <= 0) groundSpeed = -ROLL_MIN_SPEED;
                } else if (pressingRight && groundSpeed < 0) {
                    groundSpeed += ROLL_DECELERATION_SPEED;
                    groundSpeed += ROLL_FRICTION_SPEED;
                    if (groundSpeed >= 0) groundSpeed = ROLL_MIN_SPEED;
                } else {
                    float friction = min(abs(groundSpeed), ROLL_FRICTION_SPEED) * (groundSpeed > 0 ? -1 : 1);
                    groundSpeed += friction;
                }
            } else {
                bool pressingLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000);
                bool pressingRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000);

                if (pressingLeft) {
                    facingLeft = true;
                    if (groundSpeed > 0) {
                        groundSpeed -= DECELERATION_SPEED;
                        if (groundSpeed <= 0) groundSpeed = -0.5f;
                    }
                    else if (groundSpeed > -TOP_SPEED) {
                        groundSpeed -= ACCELERATION_SPEED;
                        if (groundSpeed <= -TOP_SPEED) groundSpeed = -TOP_SPEED;
                    }
                }
                else if (pressingRight) {
                    facingLeft = false;
                    if (groundSpeed < 0) {
                        groundSpeed += DECELERATION_SPEED;
                        if (groundSpeed >= 0) groundSpeed = 0.5f;
                    }
                    else if (groundSpeed < TOP_SPEED) {
                        groundSpeed += ACCELERATION_SPEED;
                        if (groundSpeed >= TOP_SPEED) groundSpeed = TOP_SPEED;
                    }
                }
                else {
                    float friction = min(abs(groundSpeed), FRICTION_SPEED) * (groundSpeed > 0 ? -1 : 1);
                    groundSpeed += friction;
                }
            }
        }

        if (GetAsyncKeyState('Z') & 0x8000 && !isJumping) {
            ySpeed = -JUMP_FORCE;
            isJumping = true;
            isOnGround = false;
            PlaySoundEffect(L"assets/sounds/S1_A0.wav");
        }

        xSpeed = groundSpeed;

        if (abs(groundSpeed) > ROLL_MAX_SPEED) {
            groundSpeed = ROLL_MAX_SPEED * (groundSpeed > 0 ? 1 : -1);
        }

        xSpeed = groundSpeed;
    }
    else {
        bool pressingLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000);
        bool pressingRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000);

        if (pressingLeft && xSpeed > -TOP_SPEED) {
            xSpeed -= AIR_ACCELERATION;
            facingLeft = true;
        }
        else if (pressingRight && xSpeed < TOP_SPEED) {
            xSpeed += AIR_ACCELERATION;
            facingLeft = false;
        }

        if (ySpeed < 0 && ySpeed > -4) {
            xSpeed -= (floor(xSpeed / 0.125f) / 256.0f);
        }

        ySpeed += GRAVITY_FORCE;
        if (ySpeed > MAX_Y_SPEED) ySpeed = MAX_Y_SPEED;
    }

    petX += static_cast<int>(xSpeed);
    petY += static_cast<int>(ySpeed);

    if (CheckCollision(petX, petY)) {
        petY = min(GetSystemMetrics(SM_CYSCREEN) - WINDOW_SIZE,
                   min(taskbarRect.top - WINDOW_SIZE, petY));
        ySpeed = 0;
        isOnGround = true;
        isJumping = false;
        isHurt = false;
        groundSpeed = xSpeed;
    }

    if (petX < 0) {
        petX = 0;
        xSpeed = 0;
        groundSpeed = 0;
    }
    if (petX + WINDOW_SIZE > GetSystemMetrics(SM_CXSCREEN)) {
        petX = GetSystemMetrics(SM_CXSCREEN) - WINDOW_SIZE;
        xSpeed = 0;
        groundSpeed = 0;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        ShowWindow(hwnd, SW_SHOW);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        HDC hdcScreen = GetDC(NULL);
        HDC memDC = CreateCompatibleDC(hdcScreen);
        
        BITMAPINFOHEADER bmi = { 0 };
        bmi.biSize = sizeof(BITMAPINFOHEADER);
        bmi.biWidth = WINDOW_SIZE;
        bmi.biHeight = -WINDOW_SIZE;
        bmi.biPlanes = 1;
        bmi.biBitCount = 32;
        bmi.biCompression = BI_RGB;

        void* pvBits;
        HBITMAP hBitmap = CreateDIBSection(memDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

        memset(pvBits, 0, WINDOW_SIZE * WINDOW_SIZE * 4);

        Graphics graphics(memDC);
        graphics.SetSmoothingMode(SmoothingModeNone);
        graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
        graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

        Image* currentSprite = nullptr;
        if (!isOnGround && !isRolling) {
            if (isHurt) {
                currentSprite = hurtSprites[(currentFrame / FRAME_DELAY) % hurtSprites.size()];
            } else {
                currentSprite = spinSprites[currentFrame % spinSprites.size()];
            }
        }
        else if (isRolling) {
            currentSprite = spinSprites[currentFrame % spinSprites.size()];
        }
        else if (isLookingUp && lookUpSprite) {
            currentSprite = lookUpSprite;
        }
        else if (isCrouching && crouchSprite) {
            currentSprite = crouchSprite;
        }
        else if (abs(groundSpeed) > TOP_SPEED * 0.7f && !runSprites.empty()) {
            currentSprite = runSprites[currentFrame % runSprites.size()];
        }
        else if (abs(groundSpeed) > 0.1f && !walkSprites.empty()) {
            currentSprite = walkSprites[currentFrame % walkSprites.size()];
        }
        else if (isBalancing && !balanceSprites.empty()) {
            currentSprite = balanceSprites[(currentFrame / BALANCE_FRAME_DELAY) % balanceSprites.size()];
        }
        else if (isBored && !boredSprites.empty()) {
            currentSprite = boredSprites[(currentFrame / BORED_FRAME_DELAY) % boredSprites.size()];
        }
        else if (idleSprite) {
            currentSprite = idleSprite;
        }

        if (currentSprite) {
            ImageAttributes imgAttr;
            imgAttr.SetColorKey(Color(255, 255, 0, 255), Color(255, 255, 0, 255));

            Matrix matrix;
            if (facingLeft) {
                matrix.Scale(-1.0f, 1.0f);
                matrix.Translate(static_cast<float>(WINDOW_SIZE), 0.0f, MatrixOrderAppend);
            }
            graphics.SetTransform(&matrix);

            Rect destRect(0, 0, WINDOW_SIZE, WINDOW_SIZE);
            graphics.DrawImage(currentSprite, 
                destRect,
                0, 0,
                currentSprite->GetWidth(), currentSprite->GetHeight(),
                UnitPixel,
                &imgAttr);
        }

        POINT ptSrc = {0, 0};
        POINT ptDst = {petX, petY};
        SIZE sizeWnd = {WINDOW_SIZE, WINDOW_SIZE};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        
        UpdateLayeredWindow(hwnd, NULL, &ptDst, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

        SelectObject(memDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, hdcScreen);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        OutputDebugString(L"Mouse Down\n");
        isDragging = true;
        GetCursorPos(&lastMousePos);
        SetCapture(hwnd);
        return 0;
    }

    case WM_LBUTTONUP:
    {
        OutputDebugString(L"Mouse Up\n");
        isDragging = false;
        isHurt = true;
        isOnGround = false;
        
        xSpeed = lastValidMouseDelta.x * THROW_FORCE_MULTIPLIER;
        ySpeed = lastValidMouseDelta.y * THROW_FORCE_MULTIPLIER;
        
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (isDragging) {
            OutputDebugString(L"Dragging\n");
            POINT currentPos;
            GetCursorPos(&currentPos);
            
            int deltaX = currentPos.x - lastMousePos.x;
            int deltaY = currentPos.y - lastMousePos.y;
            
            petX += deltaX;
            petY += deltaY;
            
            lastValidMouseDelta.x = deltaX;
            lastValidMouseDelta.y = deltaY;
            
            lastMousePos = currentPos;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    if (!LoadSprites()) {
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    if (!LoadSounds()) {
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    hInst = hInstance;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Desktop Sonic",
        WS_POPUP,
        petX, petY, WINDOW_SIZE, WINDOW_SIZE,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Failed to create window", L"Error", MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    UpdateTaskbarRect();

    MSG msg = {};
    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto cleanup;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        double deltaTime = (currentTime.QuadPart - lastTime.QuadPart) / (double)frequency.QuadPart;
        lastTime = currentTime;

        if (deltaTime > 0.25) {
            deltaTime = 0.25;
        }

        accumulator += deltaTime;

        int updateCount = 0;
        while (accumulator >= TIME_STEP && updateCount < 4) {
            UpdatePetPhysics();
            accumulator -= TIME_STEP;
            
            frameCounter++;
            if (frameCounter >= FRAME_DELAY) {
                frameCounter = 0;
                currentFrame++;
            }
            updateCount++;
        }

        SetWindowPos(hwnd, HWND_TOPMOST, petX, petY, 0, 0, SWP_NOSIZE);
        InvalidateRect(hwnd, NULL, TRUE);

        Sleep(16);
    }

cleanup:
    CleanupSprites();
    GdiplusShutdown(gdiplusToken);
    return 0;
}

// super secret go pico comment!!