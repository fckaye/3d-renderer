#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "upng.h"
#include "array.h"
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "matrix.h"
#include "light.h"
#include "texture.h"
#include "triangle.h"
#include "camera.h"
#include "clipping.h"

////////////////////////////////////////////////////////////////////
// Array of triangles to be rendered frame by frame
////////////////////////////////////////////////////////////////////
#define MAX_TRIANGLES_PER_MESH 10000
triangle_t triangles_to_render[MAX_TRIANGLES_PER_MESH];
int num_triangles_to_render = 0;

////////////////////////////////////////////////////////////////////
// Global variables for execution status and game loop
////////////////////////////////////////////////////////////////////
bool is_running = false;
int previous_frame_time = 0;
float delta_time = 0;

////////////////////////////////////////////////////////////////////
// Declaration of global transformation matrices
////////////////////////////////////////////////////////////////////
mat4_t world_matrix;
mat4_t view_matrix;
mat4_t proj_matrix;

////////////////////////////////////////////////////////////////////
// Setup function to initialize variables and game obejcts
////////////////////////////////////////////////////////////////////
void setup(void)
{
    // Initialize Render mode and culling mode
    render_method = RENDER_WIRE;
    cull_method = CULL_BACKFACE;

    // Allocate memory in bytes to hold color buffer and z buffer
    color_buffer = (uint32_t *)malloc(sizeof(uint32_t) * window_width * window_height);
    z_buffer = (float *)malloc(sizeof(float) * window_width * window_height);

    // Create SDL texture used to display the color buffer
    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height);

    // Initialize perspective projection matrix
    float fov = M_PI / 3.0; // Same as 180 / 3 i.e. 60deg but in rad
    float aspect = (float)window_height / (float)window_width;
    float z_near = 0.1;
    float z_far = 100.0;
    proj_matrix = mat4_make_perspective(fov, aspect, z_near, z_far);

    // Initialize frustum planes with point and normal
    init_frustum_planes(fov, z_near, z_far);

    // Loads cube values into mesh data structure
    load_obj_file_data("./assets/cube.obj");

    // Load the texture information from an external png file
    load_png_teture_data("./assets/cube.png");
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
            is_running = false;
        if (event.key.keysym.sym == SDLK_1)
            render_method = RENDER_WIRE_VERTEX;
        if (event.key.keysym.sym == SDLK_2)
            render_method = RENDER_WIRE;
        if (event.key.keysym.sym == SDLK_3)
            render_method = RENDER_FILL_TRIANGLE;
        if (event.key.keysym.sym == SDLK_4)
            render_method = RENDER_FILL_TRIANGLE_WIRE;
        if (event.key.keysym.sym == SDLK_5)
            render_method = RENDER_TEXTURED;
        if (event.key.keysym.sym == SDLK_6)
            render_method = RENDER_TEXTURED_WIRE;
        if (event.key.keysym.sym == SDLK_c)
            cull_method = CULL_BACKFACE;
        if (event.key.keysym.sym == SDLK_x)
            cull_method = CULL_NONE;
        if (event.key.keysym.sym == SDLK_UP)
            camera.position.y += 3.0 * delta_time;
        if (event.key.keysym.sym == SDLK_DOWN)
            camera.position.y -= 3.0 * delta_time;
        if (event.key.keysym.sym == SDLK_a)
            camera.yaw -= 1.0 * delta_time;
        if (event.key.keysym.sym == SDLK_d)
            camera.yaw += 1.0 * delta_time;
        if (event.key.keysym.sym == SDLK_w)
        {
            camera.forward_velocity = vec3_mul(camera.direction, 5.0 * delta_time);
            camera.position = vec3_add(camera.position, camera.forward_velocity);
        }
        if (event.key.keysym.sym == SDLK_s)
        {
            camera.forward_velocity = vec3_mul(camera.direction, 5.0 * delta_time);
            camera.position = vec3_sub(camera.position, camera.forward_velocity);
        }
        break;
    }
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

    // Get a delta time factor converted to seconds to be used to update game movements
    delta_time = (SDL_GetTicks() - previous_frame_time) / 1000.0;

    // SDL_GetTicks returns number of ms since app started
    previous_frame_time = SDL_GetTicks();

    // Initialize the counter of triangles to render for this frame
    num_triangles_to_render = 0;

    // Change the mesh rotation and scale values per frame.
    // mesh.rotation.x += 0.1 * delta_time;
    // mesh.rotation.y += 0.1 * delta_time;
    // mesh.rotation.z += 0.1 * delta_time;
    mesh.translation.z = 5.0;

    // Find the target point based on camera forward velocity and yaw.
    // Initialize the target as a normalized positive z axis
    vec3_t target = {0, 0, 1};
    mat4_t camera_yaw_rotation = mat4_make_rotation_y(camera.yaw);
    camera.direction = vec3_from_vec4(mat4_mul_vec4(camera_yaw_rotation, vec4_from_vec3(target)));

    // Offset the camera position in the direction the camera is pointing at
    target = vec3_add(camera.position, camera.direction);
    vec3_t up_direction = {0, 1, 0};

    // Create the view matrix
    mat4_t view_matrix = mat4_look_at(camera.position, target, up_direction);

    // Create a scale matrix to multiply the mesh vertices
    mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
    mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);

    // Loop all triangle faces of mesh.
    int num_faces = array_length(mesh.faces);
    for (int i = 0; i < num_faces; i++)
    {
        if (i != 4)
        {
            continue;
        }

        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh.vertices[mesh_face.a];
        face_vertices[1] = mesh.vertices[mesh_face.b];
        face_vertices[2] = mesh.vertices[mesh_face.c];

        vec4_t transformed_vertices[3];

        // Loop 3 vertices of current faces and apply transformations
        for (int j = 0; j < 3; j++)
        {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Create a World Matrix combining scale, rotation and translation matrices
            world_matrix = mat4_identity();
            // Order matters. Scale -> Rotation -> Translation. [T]*[R]*[S]*v
            world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
            world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
            world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

            // Multiply the world matrix by the original vector
            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);

            // Multiply the view matrix by the original vector to transform the scene to camera space
            transformed_vertex = mat4_mul_vec4(view_matrix, transformed_vertex);

            // Save the transformed vertex unto the array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        // Check for backface culling
        vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A   */
        vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \  */
        vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C - B */

        // Get vectors A to B and A to C
        vec3_t vector_ab = vec3_sub(vector_b, vector_a);
        vec3_t vector_ac = vec3_sub(vector_c, vector_a);
        vec3_normalize(&vector_ab);
        vec3_normalize(&vector_ac);

        // Get perpendicular from cross product and normalize
        vec3_t normal = vec3_cross(vector_ab, vector_ac);
        vec3_normalize(&normal);

        // Get Camera to A position.
        vec3_t origin = {0, 0, 0};
        vec3_t camera_ray = vec3_sub(origin, vector_a);

        // Get dot product of camera ray and face normal
        float dot_cam_normal = vec3_dot(normal, camera_ray);

        // Apply backface culling
        if (cull_method == CULL_BACKFACE)
        {
            // Bypass triangles that look away from camera.
            bool cull = dot_cam_normal < 0;
            if (cull)
                continue;
        }

        // Create a polygon from the original triangle to be clipped
        polygon_t polygon = create_polygon_from_triangle(
            vec3_from_vec4(transformed_vertices[0]),
            vec3_from_vec4(transformed_vertices[1]),
            vec3_from_vec4(transformed_vertices[2]));

        // TODO: After clipping, break the polygon in triangles
        clip_polygon(&polygon);

        // if (polygon.num_vertices > 3)
        // {
        printf("Number of polygon vertices after clipping: %d \n", polygon.num_vertices);
        // }

        vec4_t projected_points[3];

        // Look all 3 vertices to perform projection
        for (int j = 0; j < 3; j++)
        {
            // Project the current vertex
            projected_points[j] = mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);

            // Scale projected points to half window size
            projected_points[j].x *= (window_width / 2.0);
            projected_points[j].y *= (window_height / 2.0);

            // Invert y values to account for flipped screen y coordinates.
            projected_points[j].y *= -1;

            // Translate projected point to middle of screen
            projected_points[j].x += (window_width / 2.0);
            projected_points[j].y += (window_height / 2.0);
        }

        // Calculate shade intensity based on alignment of the triangle normal and the inverse of the light direction
        float light_intensity_factor = -vec3_dot(normal, light.direction);
        // Calculate triangle color based on light angle
        uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

        triangle_t projected_triangle = {
            .points = {
                {projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w},
                {projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w},
                {projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w}},
            .texcoords = {
                {mesh_face.a_uv.u, mesh_face.a_uv.v},
                {mesh_face.b_uv.u, mesh_face.b_uv.v},
                {mesh_face.c_uv.u, mesh_face.c_uv.v},
            },
            .color = triangle_color,
        };

        // Save the projected triangle in array of screen space triangles
        if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH)
        {
            triangles_to_render[num_triangles_to_render] = projected_triangle;
            num_triangles_to_render++;
        }
    }
}

