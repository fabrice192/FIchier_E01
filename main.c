#include <stdio.h>
#include <stdlib.h>
#include <libewf.h>
#include <tsk/libtsk.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <pthread.h>
#include <wchar.h>
#define TSK_TCHAR wchar_t
#define tsk_tchar_to_utf8(str) (str)
#else
#include <pthread.h>
#define TSK_TCHAR char
#define tsk_tchar_to_utf8(str) (str)
#endif

#define BUFFER_SIZE 1048576  // 1 MB buffer

int file_count = 0;
pthread_mutex_t file_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_write_mutex = PTHREAD_MUTEX_INITIALIZER;
FILE *output_file;
char buffer[BUFFER_SIZE];
size_t buffer_offset = 0;

typedef struct {
    TSK_IMG_INFO *img_info;
    TSK_OFF_T start;
    TSK_DADDR_T length;
    int partition_number;
} partition_info_t;

void flush_buffer() {
    pthread_mutex_lock(&file_write_mutex);
    if (buffer_offset > 0) {
        fwrite(buffer, 1, buffer_offset, output_file);
        buffer_offset = 0;
    }
    pthread_mutex_unlock(&file_write_mutex);
}

void append_to_buffer(const char *str) {
    size_t len = strlen(str);
    if (buffer_offset + len >= BUFFER_SIZE) {
        flush_buffer();
    }
    memcpy(buffer + buffer_offset, str, len);
    buffer_offset += len;
}

void list_files_recursive(TSK_FS_INFO *fs_info, TSK_FS_DIR *fs_dir, const char *parent_path) {
    TSK_FS_FILE *fs_file;
    size_t dir_count = tsk_fs_dir_getsize(fs_dir);

    for (size_t i = 0; i < dir_count; i++) {
        fs_file = tsk_fs_dir_get(fs_dir, i);
        if (fs_file == NULL) {
            fprintf(stderr, "Error getting file from directory.\n");
            continue;
        }

        if (fs_file->name == NULL || fs_file->name->name == NULL) {
            fprintf(stderr, "Error: file name is NULL.\n");
            continue;
        }

        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s/%s", parent_path, fs_file->name->name);

        if (fs_file->name->type == TSK_FS_NAME_TYPE_DIR) {
            if (strcmp(fs_file->name->name, ".") != 0 && strcmp(fs_file->name->name, "..") != 0) {
                if (fs_file->meta != NULL) {
                    TSK_FS_DIR *sub_dir = tsk_fs_dir_open_meta(fs_info, fs_file->meta->addr);
                    if (sub_dir != NULL) {
                        list_files_recursive(fs_info, sub_dir, file_path);
                        tsk_fs_dir_close(sub_dir);
                    } else {
                        fprintf(stderr, "Error opening sub-directory: %s\n", file_path);
                    }
                } else {
                    fprintf(stderr, "Error: meta data is NULL for directory: %s\n", file_path);
                }
            }
        } else {
            printf("File: %s\n", file_path);
            char buffer_line[2048];
            snprintf(buffer_line, sizeof(buffer_line), "File: %s\n", file_path);
            append_to_buffer(buffer_line);

            pthread_mutex_lock(&file_count_mutex);
            file_count++;
            pthread_mutex_unlock(&file_count_mutex);
        }
    }
}

void *list_files_in_partition(void *arg) {
    partition_info_t *pinfo = (partition_info_t *)arg;
    TSK_FS_INFO *fs_info = tsk_fs_open_img(pinfo->img_info, pinfo->start * pinfo->length, TSK_FS_TYPE_DETECT);

    if (fs_info == NULL) {
        fprintf(stderr, "No valid filesystem in partition %d.\n", pinfo->partition_number);
        return NULL;
    }

    TSK_FS_DIR *fs_dir = tsk_fs_dir_open(fs_info, "/");
    if (fs_dir == NULL) {
        fprintf(stderr, "Error opening root directory in partition %d.\n", pinfo->partition_number);
        tsk_fs_close(fs_info);
        return NULL;
    }

    list_files_recursive(fs_info, fs_dir, "");
    tsk_fs_dir_close(fs_dir);
    tsk_fs_close(fs_info);

    return NULL;
}

