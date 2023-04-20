#include <stdint.h>
#include "triangle.h"
#include "display.h"
#include "swap.h"

////////////////////////////////////////////////////////////////////
// Draw a Filled Triangle with a flat-bottom
////////////////////////////////////////////////////////////////////
//
//               (x0,y0)
//                / x \
//              / x x x \
//            / x x x x x \
//          / x x x x x x x \
//      (x1,y1)----------(x2,y2)
//
////////////////////////////////////////////////////////////////////
void fill_flat_bottom_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color)
{
    // Find inverted slope1 and slope2 from 2 triangle legs.
    float inv_slope_1 = (float)(x1 - x0) / (y1 - y0);
    float inv_slope_2 = (float)(x2 - x0) / (y2 - y0);

    // Start x_start and x_end from the top vertex (x0,y0)
    float x_start = x0;
    float x_end = x0;

    // Loop scanlines from top to bottom.
    for (int y = y0; y <= y2; y++)
    {
        // Draw a horizontal line from x_start to x_end
        draw_line(x_start, y, x_end, y, color);

        // Determine the new values for x_start and x_end.
        x_start += inv_slope_1;
        x_end += inv_slope_2;
    }
}

////////////////////////////////////////////////////////////////////
// Draw a Filled Triangle with a flat-top
////////////////////////////////////////////////////////////////////
//
//      (x0,y0)----------(x1,y1)
//          \ x x x x x x x /
//            \ x x x x x /
//              \ x x x /
//                \ x /
//               (x2,y2)
//
////////////////////////////////////////////////////////////////////
void fill_flat_top_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color)
{
    // Find inverted slope1 and slope2 from 2 triangle legs.
    float inv_slope_1 = (float)(x2 - x0) / (y2 - y0);
    float inv_slope_2 = (float)(x2 - x1) / (y2 - y1);

    // Start x_start and x_end from the top vertex (x0,y0)
    float x_start = x2;
    float x_end = x2;

    // Loop scanlines from bottom to top.
    for (int y = y2; y >= y0; y--)
    {
        // Draw a horizontal line from x_start to x_end
        draw_line(x_start, y, x_end, y, color);

        // Determine the new values for x_start and x_end.
        x_start -= inv_slope_1;
        x_end -= inv_slope_2;
    }
}

////////////////////////////////////////////////////////////////////
// Draw a Filled Triangle using flat-top/flat-bottom technique.
// Split the original triangle in 2, half flat-bottom half flat-top
//
//               (x0,y0)
//                / \
//              /    \
//            /       \
//          /          \
//      (x1,y1)-------(Mx,My)
//           \__         \
//              \__       \
//                 \__     \
//                    \__   \
//                       \__ \
//                         (x2,y2)
//
////////////////////////////////////////////////////////////////////
void draw_filled_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color)
{
    // Sort the vertices by ascending y-coordinate values (y0 < y1 < y2)
    if (y0 > y1)
    {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
    }
    if (y1 > y2)
    {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
    }
    if (y0 > y1)
    {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
    }

    if (y1 == y2)
    {
        // If the triangle already has a flat bottom, just pass it like that.
        fill_flat_bottom_triangle(x0, y0, x1, y1, x2, y2, color);
        return;
    }
    else if (y0 == y1)
    {
        // If the triangle already has a flat top, just pass it like that.
        fill_flat_top_triangle(x0, y0, x1, y1, x2, y2, color);
        return;
    }
    else
    {
        // Not flat horizontal side triangle, divide by midpoint and
        // perform flat-bottom flat-top approach.

        // Calculate the vertex (Mx,My) using trinagle similarity.
        int My = y1;
        int Mx = ((float)(x2 - x0) * (My - y0) / (float)(y2 - y0)) + x0;

        // Draw Flat-bottom triangle.
        fill_flat_bottom_triangle(x0, y0, x1, y1, Mx, My, color);

        // Draw Flat-top triangle.
        fill_flat_top_triangle(x1, y1, Mx, My, x2, y2, color);
    }
}

////////////////////////////////////////////////////////////////////
// Return the barycentric weights alpha, beta and gamma for point p
////////////////////////////////////////////////////////////////////
//
//            B
//          / | \
//         /  |  \
//        /   |   \
//       /   (p)   \
//      /  /     \  \
//     / /         \ \
//    A ------------- C
//
////////////////////////////////////////////////////////////////////
vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p)
{
    // Find the vectors between vertices ABC and point P
    vec2_t ac = vec2_sub(c, a);
    vec2_t ab = vec2_sub(b, a);
    vec2_t ap = vec2_sub(p, a);
    vec2_t pc = vec2_sub(c, p);
    vec2_t pb = vec2_sub(b, p);

    // Calculate the area of the full parallelogram/triangle ABC using 2D cross product
    float area_parallelogram_abc = (ac.x * ab.y - ac.y * ab.x); // || AC x AB ||

    // Alpha is the area of the small parallelogram/triangle PBC divided by the area of the full parallelogram/triangle ABC
    float alpha = (pc.x * pb.y - pc.y * pb.x) / area_parallelogram_abc;

    // Beta is the area of the small parallelogram/triangle APC divided by the area of the full parallelogram/triangle ABC
    float beta = (ac.x * ap.y - ac.y * ap.x) / area_parallelogram_abc;

    // Gamma can be inferred by the difference of alpha, beta and 1
    float gamma = 1 - alpha - beta;

    vec3_t weights = {alpha, beta, gamma};
    return weights;
}