////////////////////////////////////////////////////////////////////
// Render objects on display
////////////////////////////////////////////////////////////////////
void render(void)
{
    // Loop projected points and render them
    for (int i = 0; i < num_triangles_to_render; i++)
    {
        triangle_t triangle = triangles_to_render[i];

        // Draw filled triangle faces
        if (render_method == RENDER_FILL_TRIANGLE ||
            render_method == RENDER_FILL_TRIANGLE_WIRE)
        {
            draw_filled_triangle(
                triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w,
                triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w,
                triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w,
                triangle.color);
        }

        // Draw textured triangle
        if (render_method == RENDER_TEXTURED ||
            render_method == RENDER_TEXTURED_WIRE)
        {
            draw_textured_triangle(
                triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v, // vertex A
                triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v, // vertex B
                triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v, // vertex C
                mesh_texture);
        }

        // Draw unfilled triangle edges
        if (render_method == RENDER_WIRE ||
            render_method == RENDER_WIRE_VERTEX ||
            render_method == RENDER_FILL_TRIANGLE_WIRE ||
            render_method == RENDER_TEXTURED_WIRE)
        {
            draw_triangle(
                triangle.points[0].x,
                triangle.points[0].y,
                triangle.points[1].x,
                triangle.points[1].y,
                triangle.points[2].x,
                triangle.points[2].y,
                0xFF00FF00);
        }

        // Draw vertex points
        if (render_method == RENDER_WIRE_VERTEX)
        {
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6, 0xFFFF0000);
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6, 0xFFFF0000);
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6, 0xFFFF0000);
        }
    }

    render_color_buffer();
    clear_color_buffer(0x00000000);
    clear_z_buffer();

    SDL_RenderPresent(renderer);
}

////////////////////////////////////////////////////////////////////
// Free memory that was dynamically allocated by program.
////////////////////////////////////////////////////////////////////
void free_resources(void)
{
    free(color_buffer);
    free(z_buffer);
    upng_free(png_texture);
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