void list_partitions(const char *filename) {
    libewf_handle_t *handle = NULL;
    int result = 0;
    libewf_error_t *error = NULL;
    TSK_IMG_INFO *img_info = NULL;
    TSK_VS_INFO *vs_info = NULL;

    // Initialize the libewf handle
    result = libewf_handle_initialize(&handle, NULL);
    if (result != 1) {
        fprintf(stderr, "Error initializing libewf handle.\n");
        return;
    }

    // Open the EWF file
    char *filenames[] = { (char *)filename, NULL };
    result = libewf_handle_open(handle, filenames, LIBEWF_OPEN_READ, 0, &error);
    if (result != 1) {
        fprintf(stderr, "Error opening EWF file: %s.\n", filename);
        if (error != NULL) {
            libewf_error_fprint(error, stderr);
            libewf_error_free(&error);
        }
        libewf_handle_free(&handle, NULL);
        return;
    }

    // Get the size of the image
    int64_t image_size = 0;
    result = libewf_handle_get_media_size(handle, &image_size, &error);
    if (result != 1) {
        fprintf(stderr, "Error getting media size.\n");
        if (error != NULL) {
            libewf_error_fprint(error, stderr);
            libewf_error_free(&error);
        }
        libewf_handle_close(handle, &error);
        libewf_handle_free(&handle, NULL);
        return;
    }

    // Create an img_info structure using libtsk
#ifdef _WIN32
    wchar_t tchar_filename[256];
    mbstowcs(tchar_filename, filename, strlen(filename) + 1);
#else
    char tchar_filename[256];
    strncpy(tchar_filename, filename, strlen(filename) + 1);
#endif
    img_info = tsk_img_open_sing(tchar_filename, TSK_IMG_TYPE_EWF_EWF, 0);
    if (img_info == NULL) {
        fprintf(stderr, "Error opening image with TSK.\n");
        libewf_handle_close(handle, &error);
        libewf_handle_free(&handle, NULL);
        return;
    }

    // Open the volume system
    vs_info = tsk_vs_open(img_info, 0, TSK_VS_TYPE_DETECT);
    if (vs_info == NULL) {
        fprintf(stderr, "Error opening volume system.\n");
        tsk_img_close(img_info);
        libewf_handle_close(handle, &error);
        libewf_handle_free(&handle, NULL);
        return;
    }

    // Create and launch threads for each partition
    pthread_t threads[vs_info->part_count];
    partition_info_t pinfo[vs_info->part_count];

    for (TSK_PNUM_T part_num = 0; part_num < vs_info->part_count; part_num++) {
        const TSK_VS_PART_INFO *part_info = tsk_vs_part_get(vs_info, part_num);
        if (part_info != NULL) {
            printf("Partition %d: Start: %" PRIuDADDR ", Length: %" PRIuDADDR "\n",
                   part_num, part_info->start, part_info->len);
            char buffer_line[2048];
            snprintf(buffer_line, sizeof(buffer_line), "Partition %d: Start: %" PRIuDADDR ", Length: %" PRIuDADDR "\n",
                    part_num, part_info->start, part_info->len);
            append_to_buffer(buffer_line);

            pinfo[part_num].img_info = img_info;
            pinfo[part_num].start = part_info->start;
            pinfo[part_num].length = vs_info->block_size;
            pinfo[part_num].partition_number = part_num;

            pthread_create(&threads[part_num], NULL, list_files_in_partition, &pinfo[part_num]);
        }
    }

    // Wait for all threads to finish
    for (TSK_PNUM_T part_num = 0; part_num < vs_info->part_count; part_num++) {
        pthread_join(threads[part_num], NULL);
    }

    // Clean up
    flush_buffer();
    tsk_vs_close(vs_info);
    tsk_img_close(img_info);
    libewf_handle_close(handle, &error);
    libewf_handle_free(&handle, NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename.e01>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Supprimer le fichier de sortie s'il existe
    remove("file_list.txt");

    clock_t start_time = clock();

    output_file = fopen("file_list.txt", "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return EXIT_FAILURE;
    }

    list_partitions(argv[1]);

    fclose(output_file);

    clock_t end_time = clock();
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Total files found: %d\n", file_count);
    printf("Time taken: %.2f seconds\n", time_taken);

    return EXIT_SUCCESS;
}
