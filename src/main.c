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
    set_render_method(RENDER_WIRE);
    set_cull_method(CULL_BACKFACE);

    // Initialize scene light direction
    init_light(vec3_new(0, 0, 1));

    // Initialize perspective projection matrix
    float aspect_x = (float)get_window_width() / (float)get_window_height();
    float aspect_y = (float)get_window_height() / (float)get_window_width();
    float fov_y = M_PI / 3.0; // Same as 180 / 3 i.e. 60deg but in rad
    float fov_x = atan(tan(fov_y / 2) * aspect_x) * 2.0;
    float z_near = 0.1;
    float z_far = 100.0;
    proj_matrix = mat4_make_perspective(fov_y, aspect_y, z_near, z_far);

    // Initialize frustum planes with point and normal
    init_frustum_planes(fov_x, fov_y, z_near, z_far);

    // TODO: Load a mesh by passing the filepath of the obj file and the filepath of the texture
    // While also passing initial mesh scale, translation and rotation.
    load_mesh("./assets/runway.obj", "./assets/runway.png", vec3_new(1, 1, 1), vec3_new(0, -1.5, 23), vec3_new(0, 0, 0));
    load_mesh("./assets/f22.obj", "./assets/f22.png", vec3_new(1, 1, 1), vec3_new(0, -1.3, 5), vec3_new(0, -M_PI / 2, 0));
    load_mesh("./assets/efa.obj", "./assets/efa.png", vec3_new(1, 1, 1), vec3_new(-2, -1.3, 9), vec3_new(0, -M_PI / 2, 0));
    load_mesh("./assets/f117.obj", "./assets/f117.png", vec3_new(1, 1, 1), vec3_new(2, -1.3, 9), vec3_new(0, -M_PI / 2, 0));
}

