#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"
#include "upng.h"

////////////////////////////////////////////////////////////////////
// Define a struct for a dynamic sized mesh.
////////////////////////////////////////////////////////////////////
typedef struct
{
    vec3_t *vertices;   // Dynamic array of vertices
    face_t *faces;      // Dynamic array of faces
    upng_t *texture;    // Mesh PNG texture pointer
    vec3_t rotation;    // Rotation as x,y,z euler angles.
    vec3_t scale;       // Scale with x,y,z values
    vec3_t translation; // Translation with x,y,z values.
} mesh_t;

void load_mesh(char *obj_filename, char *png_filename, vec3_t scale, vec3_t translation, vec3_t rotation);

void load_mesh_obj_data(mesh_t *mesh, char *obj_filename);

void load_mesh_png_data(mesh_t *mesh, char *png_filename);

int get_num_meshes(void);

mesh_t *get_mesh(int index);

void free_meshes(void);

#endif