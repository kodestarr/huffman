#include "huffman.h"
#include <time.h>
#ifdef _WIN32
#define CLEAR_SCREEN "cls"
#include <windows.h>
#else
#define CLEAR_SCREEN "clear"
#include <glob.h>
#include <sys/stat.h>
#endif

int show_progress = 1;
int is_directory(const char *path);
int compress_folder(const char *input, const char *output);
int interactive_mode();
static int expand_wildcard(const char *pattern, char ***files, ull *count)
{
    *files = NULL, *count = 0;
    ull capacity = 16;
    *files = malloc(capacity * sizeof(char *));

#ifdef _WIN32
    char prefix[MAX_PATH] = "";
    const char *last_slash1 = strrchr(pattern, '\\');
    const char *last_slash2 = strrchr(pattern, '/');
    if (last_slash2 > last_slash1)
        last_slash1 = last_slash2;
    if (last_slash1)
    {

        ull len = last_slash1 - pattern + 1;
        memcpy(prefix, pattern, len);
        prefix[len] = '\0';
    }
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (*count >= capacity)
                {
                    capacity *= 2;
                    *files = realloc(*files, capacity * sizeof(char *));
                }
                char full_path[MAX_PATH];
                snprintf(full_path, MAX_PATH, "%s%s", prefix, fd.cFileName);
                (*files)[(*count)++] = strdup(full_path);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
#else
    glob_t glob_buf;
    glob(pattern, GLOB_TILDE, NULL, &glob_buf);
    for (ull i = 0; i < glob_buf.gl_pathc; i++)
    {
        struct stat st;
        if (stat(glob_buf.gl_pathv[i], &st) == 0 && S_ISREG(st.st_mode))
        {
            if (*count >= capacity)
            {
                capacity *= 2;
                *files = realloc(*files, capacity * sizeof(char *));
            }
            (*files)[(*count)++] = strdup(glob_buf.gl_pathv[i]);
        }
    }
    globfree(&glob_buf);
#endif
    return 0;
}
int main()
{
    interactive_mode();
    // if (argc == 3)
    // {
    //     const char *infile = argv[1], *outfile = argv[2];

    //     if (is_directory(infile))
    //     {
    //         printf("Compressing folder...\n");
    //         clock_t start = clock();
    //         int result = compress_folder(infile, outfile);
    //         clock_t end = clock();
    //         double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    //         printf("Folder compression time: %.3f seconds\n", cpu_time);
    //         return result;
    //     }
    //     else
    //     {
    //         FILE *fin = fopen(infile, "rb");
    //         if (fin)
    //         {
    //             uchar magic_buf[magic_len + 1] = {0};
    //             ull read_len = fread(magic_buf, 1, magic_len, fin);
    //             fclose(fin);

    //             clock_t start, end;
    //             double cpu_time;
    //             int result;

    //             if (read_len == magic_len && !memcmp(magic_buf, magic_string, magic_len))
    //             {
    //                 printf("Decompressing...\n");
    //                 start = clock();
    //                 result = decompress(infile, outfile);
    //                 end = clock();
    //                 cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    //                 printf("Decompression time: %.3f seconds\n", cpu_time);
    //                 return result;
    //             }
    //             else
    //             {
    //                 printf("Compressing...\n");
    //                 start = clock();
    //                 result = compress(infile, outfile);
    //                 end = clock();
    //                 cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    //                 printf("Compression time: %.3f seconds\n", cpu_time);
    //                 return result;
    //             }
    //         }
    //     }
    // }
}

