#include "huffman.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#define INITIAL_BUFFER_SIZE 4096
#define APPEND_BYTE(buf, buf_len, buf_cap, byte)                         \
    do                                                                   \
    {                                                                    \
        if ((buf_len) >= (buf_cap))                                      \
        {                                                                \
            (buf_cap) = (buf_cap) ? (buf_cap) * 2 : INITIAL_BUFFER_SIZE; \
            (buf) = realloc((buf), (buf_cap));                           \
        }                                                                \
        (buf)[(buf_len)++] = (byte);                                     \
    } while (0)

typedef struct
{
    int ch, len;
} Symbol;
static int cmp(const void *a, const void *b)
{
    Symbol *sa = (Symbol *)a, *sb = (Symbol *)b;
    if (sa->len != sb->len)
        return sa->len - sb->len;
    return sa->ch - sb->ch;
}
static void compute_depth(Node *node, int depth, int *length)
{
    if (node)
    {
        if (!node->left && !node->right)
        {
            length[node->byte] = depth;
            return;
        }
        compute_depth(node->left, depth + 1, length);
        compute_depth(node->right, depth + 1, length);
    }
}
static void free_tree(Node *node)
{
    if (node)
    {
        free_tree(node->left);
        free_tree(node->right);
        free(node);
    }
}
static Node *create_node(uchar byte, ull freq, Node *left, Node *right)
{
    Node *node = (Node *)malloc(sizeof(Node));

    node->byte = byte;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}
