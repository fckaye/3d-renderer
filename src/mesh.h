#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"

#define N_CUBE_VERTICES 8
#define N_CUBE_FACES (6 * 2)

extern vec3_t cube_vertices[N_CUBE_VERTICES];
extern face_t cube_faces[N_CUBE_FACES];

////////////////////////////////////////////////////////////////////
// Define a struct for a dynamic sized mesh.
////////////////////////////////////////////////////////////////////
typedef struct
{
    vec3_t *vertices;   // Dynamic array of vertices
    face_t *faces;      // Dynamic array of faces
    vec3_t rotation;    // Rotation as x,y,z euler angles.
    vec3_t scale;       // Scale with x,y,z values
    vec3_t translation; // Translation with x,y,z values.
} mesh_t;

extern mesh_t mesh;

void load_cube_mesh_data(void);

void load_obj_file_data(char *filePath);

void load_obj_file_data_ky(char *filePath);

void analyze_line(char *line);

int get_face_data(char *faceDataItem);

#endif