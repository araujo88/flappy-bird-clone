#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <list>
#include <iostream>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 600

struct Pipe
{
    int x;                             // x-coordinate of the pipe
    int gapY;                          // y-coordinate of the gap's center
    static const int GAP_HEIGHT = 250; // The height of the gap; you can adjust this value
    static const int PIPE_WIDTH = 50;  // Width of the pipe; adjust based on your pipe sprite
    bool passed = false;
};

struct Bird
{
    int x, y;          // Bird's position
    int width, height; // Bird's dimensions
    float velocity;    // Bird's vertical velocity
    float gravity;     // Gravity pulling the bird downwards
    float lift;        // Upwards force when the bird flaps
    float terminal_velocity;
    bool isColliding = false;
};

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        SDL_Log("Unable to initialize SDL_image: %s", IMG_GetError());
        return 1;
    }

    // load sprites
    SDL_Texture *birdTexture = IMG_LoadTexture(renderer, "sprites/bird.png");
    if (!birdTexture)
    {
        SDL_Log("Failed to load bird texture: %s", IMG_GetError());
        return 1;
    }

    SDL_Texture *upperPipeTexture = IMG_LoadTexture(renderer, "sprites/upper_pipe.png");
    if (!upperPipeTexture)
    {
        SDL_Log("Failed to load upper_pipe texture: %s", IMG_GetError());
        return 1;
    }

    SDL_Texture *lowerPipeTexture = IMG_LoadTexture(renderer, "sprites/lower_pipe.png");
    if (!lowerPipeTexture)
    {
        SDL_Log("Failed to load lower_pipe texture: %s", IMG_GetError());
        return 1;
    }

    SDL_Texture *bgTexture = IMG_LoadTexture(renderer, "sprites/background.png");
    if (!bgTexture)
    {
        SDL_Log("Failed to load background texture: %s", IMG_GetError());
        // handle the error appropriately
    }

    bool isRunning = true;
    SDL_Event event;
    int score = 0;
    bool gameOver = false;

    // Bird instance (player)

    Bird bird;
    bird.x = WINDOW_WIDTH / 8;  // starting position
    bird.y = WINDOW_HEIGHT / 2; // starting position
    bird.width = 50;            // adjust based on your sprite
    bird.height = 50;           // adjust based on your sprite
    bird.velocity = 0;
    bird.gravity = 0.008;
    bird.lift = -3;
    bird.terminal_velocity = 6;

    std::list<Pipe> pipes;
    Uint32 lastPipeSpawnTime = SDL_GetTicks();
    const Uint32 PIPE_SPAWN_INTERVAL = 2000;

    while (isRunning)
    {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastPipeSpawnTime > PIPE_SPAWN_INTERVAL)
        {
            Pipe newPipe;
            newPipe.x = WINDOW_WIDTH; // Assuming WINDOW_WIDTH is the width of your game window
            newPipe.gapY = (rand() % (WINDOW_HEIGHT - Pipe::GAP_HEIGHT)) + Pipe::GAP_HEIGHT / 2;
            pipes.push_back(newPipe);
            lastPipeSpawnTime = currentTime;
        }
        // Move all pipes to the left
        for (Pipe &pipe : pipes)
        {
            pipe.x -= 1; // Adjust the speed as needed
        }

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                isRunning = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE))
            {
                bird.velocity += bird.lift;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        SDL_Rect bgRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderCopy(renderer, bgTexture, NULL, &bgRect);

        // drawing the bird
        SDL_Rect birdRect = {bird.x, bird.y, bird.width, bird.height};
        SDL_RenderCopy(renderer, birdTexture, NULL, &birdRect);

        // Bird physics
        bird.velocity += bird.gravity;
        bird.y += bird.velocity;

        // Optional: Add a terminal velocity
        if (abs(bird.velocity) > abs(bird.terminal_velocity))
        {
            bird.velocity = bird.terminal_velocity;
        }

        if (bird.y < 0)
        {
            bird.y = 0;
            bird.velocity = 0; // stop the bird from moving upwards
        }
        else if (bird.y + bird.height > WINDOW_HEIGHT)
        {
            bird.y = WINDOW_HEIGHT - bird.height;
            // Here you can also handle the game over logic if the bird hits the ground
        }

        // Drawing Pipes
        for (const Pipe &pipe : pipes)
        {
            // Upper pipe
            SDL_Rect upperDest = {pipe.x, 0, Pipe::PIPE_WIDTH, pipe.gapY - Pipe::GAP_HEIGHT / 2};
            SDL_RenderCopy(renderer, upperPipeTexture, NULL, &upperDest);

            // Lower pipe
            SDL_Rect lowerDest = {pipe.x, pipe.gapY + Pipe::GAP_HEIGHT / 2, Pipe::PIPE_WIDTH, WINDOW_HEIGHT - (pipe.gapY + Pipe::GAP_HEIGHT / 2)};
            SDL_RenderCopy(renderer, lowerPipeTexture, NULL, &lowerDest);
        }

        // Removing Off-screen Pipes
        while (!pipes.empty() && pipes.front().x + Pipe::PIPE_WIDTH < 0)
        {
            pipes.pop_front();
        }

        bool hasCollided = false; // Local flag for this frame
        for (Pipe &pipe : pipes)
        {
            if (!pipe.passed && bird.x > pipe.x + Pipe::PIPE_WIDTH)
            {
                score++;
                pipe.passed = true;
                std::cout << "Score: " << score << std::endl;
            }

            if (bird.x + bird.width > pipe.x && bird.x < pipe.x + Pipe::PIPE_WIDTH)
            {
                if (bird.y < pipe.gapY - Pipe::GAP_HEIGHT / 2 || bird.y + bird.height > pipe.gapY + Pipe::GAP_HEIGHT / 2)
                {
                    hasCollided = true;
                    break;
                }
            }
        }

        if (hasCollided && !bird.isColliding) // If there's a collision now, but there wasn't in the last frame
        {
            std::cout << "Bird collided with pipe!" << std::endl;
            bird.isColliding = true; // Update the global flag
            gameOver = true;
        }
        else if (!hasCollided) // If there's no collision now
        {
            bird.isColliding = false; // Reset the global flag
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(birdTexture);
    SDL_DestroyTexture(upperPipeTexture);
    SDL_DestroyTexture(lowerPipeTexture);
    SDL_DestroyTexture(bgTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