static MinHeap *create_minheap(ull capacity)
{
    MinHeap *minheap = (MinHeap *)malloc(sizeof(MinHeap));
    minheap->arr = (Node **)malloc(capacity * sizeof(Node *));
    minheap->capacity = capacity;
    minheap->size = 0;

    return minheap;
}
static void destroy_minheap(MinHeap *heap)
{
    free(heap->arr);
    free(heap);
}
static void heap_push(MinHeap *heap, Node *node)
{
    int i = heap->size++;
    while (i > 0)
    {
        int parent = (i - 1) / 2;
        if (heap->arr[parent]->freq <= node->freq)
            break;
        heap->arr[i] = heap->arr[parent];
        i = parent;
    }
    heap->arr[i] = node;
}
static Node *heap_pop(MinHeap *heap)
{
    Node *min = heap->arr[0];
    Node *last = heap->arr[--heap->size];

    int i = 0;
    while (1)
    {
        int left = 2 * i + 1, right = 2 * i + 2, smallest = i;
        if (left < heap->size && heap->arr[left]->freq < last->freq)
            smallest = left;
        if (right < heap->size && heap->arr[right]->freq < heap->arr[smallest]->freq)
            smallest = right;
        if (smallest == i)
            break;
        heap->arr[i] = heap->arr[smallest];
        i = smallest;
    }
    heap->arr[i] = last;
    return min;
}
// static void generate_codes(Node *root, uchar *path, ull depth, Code *codes)
// {
//     if (root->left)
//     {
//         path[depth] = 0;
//         generate_codes(root->left, path, depth + 1, codes);
//     }
//     if (root->right)
//     {
//         path[depth] = 1;
//         generate_codes(root->right, path, depth + 1, codes);
//     }
//     if (!root->left && !root->right)
//     {
//         codes[root->byte].length = depth;
//         for (int i = 0; i < depth; i++)
//             codes[root->byte].bits[i] = path[i];
//         return;
//     }
// }
static void animation(const char *filename, double progress)
{
    extern int show_progress;
    if (!show_progress)
        return;

    int name_len = strlen(filename);
    int max_name_len = 15;
    name_len = name_len > max_name_len ? max_name_len : name_len;

    int bar_width = 60 - name_len - 1;
    int filled_len = (int)(bar_width * progress + 0.5);
    if (filled_len > bar_width)
        filled_len = bar_width;
    if (filled_len < 0)
        filled_len = 0;

    printf("\r%.*s", name_len, filename);
    for (int i = 0; i < filled_len; i++)
        printf("*");
    for (int i = filled_len; i < bar_width; i++)
        printf(" ");
    printf(" %.2f%% ", progress * 100.0);
    fflush(stdout);
}
int compress_to_buffer(FILE *fin, uchar **out_buffer, ull *out_len, const char *filename)
{
    ull freq[max_char] = {0};
    ull total_bytes = 0, compressed_bytes = 0;
    int c;

    while ((c = fgetc(fin)) != EOF)
    {
        freq[(uchar)c]++;
        total_bytes++;
    }

    Node *root = NULL;
    MinHeap *heap = create_minheap(max_char * 2);

    for (int i = 0; i < max_char; i++)
    {
        if (freq[i] > 0)
        {
            Node *leaf = create_node((uchar)i, freq[i], NULL, NULL);
            heap_push(heap, leaf);
        }
    }

    while (heap->size > 1)
    {
        Node *left = heap_pop(heap);
        Node *right = heap_pop(heap);
        Node *parent = create_node(0, left->freq + right->freq, left, right);
        heap_push(heap, parent);
    }
    root = heap_pop(heap);
    destroy_minheap(heap);

    int length[max_char] = {0};
    compute_depth(root, 0, length);

    Symbol syms[max_char];
    int sym_count = 0;
    for (int i = 0; i < max_char; i++)
    {
        if (length[i])
        {
            syms[sym_count].ch = i;
            syms[sym_count].len = length[i];
            sym_count++;
        }
    }
    qsort(syms, sym_count, sizeof(Symbol), cmp);

    Code codes[max_char] = {0};
    ull current_code = 0;
    int prev_len = 0;
    for (int i = 0; i < sym_count; i++)
    {
        int ch = syms[i].ch;
        int len = syms[i].len;
        current_code <<= (len - prev_len);
        codes[ch].length = len;
        for (int j = 0; j < len; j++)
        {
            int bit = (current_code >> (len - 1 - j)) & 1;
            codes[ch].bits[j] = bit;
        }
        current_code++;
        prev_len = len;
    }

    uchar *buf = NULL;
    ull buf_len = 0, buf_cap = 0;
    for (ull i = 0; i < magic_len; i++)
        APPEND_BYTE(buf, buf_len, buf_cap, magic_string[i]);
    int cnt = sym_count;
    for (int i = 0; i < sizeof(int); i++)
        APPEND_BYTE(buf, buf_len, buf_cap, ((uchar *)&cnt)[i]);
    for (int i = 0; i < max_char; i++)
        if (freq[i] > 0)
        {
            APPEND_BYTE(buf, buf_len, buf_cap, (uchar)i);
            APPEND_BYTE(buf, buf_len, buf_cap, (uchar)codes[i].length);
        }

    rewind(fin);
    uchar buffer = 0;
    ull buffer_len = 0, processed_bytes = 0;

    while ((c = fgetc(fin)) != EOF)
    {
        c = (uchar)c;
        processed_bytes++;
        int len = codes[c].length;
        for (int i = 0; i < len; i++)
        {
            buffer = buffer << 1 | codes[c].bits[i];
            buffer_len++;
            if (buffer_len == 8)
            {
                APPEND_BYTE(buf, buf_len, buf_cap, buffer);
                buffer = 0, buffer_len = 0;
            }
        }

        ull report_interval = total_bytes / 100;
        if (report_interval == 0)
            report_interval = 1;
        if ((processed_bytes % report_interval == 0 || processed_bytes == total_bytes) && show_progress)
        {
            double progress = total_bytes > 0 ? (double)processed_bytes / total_bytes : 1.0;
            animation(filename, progress);
        }
    }
    if (buffer_len)
    {
        buffer <<= (8 - buffer_len);
        APPEND_BYTE(buf, buf_len, buf_cap, buffer);
    }
    uchar padding = (uchar)buffer_len;
    APPEND_BYTE(buf, buf_len, buf_cap, padding);

    free(root);
    *out_buffer = buf, *out_len = buf_len;

    return 0;
}
int compress(const char *input, const char *output)
{
    FILE *fin = fopen(input, "rb");
    if (fin)
    {
        uchar *buf = NULL;
        ull len = 0;
        int ret = compress_to_buffer(fin, &buf, &len, input);
        fclose(fin);

        FILE *fout = fopen(output, "wb");
        if (fout)
        {
            fwrite(buf, 1, len, fout);
            fclose(fout);
        }
        free(buf);
        // printf("\n");
    }
    return 0;
}
int is_directory(const char *path)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if (stat(path, &st) == 0)
    {
        return S_ISDIR(st.st_mode);
    }
    return 0;
#endif
}
static void collect_files_impl(const char *dir_path, char ***file_list, ull *file_count, ull *capacity)
{
#ifdef _WIN32
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", dir_path);
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
            {
                char full_path[MAX_PATH];
                snprintf(full_path, MAX_PATH, "%s\\%s", dir_path, find_data.cFileName);
                for (char *p = full_path; *p; p++)
                {
                    if (*p == '\\')
                        *p = '/';
                }
                // (*file_list)[(*file_count)++] = _strdup(full_path);

                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    collect_files_impl(full_path, file_list, file_count, capacity);
                }
                else
                {
                    if (*file_count >= *capacity)
                    {
                        *capacity = *capacity ? *capacity * 2 : 16;
                        *file_list = realloc(*file_list, *capacity * sizeof(char *));
                    }
                    (*file_list)[(*file_count)++] = _strdup(full_path);
                }
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(dir_path);
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                char full_path[MAX_PATH];
                snprintf(full_path, MAX_PATH, "%s/%s", dir_path, entry->d_name);
                struct stat st;
                if (stat(full_path, &st) == 0)
                {
                    if (S_ISDIR(st.st_mode))
                    {
                        collect_files_impl(full_path, file_list, file_count, capacity);
                    }
                    else if (S_ISREG(st.st_mode))
                    {
                        if (*file_count >= *capacity)
                        {
                            *capacity = *capacity ? *capacity * 2 : 16;
                            *file_list = realloc(*file_list, *capacity * sizeof(char *));
                        }
                        (*file_list)[(*file_count)++] = strdup(full_path);
                    }
                }
            }
        }
        closedir(dir);
    }
