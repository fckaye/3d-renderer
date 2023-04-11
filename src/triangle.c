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
    int x0, int y0, float u0, float v0,
    int x1, int y1, float u1, float v1,
    int x2, int y2, float u2, float v2,
    uint32_t* texture
){
    // Sort the vertices by ascending Y coordinates y0 < y1 < y2
    if (y0 > y1)
    {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }
    if (y1 > y2)
    {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&u1, &u2);
        float_swap(&v1, &v2);
    }
    if (y0 > y1)
    {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }

    ///////////////////////////////////////////////////////
    // Render the upper part of the triangle (flat-bottom)
    ///////////////////////////////////////////////////////
    float inverse_slope_1 = 0;
    float inverse_slope_2 = 0;

    if(y1 - y0 != 0) 
        inverse_slope_1 = (float)(x1 - x0) / abs(y1 - y0);
    if(y2 - y0 != 0) 
        inverse_slope_2 = (float)(x2 - x0) / abs(y2 - y0);    

    if(y1 - y0 != 0){
        for(int y = y0; y <= y1; y++){
            int x_start = x1 + (y - y1) * inverse_slope_1;
            int x_end = x0 + (y - y0) * inverse_slope_2;

            if(x_end < x_start)
                int_swap(&x_start, &x_end); // Swap if x_start is to the is to the right of x_end

            for(int x = x_start; x < x_end; x++){
                // TODO: draw pixel with the color that comes from the texture map.
                draw_pixel(x, y, 0xFFFF00FF);
            }
        }
    }
    
    ///////////////////////////////////////////////////////
    // Render the lower part of the triangle (flat-top)
    ///////////////////////////////////////////////////////
    inverse_slope_1 = 0;
    inverse_slope_2 = 0;

    if(y2 - y1 != 0) 
        inverse_slope_1 = (float)(x2 - x1) / abs(y2 - y1);
    if(y2 - y1 != 0) 
        inverse_slope_2 = (float)(x2 - x0) / abs(y2 - y0);    

    if(y2 - y1 != 0){
        for(int y = y1; y <= y2; y++){
            int x_start = x1 + (y - y1) * inverse_slope_1;
            int x_end = x0 + (y - y0) * inverse_slope_2;

            if(x_end < x_start)
                int_swap(&x_start, &x_end); // Swap if x_start is to the is to the right of x_end

            for(int x = x_start; x < x_end; x++){
                // TODO: draw pixel with the color that comes from the texture map.
                draw_pixel(x, y, (x % 2 == 0 && y % 2 == 0) ? 0xFFFF00FF : 0xFF000000);
            }
        }
    }
}