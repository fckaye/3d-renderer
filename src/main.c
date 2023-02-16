#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "array.h"
#include "display.h"
#include "vector.h"
#include "mesh.h"

////////////////////////////////////////////////////////////////////
// Array of triangles to be rendered frame by frame
////////////////////////////////////////////////////////////////////
triangle_t *triangles_to_render = NULL;

////////////////////////////////////////////////////////////////////
// Global variables for execution status and game loop
////////////////////////////////////////////////////////////////////
vec3_t camera_position = {.x = 0, .y = 0, .z = -5};

float fov_factor = 640;
bool is_running = false;
int previous_frame_time = 0;

////////////////////////////////////////////////////////////////////
// Setup function to initialize variables and game obejcts
////////////////////////////////////////////////////////////////////
void setup(void)
{
    // Allocate memory in bytes to hold color buffer
    color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);

    // Create SDL texture used to display the color buffer
    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height);

    // Loads cube values into mesh data structure
    load_cube_mesh_data();
}

////////////////////////////////////////////////////////////////////
// Poll system events and handle keyboard input
////////////////////////////////////////////////////////////////////
void process_input(void)
{
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

/////////////////////////////////////////////////////
// Gets a 3D vector and returns a projected 2D point
/////////////////////////////////////////////////////
vec2_t project(vec3_t point)
{
    vec2_t projected_point = {
        .x = (fov_factor * point.x) / point.z,
        .y = (fov_factor * point.y) / point.z};
    return projected_point;
}

////////////////////////////////////////////////////////////////////
// Update frames by a fixed timestep
////////////////////////////////////////////////////////////////////
void update(void)
{
    // Wait some time until the reaching target frame time in ms
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);

    // Delay execution if going too fast
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
    {
        SDL_Delay(time_to_wait);
    }

    // SDL_GetTicks returns number of ms since app started
    previous_frame_time = SDL_GetTicks();

    // Initialize the array of triangles to render
    triangles_to_render = NULL;

    mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.01;
    mesh.rotation.z += 0.01;

    // Loop all triangle faces of mesh.
    int num_faces = array_length(mesh.faces);
    for (int i = 0; i < num_faces; i++)
    {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a - 1];
        face_vertices[1] = mesh.vertices[mesh_face.b - 1];
        face_vertices[2] = mesh.vertices[mesh_face.c - 1];

        triangle_t projected_triangle;

        // Loop 3 vertices of current faces and apply transformations
        for (int j = 0; j < 3; j++)
        {
            vec3_t transformed_vertex = face_vertices[j];

            transformed_vertex = vec3_rotate_x(transformed_vertex, mesh.rotation.x);
            transformed_vertex = vec3_rotate_y(transformed_vertex, mesh.rotation.y);
            transformed_vertex = vec3_rotate_z(transformed_vertex, mesh.rotation.z);

            // Translate the vertex away from camera
            transformed_vertex.z -= camera_position.z;

            // Project the current vertex
            vec2_t projected_point = project(transformed_vertex);

            // Scale and translate projected point to middle of screen
            projected_point.x += window_width / 2;
            projected_point.y += window_height / 2;

            projected_triangle.points[j] = projected_point;
        }

        // Save the projected triangle in array of screen space triangles
        array_push(triangles_to_render, projected_triangle);
    }
}

////////////////////////////////////////////////////////////////////
// Render objects on display
////////////////////////////////////////////////////////////////////
void render(void)
{
    // Loop projected points and render them
    int num_triangles = array_length(triangles_to_render);
    for (int i = 0; i < num_triangles; i++)
    {
        triangle_t triangle = triangles_to_render[i];

        // Draw vertex points
        draw_rect(triangle.points[0].x, triangle.points[0].y, 5, 5, 0xFFFFFF00);
        draw_rect(triangle.points[1].x, triangle.points[1].y, 5, 5, 0xFFFFFF00);
        draw_rect(triangle.points[2].x, triangle.points[2].y, 5, 5, 0xFFFFFF00);

        // Draw unfilled triangle edges
        draw_triangle(
            triangle.points[0].x,
            triangle.points[0].y,
            triangle.points[1].x,
            triangle.points[1].y,
            triangle.points[2].x,
            triangle.points[2].y,
            0xFF00FF00);
    }

    // Clear the array of triangles to render every frame
    array_free(triangles_to_render);

    render_color_buffer();
    clear_color_buffer(0x00000000);

    SDL_RenderPresent(renderer);
}

////////////////////////////////////////////////////////////////////
// Free memory that was dynamically allocated by program.
////////////////////////////////////////////////////////////////////
void free_resources(void)
{
    free(color_buffer);
    array_free(mesh.faces);
    array_free(mesh.vertices);
}

////////////////////////////////////////////////////////////////////
// Main function (entry point of application).
////////////////////////////////////////////////////////////////////
int main(void)
{
    is_running = initialize_window();
    setup();

    while (is_running)
    {
        process_input();
        update();
        render();
    }

    destroy_window();
    free_resources();
    return 0;
}