////////////////////////////////////////////////////////////////////
// Poll system events and handle keyboard input
////////////////////////////////////////////////////////////////////
void process_input(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            is_running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                is_running = false;
                break;
            }
            if (event.key.keysym.sym == SDLK_1)
            {
                set_render_method(RENDER_WIRE_VERTEX);
                break;
            }
            if (event.key.keysym.sym == SDLK_2)
            {
                set_render_method(RENDER_WIRE);
                break;
            }
            if (event.key.keysym.sym == SDLK_3)
            {
                set_render_method(RENDER_FILL_TRIANGLE);
                break;
            }
            if (event.key.keysym.sym == SDLK_4)
            {
                set_render_method(RENDER_FILL_TRIANGLE_WIRE);
                break;
            }
            if (event.key.keysym.sym == SDLK_5)
            {
                set_render_method(RENDER_TEXTURED);
                break;
            }
            if (event.key.keysym.sym == SDLK_6)
            {
                set_render_method(RENDER_TEXTURED_WIRE);
                break;
            }
            if (event.key.keysym.sym == SDLK_c)
            {
                set_cull_method(CULL_BACKFACE);
                break;
            }
            if (event.key.keysym.sym == SDLK_x)
            {
                set_cull_method(CULL_NONE);
                break;
            }
            if (event.key.keysym.sym == SDLK_UP)
            {
                update_camera_forward_velocity(vec3_mul(get_camera_direction(), 5.0 * delta_time));
                update_camera_position(vec3_add(get_camera_position(), get_camera_forward_velocity()));
                break;
            }
            if (event.key.keysym.sym == SDLK_DOWN)
            {
                update_camera_forward_velocity(vec3_mul(get_camera_direction(), 5.0 * delta_time));
                update_camera_position(vec3_sub(get_camera_position(), get_camera_forward_velocity()));
                break;
            }
            if (event.key.keysym.sym == SDLK_w)
            {
                rotate_camera_pitch(+3.0 * delta_time);
                break;
            }
            if (event.key.keysym.sym == SDLK_s)
            {
                rotate_camera_pitch(-3.0 * delta_time);
                break;
            }
            if (event.key.keysym.sym == SDLK_RIGHT)
            {
                rotate_camera_yaw(+1.0 * delta_time);
                break;
            }
            if (event.key.keysym.sym == SDLK_LEFT)
            {
                rotate_camera_yaw(-1.0 * delta_time);
                break;
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Process the graphics pipeline stages for all the triangles on a mesh
///////////////////////////////////////////////////////////////////////////////
// +-------------+
// | Model space |  <-- original mesh vertices
// +-------------+
// |   +-------------+
// `-> | World space |  <-- multiply by world matrix
//     +-------------+
//     |   +--------------+
//     `-> | Camera space |  <-- multiply by view matrix
//         +--------------+
//         |    +------------+
//         `--> |  Clipping  |  <-- clip against the six frustum planes
//              +------------+
//              |    +------------+
//              `--> | Projection |  <-- multiply by projection matrix
//                   +------------+
//                   |    +-------------+
//                   `--> | Image space |  <-- apply perspective divide
//                        +-------------+
//                        |    +--------------+
//                        `--> | Screen space |  <-- ready to render
//                             +--------------+
///////////////////////////////////////////////////////////////////////////////
void process_graphic_pipeline_stages(mesh_t *mesh)
{
    // Create a scale matrix to multiply the mesh vertices
    mat4_t scale_matrix = mat4_make_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
    mat4_t translation_matrix = mat4_make_translation(mesh->translation.x, mesh->translation.y, mesh->translation.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh->rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh->rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh->rotation.z);

    // Update camera look at target to create view matrix
    vec3_t target = get_camera_lookat_target();
    vec3_t up_direction = vec3_new(0, 1, 0);
    mat4_t view_matrix = mat4_look_at(get_camera_position(), target, up_direction);

    // Loop all triangle faces of mesh.
    int num_faces = array_length(mesh->faces);
    for (int i = 0; i < num_faces; i++)
    {
        face_t mesh_face = mesh->faces[i];

        vec3_t face_vertices[3];
        face_vertices[0] = mesh->vertices[mesh_face.a];
        face_vertices[1] = mesh->vertices[mesh_face.b];
        face_vertices[2] = mesh->vertices[mesh_face.c];

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

        // Calculate the triangle face normal
        vec3_t face_normal = get_triangle_normal(transformed_vertices);

        // Apply backface culling
        if (is_cull_backface())
        {
            // Get Camera to A position.
            vec3_t camera_ray = vec3_sub(vec3_new(0, 0, 0), vec3_from_vec4(transformed_vertices[0]));

            // Get dot product of camera ray and face normal
            float dot_normal_camera = vec3_dot(face_normal, camera_ray);

            // Bypass triangles that look away from camera.
            if (dot_normal_camera < 0)
                continue;
        }

        // Create a polygon from the original triangle to be clipped
        polygon_t polygon = create_polygon_from_triangle(
            vec3_from_vec4(transformed_vertices[0]),
            vec3_from_vec4(transformed_vertices[1]),
            vec3_from_vec4(transformed_vertices[2]),
            mesh_face.a_uv,
            mesh_face.b_uv,
            mesh_face.c_uv);

        // Clip the polygon from the original transformed triangle to be clipped
        clip_polygon(&polygon);

        // Break the polygon apart back into individual triangles
        triangle_t triangles_after_clipping[MAX_NUM_POLY_TRIANGLES];
        int num_triangles_after_clipping = 0;

        triangles_from_polygon(&polygon, triangles_after_clipping, &num_triangles_after_clipping);

        // Loop the assembled triangles after clipping
        for (int t = 0; t < num_triangles_after_clipping; t++)
        {
            triangle_t triangle_after_clipping = triangles_after_clipping[t];

            vec4_t projected_points[3];

            // Look all 3 vertices to perform projection
            for (int j = 0; j < 3; j++)
            {
                // Project the current vertex
                projected_points[j] = mat4_mul_vec4_project(proj_matrix, triangle_after_clipping.points[j]);

                // Perform perspective divide
                // if (projected_points[j].w != 0)
                // {
                //     projected_points[j].x /= projected_points[j].w;
                //     projected_points[j].y /= projected_points[j].w;
                //     projected_points[j].z /= projected_points[j].w;
                // }

                // Invert y values to account for flipped screen y coordinates.
                projected_points[j].y *= -1;

                // Scale projected points to half window size
                projected_points[j].x *= (get_window_width() / 2.0);
                projected_points[j].y *= (get_window_height() / 2.0);

                // Translate projected point to middle of screen
                projected_points[j].x += (get_window_width() / 2.0);
                projected_points[j].y += (get_window_height() / 2.0);
            }

            // Calculate shade intensity based on alignment of the triangle normal and the inverse of the light direction
            float light_intensity_factor = -vec3_dot(face_normal, get_light_direction());

            // Calculate triangle color based on light angle
            uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

            triangle_t triangle_to_render = {
                .points = {
                    {projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w},
                    {projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w},
                    {projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w}},
                .texcoords = {
                    {triangle_after_clipping.texcoords[0].u, triangle_after_clipping.texcoords[0].v},
                    {triangle_after_clipping.texcoords[1].u, triangle_after_clipping.texcoords[1].v},
                    {triangle_after_clipping.texcoords[2].u, triangle_after_clipping.texcoords[2].v},
                },
                .color = triangle_color,
                .texture = mesh->texture};

            // Save the projected triangle in array of screen space triangles
            if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH)
            {
                triangles_to_render[num_triangles_to_render++] = triangle_to_render;
            }
        }
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

    // Loop through all the meshes on the scene (array of meshes)
    for (int mesh_index = 0; mesh_index < get_num_meshes(); mesh_index++)
    {
        mesh_t *mesh = get_mesh(mesh_index);

        // Change the mesh rotation and scale values per frame.
        // mesh.rotation.x += 0.1 * delta_time;
        // mesh.rotation.y += 0.1 * delta_time;
        // mesh.rotation.z += 0.1 * delta_time;
        // mesh.translation.z = 5.0;

        // Process the graphics pipeline stages for every mesh of the 3D scene
        process_graphic_pipeline_stages(mesh);
    }
}

////////////////////////////////////////////////////////////////////
// Render objects on display
////////////////////////////////////////////////////////////////////
void render(void)
{
    // Clear the buffers to prepare for the next frame
    clear_color_buffer(0x00000000);
    clear_z_buffer();

    // Loop projected points and render them
    for (int i = 0; i < num_triangles_to_render; i++)
    {
        triangle_t triangle = triangles_to_render[i];

        // Draw filled triangle faces
        if (should_render_filled_triangle())
        {
            draw_filled_triangle(
                triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w,
                triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w,
                triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w,
                triangle.color);
        }

        // Draw textured triangle
        if (should_render_textured_triangle())
        {
            draw_textured_triangle(
                triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v, // vertex A
                triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v, // vertex B
                triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v, // vertex C
                triangle.texture);
        }

        // Draw unfilled triangle edges
        if (should_render_wireframe())
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
        if (should_render_wire_vertex())
        {
            draw_rect(triangle.points[0].x - 3, triangle.points[0].y - 3, 6, 6, 0xFFFF0000);
            draw_rect(triangle.points[1].x - 3, triangle.points[1].y - 3, 6, 6, 0xFFFF0000);
            draw_rect(triangle.points[2].x - 3, triangle.points[2].y - 3, 6, 6, 0xFFFF0000);
        }
    }

    render_color_buffer();
}

////////////////////////////////////////////////////////////////////
// Free memory that was dynamically allocated by program.
////////////////////////////////////////////////////////////////////
void free_resources(void)
{
    free_meshes();
    destroy_window();
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

    free_resources();
    return 0;
}