int interactive_mode()
{
    char src[512];
    char dst[512];
    int exit_flag = 0;

    const char *RESET = "\033[0m";
    const char *RED = "\033[31m";
    const char *GREEN = "\033[32m";
    const char *YELLOW = "\033[33m";
    const char *BLUE = "\033[34m";
    const char *MAGENTA = "\033[35m";
    const char *CYAN = "\033[36m";
    const char *BOLD = "\033[1m";

    while (!exit_flag)
    {
        system(CLEAR_SCREEN);
        printf("%s================================================================%s\n", CYAN, RESET);
        printf("%s                       HUFFMAN COMPRESSION TOOL               %s\n", BOLD, RESET);
        printf("%s================================================================%s\n", CYAN, RESET);

        // Source path input
        printf("%s[Source]%s\n", GREEN, RESET);
        printf("Enter source file/directory path (or 'exit' to quit):\n> ");
        if (fgets(src, sizeof(src), stdin) == NULL)
            break;
        src[strcspn(src, "\n")] = '\0';
        if (strcmp(src, "exit") == 0 || strcmp(src, "quit") == 0)
            break;

        // Destination path input
        printf("%s[Destination]%s\n", GREEN, RESET);
        printf("Enter destination path:\n> ");
        if (fgets(dst, sizeof(dst), stdin) == NULL)
            break;
        dst[strcspn(dst, "\n")] = '\0';

        // Progress option
        printf("%s[Progress]%s\n", GREEN, RESET);
        printf("Show detailed progress? (y/N): ");
        char choice[2];
        if (fgets(choice, sizeof(choice), stdin))
        {
            show_progress = (choice[0] == 'y' || choice[0] == 'Y');
        }
        // consume extra newline
        fgets(choice, sizeof(choice), stdin);

        clock_t start, end;
        double cpu_time;
        int result = -1;

        if (strchr(src, '*') || strchr(src, '?'))
        {
            printf("%s[INFO]%s Expanding wildcard pattern...\n", BLUE, RESET);
            char **file_list = NULL;
            ull file_count = 0;
            expand_wildcard(src, &file_list, &file_count);
            if (file_count > 0)
            {
                printf("\n%s[INFO]%s Compressing %llu files matching pattern...\n", BLUE, RESET, file_count);
                start = clock();
                FILE *fout = fopen(dst, "wb");
                fwrite(magic_string, 1, magic_len, fout);
                fwrite(&file_count, sizeof(file_count), 1, fout);
                for (ull i = 0; i < file_count; i++)
                {
                    const char *file_name = strrchr(file_list[i], '/') ? strrchr(file_list[i], '/') + 1 : file_list[i];
                    uchar name_len = (uchar)strlen(file_name);
                    fwrite(&name_len, 1, 1, fout);
                    fwrite(file_name, 1, name_len, fout);

                    FILE *fin = fopen(file_list[i], "rb");
                    if (fin)
                    {
                        uchar *buf = NULL;
                        ull len = 0;
                        compress_to_buffer(fin, &buf, &len, file_name);
                        fclose(fin);
                        fwrite(&len, sizeof(len), 1, fout);
                        fwrite(buf, 1, len, fout);
                        free(buf);
                        if (show_progress)
                            printf("\n");
                    }
                }
                end = clock();
                cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
                printf("%sTime taken: %.3f seconds%s\n", YELLOW, cpu_time, RESET);
                for (ull i = 0; i < file_count; i++)
                    free(file_list[i]);
                free(file_list);
                fclose(fout);
            }
            else
            {
                printf("\n%s[ERROR]%s No files found matching pattern '%s'%s\n", RED, RESET, src, RESET);
            }
        }
        else if (is_directory(src))
        {
            printf("\n%s[INFO]%s Compressing folder...\n", BLUE, RESET);
            start = clock();
            result = compress_folder(src, dst);
            end = clock();
            cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
            if (result == 0)
                printf("%s[SUCCESS]%s Folder compressed. %sTime: %.3f seconds%s\n", GREEN, RESET, YELLOW, cpu_time, RESET);
            else
                printf("%s[ERROR]%s Folder compression failed (error code: %d)%s\n", RED, RESET, result, RESET);
        }
        else
        {
            FILE *fin = fopen(src, "rb");
            if (fin == NULL)
            {
                printf("\n%s[ERROR]%s Cannot open source file '%s'%s\n", RED, RESET, src, RESET);
            }
            else
            {
                unsigned char magic_buf[magic_len + 1] = {0};
                size_t read_len = fread(magic_buf, 1, magic_len, fin);
                fclose(fin);
                if (read_len == magic_len && memcmp(magic_buf, magic_string, magic_len) == 0)
                {
                    printf("\n%s[INFO]%s Compressed file detected. Decompressing...\n", BLUE, RESET);
                    start = clock();
                    result = decompress(src, dst);
                    end = clock();
                    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
                    if (result == 0)
                        printf("%s[SUCCESS]%s Decompression done. %sTime: %.3f seconds%s\n", GREEN, RESET, YELLOW, cpu_time, RESET);
                    else
                        printf("%s[ERROR]%s Decompression failed (error code: %d)%s\n", RED, RESET, result, RESET);
                }
                else
                {
                    printf("\n%s[INFO]%s Compressing file...\n", BLUE, RESET);
                    start = clock();
                    result = compress(src, dst);
                    end = clock();
                    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
                    if (result == 0)
                        printf("%s[SUCCESS]%s Compression done. %sTime: %.3f seconds%s\n", GREEN, RESET, YELLOW, cpu_time, RESET);
                    else
                        printf("%s[ERROR]%s Compression failed (error code: %d)%s\n", RED, RESET, result, RESET);
                }
            }
        }
        printf("\n%s[Press Enter to continue]%s", CYAN, RESET);
        while (getchar() != '\n')
            ;
    }
    printf("\n%sGoodbye!%s\n", MAGENTA, RESET);
    return 0;
}

