#include "huffman.h"
#include <limits.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

typedef struct Decode
{
    uchar byte;
    int is_leaf;
    struct Decode *left, *right;
} Decode;
typedef struct
{
    uchar byte;
    uchar length;
} BitCode;
static Decode *create_decode()
{
    Decode *node = (Decode *)calloc(1, sizeof(Decode));
    return node;
}
static void free_decode(Decode *root)
{
    if (!root)
        return;
    free_decode(root->left);
    free_decode(root->right);
    free(root);
}
static void push_decode(Decode *root, ull code, int len, uchar byte)
{
    Decode *current = root;
    for (int i = len - 1; i >= 0; i--)
    {
        int bit = (code >> i) & 1;
        if (bit)
        {
            if (!current->right)
                current->right = create_decode();
            current = current->right;
        }
        else
        {
            if (!current->left)
                current->left = create_decode();
            current = current->left;
        }
    }
    current->is_leaf = 1, current->byte = byte;
}
static int cmp(const void *a, const void *b)
{
    BitCode *x = (BitCode *)a, *y = (BitCode *)b;
    if (x->length != y->length)
        return x->length - y->length;
    return x->byte - y->byte;
}
static int create_directory(const char *path)
{
#ifdef _WIN32
    return mkdir(path, 0777);
#else
    return mkdir(path, 0755);
#endif
}
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
static int mkdir_recursive(const char *path)
{
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    for (size_t i = 0; i < len; i++)
    {
        if (tmp[i] == '/' || tmp[i] == '\\')
        {
            char ch = tmp[i];
            tmp[i] = '\0';
            if (strlen(tmp) > 0)
            {
#ifdef _WIN32
                _mkdir(tmp);
#else
                mkdir(tmp, 0755);
#endif
            }
            tmp[i] = ch;
        }
    }
#ifdef _WIN32
    return _mkdir(tmp);
#else
    return mkdir(tmp, 0755);
#endif
}
static int decompress_buffer(uchar *buffer, ull buffer_len, const char *output, const char *filename)
{
    ull pos = magic_len;
    int cnt = 0;
    memcpy(&cnt, buffer + pos, sizeof(int));
    pos += sizeof(int);
    BitCode *tables = (BitCode *)malloc(cnt * sizeof(BitCode));
    for (int i = 0; i < cnt; i++)
        tables[i].byte = buffer[pos++], tables[i].length = buffer[pos++];

    qsort(tables, cnt, sizeof(BitCode), cmp);
    ull current = 0, previous = 0;
    typedef struct
    {
        uchar byte, length;
        ull code;
    } TableEntry;
    TableEntry *Tables = (TableEntry *)malloc(cnt * sizeof(TableEntry));
    for (int i = 0; i < cnt; i++)
    {
        int len = tables[i].length;
        current <<= (len - previous);
        Tables[i].byte = tables[i].byte;
        Tables[i].length = len;
        Tables[i].code = current;
        current++, previous = len;
    }
    free(tables);

    Decode *root = create_decode();
    for (int i = 0; i < cnt; i++)
        push_decode(root, Tables[i].code, Tables[i].length, Tables[i].byte);
    free(Tables);

    ull compressed_size = buffer_len - pos - 1;
    uchar padding = buffer[buffer_len - 1];
    uchar *compressed = buffer + pos;

    ull total_bits = compressed_size * 8;
    if (padding)
        total_bits -= 8 - padding;

    FILE *fout = fopen(output, "wb");
    int bit_pos = 0;
    Decode *current_node = root;

    while (bit_pos < total_bits)
    {
        int byte_idx = bit_pos / 8, bit_in_byte = bit_pos % 8;
        int bit = (compressed[byte_idx] >> (7 - bit_in_byte)) & 1;
        bit_pos++;

        current_node = bit ? current_node->right : current_node->left;
        if (current_node->is_leaf)
        {
            fputc(current_node->byte, fout);
            current_node = root;
        }
        if (bit_pos % (total_bits / 100) == 0 || bit_pos >= total_bits)
        {
            double progress = (double)bit_pos / total_bits;
            animation(filename, progress);
        }
    }
    fclose(fout);
    free_decode(root);
    return 0;
}
int decompress(const char *input, const char *output)
{
    FILE *fin = fopen(input, "rb");

    if (!fin)
    {
        perror("fopen");
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    ull file_end = ftell(fin);

    fseek(fin, magic_len, SEEK_SET);
    ull cnt = 0;
    fread(&cnt, 1, sizeof(ull), fin);
    ull after_header = ftell(fin);

    int is_multiple_files = 0;
    ull saved_pos = ftell(fin);
    uchar first_name_len;

    fread(&first_name_len, 1, sizeof(uchar), fin);
    char *name = (char *)malloc(first_name_len + 1);
    fread(name, 1, first_name_len, fin);
    name[first_name_len] = '\0';
    ull test_len;
    fread(&test_len, sizeof(ull), 1, fin);

    ull current_pos = ftell(fin);
    // fseek(fin, after_header, SEEK_SET);
    // ull file_end = ftell(fin);
    if (current_pos + test_len < file_end)
        is_multiple_files = 1;
    free(name);
    fseek(fin, saved_pos, SEEK_SET);

    if (is_multiple_files)
    {
        create_directory(output);
        for (int i = 0; i < cnt; i++)
        {
            uchar name_len;
            fread(&name_len, 1, sizeof(uchar), fin);
            char *file_name = (char *)malloc(name_len + 1);
            fread(file_name, 1, name_len, fin);
            file_name[name_len] = '\0';

            ull compressed_len;
            fread(&compressed_len, sizeof(ull), 1, fin);
            uchar *compressed_data = malloc(compressed_len);
            fread(compressed_data, 1, compressed_len, fin);

            char output_path[PATH_MAX];
            snprintf(output_path, sizeof(output_path), "%s/%s", output, file_name);

            char dir_buf[PATH_MAX];
            strcpy(dir_buf, output_path);
            char *last_sep1 = strrchr(dir_buf, '/');
            char *last_sep2 = strrchr(dir_buf, '\\');
            char *last_sep = last_sep1 > last_sep2 ? last_sep1 : last_sep2;
            if (last_sep)
                *last_sep = '\0';

            mkdir_recursive(dir_buf);

            decompress_buffer(compressed_data, compressed_len, output_path, file_name);

            free(compressed_data);
            free(file_name);
            if (show_progress)
                printf("\n");
        }
        fclose(fin);
    }
    else
    {
        fseek(fin, 0, SEEK_END);
        long file_len = ftell(fin);
        rewind(fin);

        uchar *buffer = malloc(file_len);
        fread(buffer, 1, file_len, fin);
        fclose(fin);
        int ret = decompress_buffer(buffer, file_len, output, output);
        free(buffer);
        if (show_progress)
            printf("\n");
    }

    // typedef struct
    // {
    //     uchar byte, length;
    // } BitCode;

    // BitCode *tables = (BitCode *)malloc(cnt * sizeof(BitCode));
    // for (int i = 0; i < cnt; i++)
    // {
    //     uchar byte, length;
    //     fread(&byte, 1, sizeof(uchar), fin);
    //     fread(&length, 1, sizeof(uchar), fin);
    //     tables[i].byte = byte;
    //     tables[i].length = length;
    // }

    // qsort(tables, cnt, sizeof(BitCode), cmp);

    // ull current = 0, previous = 0;
    // typedef struct
    // {
    //     uchar byte, length;
    //     ull code;
    // } TableEntry;
    // TableEntry *Tables = (TableEntry *)malloc(cnt * sizeof(TableEntry));
    // for (int i = 0; i < cnt; i++)
    // {
    //     int len = tables[i].length;
    //     current <<= (len - previous);
    //     Tables[i].byte = tables[i].byte;
    //     Tables[i].length = len;
    //     Tables[i].code = current;
    //     current++, previous = len;
    // }

    // Decode *root = create_decode();
    // for (int i = 0; i < cnt; i++)
    //     push_decode(root, Tables[i].code, Tables[i].length, Tables[i].byte);

    // fseek(fin, 0, SEEK_END);
    // ull size = ftell(fin);
    // ull header_size = magic_len + sizeof(int) + cnt * sizeof(BitCode);
    // fseek(fin, header_size, SEEK_SET);
    // ull compressed_size = size - header_size - 1;

    // uchar *compressed = malloc(compressed_size);
    // fread(compressed, 1, compressed_size, fin);
    // uchar padding = 0;
    // fread(&padding, 1, sizeof(uchar), fin);

    // ull total_bits = compressed_size * 8;
    // if (padding)
    //     total_bits -= 8 - padding;

    // int bit_pos = 0;
    // Decode *current_node = root;
    // ull total_bytes = 0;
    // while (bit_pos < total_bits)
    // {
    //     int byte_idx = bit_pos / 8, bit_in_byte = bit_pos % 8;
    //     int bit = (compressed[byte_idx] >> (7 - bit_in_byte)) & 1;
    //     bit_pos++;

    //     if (bit)
    //         current_node = current_node->right;
    //     else
    //         current_node = current_node->left;

    //     if (current_node->is_leaf)
    //     {
    //         fputc(current_node->byte, fout);
    //         total_bytes++, current_node = root;
    //     }
    // }
    // free(compressed);
    // free_decode(root);
    // fclose(fout);

    // printf("Decompressed %s to %s\n", input, output);
    // return 0;
}