#endif
}
static int collect_files(const char *dir_path, char ***file_list, ull *file_count)
{
    *file_list = NULL;
    *file_count = 0;
    ull capacity = 16;
    *file_list = malloc(capacity * sizeof(char *));
    collect_files_impl(dir_path, file_list, file_count, &capacity);
    return 0;
    //     *file_list = NULL;
    //     *file_count = 0;

    // #ifdef _WIN32
    //     char search_path[MAX_PATH];
    //     snprintf(search_path, MAX_PATH, "%s\\*", dir_path);
    //     WIN32_FIND_DATAA find_data;
    //     HANDLE hFind = FindFirstFileA(search_path, &find_data);
    //     if (hFind != INVALID_HANDLE_VALUE)
    //     {
    //         do
    //         {
    //             if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
    //             {
    //                 char full_path[MAX_PATH];
    //                 snprintf(full_path, MAX_PATH, "%s\\%s", dir_path, find_data.cFileName);

    //                 if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    //                 {
    //                     collect_files(full_path, file_list, file_count);
    //                 }
    //                 else
    //                 {
    //                     *file_list = realloc(*file_list, (*file_count + 1) * sizeof(char *));
    //                     (*file_list)[(*file_count)++] = _strdup(full_path);
    //                 }
    //             }
    //         } while (FindNextFileA(hFind, &find_data));
    //         FindClose(hFind);
    //     }
    // #else
    //     DIR *dir = opendir(dir_path);
    //     if (dir)
    //     {
    //         struct dirent *entry;
    //         while ((entry = readdir(dir)) != NULL)
    //         {
    //             if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
    //             {
    //                 char file_path[PATH_MAX];
    //                 snprintf(file_path, PATH_MAX, "%s/%s", dir_path, entry->d_name);
    //                 if (*file_count >= capacity)
    //                 {
    //                     capacity *= 2;
    //                     *file_list = realloc(*file_list, capacity * sizeof(char *));
    //                 }
    //                 (*file_list)[(*file_count)++] = strdup(file_path);
    //             }
    //         }
    //         closedir(dir);
    //     }
    // #endif
    //     return 0;
}

int compress_folder(const char *input, const char *output)
{
    char input_fixed[PATH_MAX];
    strncpy(input_fixed, input, PATH_MAX - 1);
    input_fixed[PATH_MAX - 1] = '\0';
    for (char *p = input_fixed; *p; p++)
    {
        if (*p == '\\')
            *p = '/';
    }

    char **file_list = NULL;
    ull file_count = 0;

    ull orig_size = 0, compressed_size = 0;

    collect_files(input_fixed, &file_list, &file_count);
    FILE *fout = fopen(output, "wb");

    fwrite(magic_string, 1, magic_len, fout);
    fwrite(&file_count, sizeof(file_count), 1, fout);
    for (ull i = 0; i < file_count; i++)
    {
        const char *rel_path = file_list[i] + strlen(input_fixed);
        if (*rel_path == '/' || *rel_path == '\\')
            rel_path++;

        // const char *file_path = strrchr(file_list[i], '\\');
        // if (file_path)
        // {
        //     file_path++;
        // }
        // else
        // {
        //     file_path = strrchr(file_list[i], '/');
        //     if (file_path)
        //         file_path = file_path++;
        //     else
        //         file_path = file_list[i];
        // }

        uchar name_len = (uchar)strlen(rel_path);
        fwrite(&name_len, 1, 1, fout);
        fwrite(rel_path, 1, name_len, fout);

        FILE *fin = fopen(file_list[i], "rb");
        if (fin)
        {
            fseek(fin, 0, SEEK_END);
            ull file_size = ftell(fin);
            orig_size += file_size;
            rewind(fin);
        }
        uchar *compressed_data = NULL;
        ull compressed_len = 0;
        int ret = compress_to_buffer(fin, &compressed_data, &compressed_len, rel_path);
        fclose(fin);
        if (!ret)
        {
            fwrite(&compressed_len, sizeof(compressed_len), 1, fout);
            fwrite(compressed_data, 1, compressed_len, fout);
        }
        free(compressed_data);
        if (show_progress)
            printf("\n");
    }

    compressed_size = ftell(fout);
    for (ull i = 0; i < file_count; i++)
        free(file_list[i]);
    free(file_list);
    fclose(fout);
    printf("Folder compressed from %llu files to %s\n", file_count, output);
    printf("Total original size: %llu bytes\n", orig_size);
    printf("Total compressed file size (including headers): %llu bytes\n", compressed_size);
    if (orig_size > 0)
    {
        double ratio = (double)compressed_size / orig_size * 100.0;
        printf("Compression ratio: %.2f%%\n", ratio);
    }
    else
    {
        printf("Compression ratio: N/A (empty folder)\n");
    }

    return 0;
}