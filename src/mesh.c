#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"
#include "mesh.h"

mesh_t mesh = {
    .vertices = NULL,
    .faces = NULL,
    .rotation = {0, 0, 0}};

vec3_t cube_vertices[N_CUBE_VERTICES] = {
    {.x = -1, .y = -1, .z = -1},
    {.x = -1, .y = 1, .z = -1},
    {.x = 1, .y = 1, .z = -1},
    {.x = 1, .y = -1, .z = -1},
    {.x = 1, .y = 1, .z = 1},
    {.x = 1, .y = -1, .z = 1},
    {.x = -1, .y = 1, .z = 1},
    {.x = -1, .y = -1, .z = 1}};

face_t cube_faces[N_CUBE_FACES] = {
    // front
    {.a = 1, .b = 2, .c = 3},
    {.a = 1, .b = 3, .c = 4},
    // right
    {.a = 4, .b = 3, .c = 5},
    {.a = 4, .b = 5, .c = 6},
    // back
    {.a = 6, .b = 5, .c = 7},
    {.a = 6, .b = 7, .c = 8},
    // left
    {.a = 8, .b = 7, .c = 2},
    {.a = 8, .b = 2, .c = 1},
    // top
    {.a = 2, .b = 7, .c = 5},
    {.a = 2, .b = 5, .c = 3},
    // bottom
    {.a = 6, .b = 8, .c = 1},
    {.a = 6, .b = 1, .c = 4}};

void load_cube_mesh_data(void)
{
    for (int i = 0; i < N_CUBE_VERTICES; i++)
    {
        vec3_t cube_vertex = cube_vertices[i];
        array_push(mesh.vertices, cube_vertex);
    }

    for (int i = 0; i < N_CUBE_FACES; i++)
    {
        face_t cube_face = cube_faces[i];
        array_push(mesh.faces, cube_face);
    }
}

void load_obj_file_data(char *filePath)
{
    FILE *file;
    file = fopen(filePath, "r");

    char line[1024];
    while (fgets(line, 1024, file))
    {
        // Line is vertex line
        if (strncmp(line, "v ", 1) == 0)
        {
            vec3_t vertex;
            sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
            array_push(mesh.vertices, vertex);
        }
        // Line is face line
        if (strncmp(line, "f ", 1) == 0)
        {
            int vertex_indices[3];
            int texture_indices[3];
            int normal_indices[3];
            sscanf(
                line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                &vertex_indices[0], &texture_indices[0], &normal_indices[0],
                &vertex_indices[1], &texture_indices[1], &normal_indices[1],
                &vertex_indices[2], &texture_indices[2], &normal_indices[2]);
            face_t face = {
                .a = vertex_indices[0],
                .b = vertex_indices[1],
                .c = vertex_indices[2]};
            array_push(mesh.faces, face);
        }
    }
}

void load_obj_file_data_ky(char *filePath)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filePath, "r");
    if (fp == NULL)
        return;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        // printf("'%s'\n", line);
        analyze_line(line);
    }

    fclose(fp);
    if (line)
        free(line);
}

void analyze_line(char *line)
{
    char *delim = " ";
    char *lineCopy = NULL;
    lineCopy = strdup(line);
    printf("\n\n'%s'\n", lineCopy);
    char *savePtr1;

    char *wordsPtr = strtok_r(lineCopy, delim, &savePtr1);

    printf("words[0] = '%c'\n", wordsPtr[0]);
    printf("wordsPtr len: '%lu'\n", strlen(wordsPtr));
    if (wordsPtr[0] == 'v' && strlen(wordsPtr) == 1)
    {
        wordsPtr = strtok_r(NULL, delim, &savePtr1);
        printf("x, wordsPtr: '%s'\n", wordsPtr);
        float xVal = atof(wordsPtr);
        wordsPtr = strtok_r(NULL, delim, &savePtr1);
        printf("y, wordsPtr: '%s'\n", wordsPtr);
        float yVal = atof(wordsPtr);
        wordsPtr = strtok_r(NULL, delim, &savePtr1);
        printf("z, wordsPtr: '%s'\n", wordsPtr);
        float zVal = atof(wordsPtr);

        printf("\n FOUND VERTEX \n");
        printf("x: '%f' y: '%f' z: '%f' \n", xVal, yVal, zVal);

        vec3_t vert = {xVal, yVal, zVal};
        array_push(mesh.vertices, vert);
    }

    if (wordsPtr[0] == 'f' && strlen(wordsPtr) == 1)
    {
        wordsPtr = strtok_r(NULL, delim, &savePtr1);
        printf("a, wordsPtr: '%s'\n", wordsPtr);
        // int a = (int)wordsPtr[0];
        int a = get_face_data(wordsPtr);

        wordsPtr = strtok_r(NULL, delim, &savePtr1);
        printf("b, wordsPtr: '%s'\n", wordsPtr);
        // int b = (int)wordsPtr[0];
        int b = get_face_data(wordsPtr);

        wordsPtr = strtok_r(NULL, delim, &savePtr1);
        printf("c, wordsPtr: '%s'\n", wordsPtr);
        // int c = (int)wordsPtr[0];
        int c = get_face_data(wordsPtr);

        printf("\n FOUND FACE \n");
        printf("x: '%d' y: '%d' z: '%d' \n", a, b, c);
        face_t face = {a, b, c};
        array_push(mesh.faces, face);
    }

    free(lineCopy);
}

int get_face_data(char *faceDataItem)
{
    char *delim = "/";
    char *lineCopy = NULL;
    lineCopy = strdup(faceDataItem);
    char *savePtr2;

    char *itemsPtr = strtok_r(lineCopy, delim, &savePtr2);

    printf("First item: '%s'\n", itemsPtr);
    int faceIndex = atoi(itemsPtr);
    return faceIndex;

    free(lineCopy);
}