////////////////////////////////////////////////////////////////////
// Draw a Textured pixel at position (x,y) using interpolation.
////////////////////////////////////////////////////////////////////
void draw_texel(
    int x, int y, uint32_t *texture,
    vec4_t point_a, vec4_t point_b, vec4_t point_c,
    tex2_t a_uv, tex2_t b_uv, tex2_t c_uv)
{
    vec2_t p = {x, y};
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);
    vec3_t weights = barycentric_weights(a, b, c, p);

    float alpha = weights.x;
    float beta = weights.y;
    float gamma = weights.z;

    // Variables to store interpoalted values of u,v and 1/w for current pixel
    float interpolated_u;
    float interpolated_v;
    float interpolated_reciprocal_w;

    // Perform the interpolation of u/w and v/w values using barycentric weights and a factor of 1/w
    interpolated_u = (a_uv.u / point_a.w) * alpha + (b_uv.u / point_b.w) * beta + (c_uv.u / point_c.w) * gamma;
    interpolated_v = (a_uv.v / point_a.w) * alpha + (b_uv.v / point_b.w) * beta + (c_uv.v / point_c.w) * gamma;

    // Also interpolate value of 1/w for the current pixel.
    interpolated_reciprocal_w = (1 / point_a.w) * alpha + (1 / point_b.w) * beta + (1 / point_c.w) * gamma;

    // Divide back both interpolated u and v by 1/w.
    interpolated_u /= interpolated_reciprocal_w;
    interpolated_v /= interpolated_reciprocal_w;

    // Map the UV coordinate to the full texture width and height
    int tex_x = abs((int)(interpolated_u * texture_width));
    int tex_y = abs((int)(interpolated_v * texture_height));

    draw_pixel(x, y, texture[(tex_y * texture_width) + tex_x]);
}

////////////////////////////////////////////////////////////////////
// Draw a Textured Triangle using flat-top/flat-bottom technique.
// Split the original triangle in 2, half flat-bottom half flat-top
//
//                 v0
//                / \
//              /    \
//            /       \
//          /          \
//         v1 --------- v3
//           \__         \
//              \__       \
//                 \__     \
//                    \__   \
//                       \__ \
//                           v2
//
////////////////////////////////////////////////////////////////////
void draw_textured_triangle(
    int x0, int y0, float z0, float w0, float u0, float v0,
    int x1, int y1, float z1, float w1, float u1, float v1,
    int x2, int y2, float z2, float w2, float u2, float v2,
    uint32_t *texture)
{
    // Sort the vertices by ascending Y coordinates y0 < y1 < y2
    if (y0 > y1)
    {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }
    if (y1 > y2)
    {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
        float_swap(&u1, &u2);
        float_swap(&v1, &v2);
    }
    if (y0 > y1)
    {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }

    // Flip the V component to account for inverted UV-coordinates.
    // V grows downwards, (origin from top-left)
    v0 = 1 - v0;
    v1 = 1 - v1;
    v2 = 1 - v2;

    // Create vector points after sorting the vertices
    vec4_t point_a = {x0, y0, z0, w0};
    vec4_t point_b = {x1, y1, z1, w1};
    vec4_t point_c = {x2, y2, z2, w2};
    tex2_t a_uv = {u0, v0};
    tex2_t b_uv = {u1, v1};
    tex2_t c_uv = {u2, v2};

    ///////////////////////////////////////////////////////
    // Render the upper part of the triangle (flat-bottom)
    ///////////////////////////////////////////////////////
    float inverse_slope_1 = 0;
    float inverse_slope_2 = 0;

    if (y1 - y0 != 0)
        inverse_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0)
        inverse_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0)
    {
        for (int y = y0; y <= y1; y++)
        {
            int x_start = x1 + (y - y1) * inverse_slope_1;
            int x_end = x0 + (y - y0) * inverse_slope_2;

            if (x_end < x_start)
                int_swap(&x_start, &x_end); // Swap if x_start is to the is to the right of x_end

            for (int x = x_start; x < x_end; x++)
            {
                draw_texel(
                    x, y, texture,
                    point_a, point_b, point_c,
                    a_uv, b_uv, c_uv);
            }
        }
    }

    ///////////////////////////////////////////////////////
    // Render the lower part of the triangle (flat-top)
    ///////////////////////////////////////////////////////
    inverse_slope_1 = 0;
    inverse_slope_2 = 0;

    if (y2 - y1 != 0)
        inverse_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if (y2 - y1 != 0)
        inverse_slope_2 = (float)(x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0)
    {
        for (int y = y1; y <= y2; y++)
        {
            int x_start = x1 + (y - y1) * inverse_slope_1;
            int x_end = x0 + (y - y0) * inverse_slope_2;

            if (x_end < x_start)
                int_swap(&x_start, &x_end); // Swap if x_start is to the is to the right of x_end

            for (int x = x_start; x < x_end; x++)
            {
                draw_texel(
                    x, y, texture,
                    point_a, point_b, point_c,
                    a_uv, b_uv, c_uv);
            }
        }
    }
}