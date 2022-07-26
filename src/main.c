#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "display.h"

bool is_running = false;

void setup(void){
    // Allocate memory in bytes to hold color buffer
    color_buffer = (uint32_t*)malloc(sizeof(uint32_t) * window_width * window_height);

    // Create SDL texture used to display the color buffer
    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height
    );
}

void process_input(void){
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type)
    {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                is_running = false;
            }
            break;
    }
}

void update(void){
    // TODO:
}

void render(void){
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    uint32_t grid_color = 0xFFFFFFFF;
    draw_grid(50, true, grid_color);

    draw_rect(100, 100, 200, 200, 0xFF00FFFF);
    draw_rect(500, 1000, 25, 75, 0xFF330033);
    draw_rect(50, 100, 15, 400, 0x22222222);
    draw_pixel(25, 25, 0xFFFF00FF);
    
    render_color_buffer();
    clear_color_buffer(0x00000000);

    SDL_RenderPresent(renderer);
}

int main(void){
    is_running = initialize_window();
    setup();
    while (is_running)
    {
        process_input();
        update();
        render();
    }
    destroy_window();
    return 0;
}