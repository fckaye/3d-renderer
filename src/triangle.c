#include "triangle.h"
#include "display.h"
#include <stdint.h>

// TODO: Create implementation of triangle.h functions
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

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
        swap(&y0, &y1);
        swap(&x0, &x1);
    }
    if (y1 > y2)
    {
        swap(&y1, &y2);
        swap(&x1, &x2);
    }
    if (y0 > y1)
    {
        swap(&y0, &y1);
        swap(&x0, &x1);
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