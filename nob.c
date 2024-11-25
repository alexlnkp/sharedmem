/* cc -o nob nob.c && ./nob */
#define NOB_IMPLEMENTATION
#include "nob.h"

#define ARR_LEN(x) (sizeof((x)) / sizeof((x)[0]))

#define CROSSPLATFORM_FOLDER_NAME "crossplatform"

#define TEST_FOLDER "test"
#define OUT_FOLDER "out"

#define WINDOWS_C_COMPILER "x86_64-w64-mingw32-gcc"
#define LINUX_C_COMPILER "gcc"

#define WINDOWS_CPP_COMPILER "x86_64-w64-mingw32-g++"
#define LINUX_CPP_COMPILER "g++"

const char* ctoplat[] = {
    WINDOWS_C_COMPILER,
    WINDOWS_CPP_COMPILER,
    LINUX_C_COMPILER,
    LINUX_CPP_COMPILER,
};

const char* platforms[] = {"win", "linux"};

/* cpp being labeled "xx" is a way for bash autocomplections to
 * not be confused too much. yes, it looks weird. is it faster
 * than typing out cpp or c++ or anything like that? - yes. */
const char* implementations[] = {"c", "xx"};
const char* standards[] = {"-std=c89", "-std=c++98"};

Nob_String_View current_compiler;

void get_basename_of_file(char * restrict filename, const char* restrict input) {
    const char *last_slash;
    const char *dot;

    last_slash = strrchr(input, '/');
    last_slash = last_slash + (last_slash != NULL);

    dot = strchr(last_slash, '.');
    if (dot != NULL) {
        strncpy(filename, last_slash, dot - last_slash);
        filename[dot - last_slash] = '\0';
    } else {
        strcpy(filename, last_slash);
    }
}

void compile_source_files(const char* parent_path, const char* out_path, const char* impl, const char* standard) {
    Nob_File_Paths fp = {0};
    int i = 0;
    nob_read_entire_dir(parent_path, &fp);

    for (i = 0; i < fp.count; ++i) {
        Nob_String_View input = nob_sv_from_cstr(nob_temp_sprintf("%s/%s", parent_path, fp.items[i]));

        if (nob_sv_end_with(input, nob_temp_sprintf(".%s", strcmp(impl, "xx") == 0 ? "cpp" : impl))) {
            char filename[32];
            get_basename_of_file(filename, nob_temp_sv_to_cstr(input));

            Nob_Cmd cmd = {0};
            nob_cmd_append(&cmd,
                nob_temp_sv_to_cstr(current_compiler),
                standard,
                nob_temp_sv_to_cstr(input),
                "-o", nob_temp_sprintf("%s/%s", out_path, filename),
                "-Iinclude"
            );
            nob_cmd_run_sync(cmd);
        }
    }
}

void compile_all_tests(void) {
    int i = 0; int j = 0;
    const char* current_impl;
    const char* current_plat;

    /* this is ugly but i have no other idea how to do this with nob */
    nob_mkdir_if_not_exists(OUT_FOLDER);

    for (i = 0; i < ARR_LEN(platforms); ++i) {
        current_plat = platforms[i];
        nob_mkdir_if_not_exists(nob_temp_sprintf("%s/%s", OUT_FOLDER, current_plat));

        for (j = 0; j < ARR_LEN(implementations); ++j) {
            current_impl = implementations[j];
            nob_mkdir_if_not_exists(
                nob_temp_sprintf("%s/%s/%s", OUT_FOLDER, current_plat, current_impl)
            );

            /* i'm kind of proud on how i figured out a way to make this work :) */
            current_compiler = nob_sv_from_cstr(ctoplat[i*2 + j]);

            compile_source_files(
                nob_temp_sprintf("%s/%s/%s", TEST_FOLDER, CROSSPLATFORM_FOLDER_NAME, current_impl),
                nob_temp_sprintf("%s/%s/%s", OUT_FOLDER, current_plat, current_impl),
                current_impl,
                standards[j]
            );
        }
    }
}

int main(int argc, char* argv[]) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    compile_all_tests();
}
