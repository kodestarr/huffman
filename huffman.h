#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long ull;
typedef unsigned char uchar;

#define max_char 256
#define magic_string "iuc-aic,ae=vb;abvj?bjhbb'vrb%vb*ab"
#define magic_len (sizeof(magic_string) - 1)

typedef struct Node
{
    uchar byte;
    ull freq;
    struct Node *left, *right;
} Node;
typedef struct
{
    Node **arr;
    ull size, capacity;
} MinHeap;
typedef struct
{
    uchar bits[max_char];
    ull length;
} Code;

int compress(const char *input, const char *output);
int decompress(const char *input, const char *output);
int compress_folder(const char *input, const char *output);
int is_directory(const char *path);
int compress_to_buffer(FILE *input, uchar **output, ull *output_len, const char *file_name);

extern int show_progress;
#endif