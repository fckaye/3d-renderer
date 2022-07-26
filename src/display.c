#include "display.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
uint32_t* color_buffer = NULL;
SDL_Texture* color_buffer_texture = NULL;
int window_width = 800;
int window_height = 600;

bool initialize_window(void){
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Use SDL to query fullscreen width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);

    window_width = display_mode.w;
    window_height = display_mode.h;

    // Create SDL Window
    window = SDL_CreateWindow(
        NULL, 
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_BORDERLESS
    );
    if (!window)
    {
        fprintf(stderr, "Error creating SDL window.\n");
        return false;
    }

    // Create SDL Renderer
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        fprintf(stderr, "Error creating SDL renderer.\n");
        return false;
    }
    
    // Set Window to FullScreen
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    return true;
}

void draw_grid(int spacing, bool fill_border, uint32_t grid_color){
    for (int y = 0; y < window_height; y++)
    {
        for (int x = 0; x < window_width; x++)
        {
            if (y % spacing == 0 || x % spacing == 0 ||
                (fill_border == true && 
                (y == 0 || y == window_height - 1 ||
                x == 0 || x == window_width - 1)))
            {
                color_buffer[(window_width * y) + x] = grid_color;
                draw_pixel(x, y, grid_color);
            }            
        }
    }
}

void draw_pixel(int x, int y, uint32_t color){
    if (x < window_width &&
        y < window_height)
    {
        color_buffer[(window_width * y) + x] = color;
    }
}

void draw_rect(int x_pos, int y_pos, int width, int height, uint32_t color){

    for (int y = y_pos; y < y_pos + height; y++)
    {
        for (int x = x_pos; x < x_pos + width; x++)
        {
            draw_pixel(x, y, color);
            // color_buffer[(window_width * y) + x] = color;
        }
    }
}

void render_color_buffer(void){
    SDL_UpdateTexture(
        color_buffer_texture,
        NULL,
        color_buffer,
        (int)(window_width * sizeof(uint32_t))
    );
    SDL_RenderCopy(
        renderer,
        color_buffer_texture,
        NULL,
        NULL
    );
}

void clear_color_buffer(uint32_t color){
    for (int y = 0; y < window_height; y++)
    {
        for (int x = 0; x < window_width; x++)
        {
            color_buffer[(window_width * y) + x] = color;
        }
    }
}

void destroy_window(void){
    free(color_buffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}