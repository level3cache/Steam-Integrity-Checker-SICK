#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

char ** gameIndexed;
size_t gameIndexedSize = 0;
size_t gameIndexedCapacity = 16;

char ** manifestIndexed;
size_t manifestIndexedSize = 0;
size_t manifestIndexedCapacity = 16;

void addGameFile(char * gameFile) {
    for (int i = 0; i < strlen(gameFile); i++) {
        if (gameFile[i] == '/') {
            gameFile[i] = '\\';
        }
    }
    if (gameIndexed == NULL) {
        gameIndexed = (char **) malloc(sizeof(char *) * gameIndexedCapacity);
    } else if (gameIndexedSize >= gameIndexedCapacity) {
        gameIndexedCapacity *= 2;
        gameIndexed = (char **) realloc(gameIndexed, sizeof(char *) * gameIndexedCapacity);
    }

    gameIndexed[gameIndexedSize++] = strdup(gameFile);
}

void indexGameFiles(const char *basePath, const char *prefix) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(basePath)) == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char fullPath[PATH_MAX];
        char relativePath[PATH_MAX];

        snprintf(fullPath, sizeof(fullPath), "%s/%s", basePath, entry->d_name);
        if (prefix[0] == '\0')
            snprintf(relativePath, sizeof(relativePath), "%s", entry->d_name);
        else
            snprintf(relativePath, sizeof(relativePath), "%s/%s", prefix, entry->d_name);

        addGameFile(relativePath); // Match "Name" column

        struct stat pathStat;
        if (stat(fullPath, &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
            indexGameFiles(fullPath, relativePath); // Recurse into subdirectory
        }
    }

    closedir(dir);
}

void addManifestFile(const char * manifestFile) {
    if (manifestIndexed == NULL) {
        manifestIndexed = (char **) malloc(sizeof(char *) * manifestIndexedCapacity);
    } else if (manifestIndexedSize >= manifestIndexedCapacity) {
        manifestIndexedCapacity *= 2;
        manifestIndexed = (char **) realloc(manifestIndexed, sizeof(char *) * manifestIndexedCapacity);
    }
    manifestIndexed[manifestIndexedSize++] = strdup(manifestFile);
}

void indexManifest(const char *manifestPath) {
    FILE *manifestFile = fopen(manifestPath, "r");

    char* buffer = malloc(sizeof(char) * 1024);
    for (int i = 0; i < 10; i++) {
        fgets(buffer, sizeof(char) * 1024, manifestFile);
    }

    while (fgets(buffer, sizeof(char) * 1024, manifestFile) != NULL) {
        char* nameColumn = buffer + 69;
        if (nameColumn[strlen(nameColumn) - 1] == '\n') {
            nameColumn[strlen(nameColumn) - 1] = '\0';
        }
        addManifestFile(nameColumn);
    }
    fclose(manifestFile);
    free(buffer);
}

int compare(const void *a, const void *b) {
    return strcmp(*(char **)a, *(char **)b);
}

void checkFileIntegrity() {
    int gameIndexedCount = 0;
    int manifestIndexedCount = 0;

    printf("Game Indexed: %d\n", (int) gameIndexedSize);
    printf("Manifest Indexed: %d\n", (int) manifestIndexedSize);

    while (gameIndexedCount < gameIndexedSize || manifestIndexedCount < manifestIndexedSize) {
        if (gameIndexedCount < gameIndexedSize && manifestIndexedCount < manifestIndexedSize) {
            int result = compare(&gameIndexed[gameIndexedCount], &manifestIndexed[manifestIndexedCount]);
            if (result == 0) {
                gameIndexedCount++;
                manifestIndexedCount++;
            } else if (result > 0) {
                    printf("Missing File: %s\n", manifestIndexed[manifestIndexedCount]);
                    manifestIndexedCount++;
                } else if (result < 0) {
                    printf("Extra File: %s\n", gameIndexed[gameIndexedCount]);
                    gameIndexedCount++;
                }
        } else if (gameIndexedCount < gameIndexedSize && manifestIndexedCount == manifestIndexedSize) {
            printf("Extra File: %s\n", gameIndexed[gameIndexedCount]);
            gameIndexedCount++;
        } else if (gameIndexedCount == gameIndexedSize && manifestIndexedCount < manifestIndexedSize) {
            printf("Missing File: %s\n", manifestIndexed[manifestIndexedCount]);
            manifestIndexedCount++;
        }
    }
}

int main (const int argc, char *argv[]) {
    if (argc == 1) {
        printf("no arguments provided, try to run \".\\SICK.exe help\" \n");
    }else if (argc == 2 && strcmp(argv[1], "help") == 0) {
        printf("Provide absolute path to game, followed by one or multiple total/relative paths to manifest files. \n");
        printf("For example:\n");
        printf(".\\SICK.exe \"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Helldivers 2\" \".\\depots\\553851\\19232456\\manifest_553851_4333239040659935267.txt\" \".\\depots\\553853...\" ...\n");
    } else if (argc == 2) {
        printf("not enough arguments provided, run \"SICK.exe help\" for information on correct usage\n");
    }
    else if (argc > 2) {
        indexGameFiles(argv[1], "");
        for (int i = 1; i < argc; i++) {
            indexManifest(argv[i]);
        }
        qsort(gameIndexed, gameIndexedSize, sizeof(char *), compare);
        qsort(manifestIndexed, manifestIndexedSize, sizeof(char *), compare);

        checkFileIntegrity();
    }

    for (int i = 0; i < gameIndexedSize; i++) {
        free(gameIndexed[i]);
    }
    free(gameIndexed);
    for (int i = 0; i < manifestIndexedSize; i++) {
        free(manifestIndexed[i]);
    }
    free(manifestIndexed);

    return (EXIT_SUCCESS);
}