// int interactive_mode()
// {
//     char src[512];
//     char dst[512];
//     int exit_flag = 0;

//     while (!exit_flag)
//     {
//         system(CLEAR_SCREEN);
//         printf("================================================================\n");
//         printf("                Huffman Compression Tool \n");
//         printf("================================================================\n");
//         printf("Enter source path (file/directory) or 'exit' to quit:\n> ");
//         if (fgets(src, sizeof(src), stdin) == NULL)
//         {
//             break;
//         }
//         src[strcspn(src, "\n")] = '\0';
//         if (strcmp(src, "exit") == 0 || strcmp(src, "quit") == 0)
//         {
//             break;
//         }
//         printf("Enter destination path:\n> ");
//         if (fgets(dst, sizeof(dst), stdin) == NULL)
//         {
//             break;
//         }
//         dst[strcspn(dst, "\n")] = '\0';

//         printf("Show progress? (y/n): ");
//         char choice[2];
//         if (fgets(choice, sizeof(choice), stdin))
//         {
//             show_progress = (choice[0] == 'y' || choice[0] == 'Y');
//         }
//         fgets(choice, sizeof(choice), stdin);

//         clock_t start, end;
//         double cpu_time;
//         int result = -1;

//         if (is_directory(src))
//         {
//             printf("\nCompressing folder...\n");
//             start = clock();
//             result = compress_folder(src, dst);
//             end = clock();
//             cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
//             if (result == 0)
//             {
//                 printf("Folder compressed successfully.\n");
//             }
//             else
//             {
//                 printf("Folder compression failed (error code %d).\n", result);
//             }
//             printf("Time: %.3f seconds\n", cpu_time);
//         }
//         else
//         {
//             FILE *fin = fopen(src, "rb");
//             if (fin == NULL)
//             {
//                 printf("\nError: Cannot open source file '%s'.\n", src);
//             }
//             else
//             {
//                 unsigned char magic_buf[magic_len + 1] = {0};
//                 size_t read_len = fread(magic_buf, 1, magic_len, fin);
//                 fclose(fin);
//                 if (read_len == magic_len && memcmp(magic_buf, magic_string, magic_len) == 0)
//                 {
//                     printf("\nDecompressing...\n");
//                     start = clock();
//                     result = decompress(src, dst);
//                     end = clock();
//                     cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
//                     if (result == 0)
//                     {
//                         printf("Decompression successful.\n");
//                     }
//                     else
//                     {
//                         printf("Decompression failed (error code %d).\n", result);
//                     }
//                     printf("Time: %.3f seconds\n", cpu_time);
//                 }
//                 else
//                 {
//                     printf("\nCompressing...\n");
//                     start = clock();
//                     result = compress(src, dst);
//                     end = clock();
//                     cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
//                     if (result == 0)
//                     {
//                         printf("Compression successful.\n");
//                     }
//                     else
//                     {
//                         printf("Compression failed (error code %d).\n", result);
//                     }
//                     printf("Time: %.3f seconds\n", cpu_time);
//                 }
//             }
//         }
//         printf("\nPress Enter to continue...");
//         while (getchar() != '\n')
//             ;
//     }
//     printf("Goodbye!\n");
// }