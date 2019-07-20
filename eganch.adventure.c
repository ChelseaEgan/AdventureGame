#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NAME_LENGTH 9
#define NUM_CONNECTIONS 6
#define NUM_ROOMS 7
#define TRUE 0
#define FALSE 1

int startRoomIndex = -1;
int endRoomIndex = -1;

struct Room* rooms;
struct Room {
    char roomName[12];
    char roomType[11];
    char* connections[NUM_CONNECTIONS];
    int index;
    int numConnections;
};

struct UserPath path;
struct UserPath {
    int* path;
    int pathSize;
    int pathUsed;
};

/*
 * https://stackoverflow.com/a/3536261
 */
void initPath(){
    path.path = (int *)malloc(10 * sizeof(int));
    path.pathUsed = 0;
    path.pathSize = 10;
}

/*
 * https://stackoverflow.com/a/3536261
 */
void addToPath(int roomIndex) {
    if (path.pathUsed == path.pathSize) {
        path.pathSize *= 2;
        path.path = (int *)realloc(path.path, path.pathSize * sizeof(int));
    }

    path.path[path.pathUsed++] = roomIndex;
}

/*
 * https://oregonstate.instructure.com/courses/1729341/pages/2-dot-4-manipulating-directories
 */
void getRoomsDirectory() {
    DIR* rootDir;
    char targetDirPrefix[32] = "eganch.rooms.";
    char newestDirName[256];
    struct dirent *fileInDir;
    struct stat dirAttributes;
    int newestDirTime = -1;

    memset(newestDirName, '\0', sizeof(newestDirName));

    rootDir = opendir(".");

    if (rootDir <= 0) {
        printf("ERROR: Could not open root directory. Exiting.\n");
        exit(1);
    }

    while ((fileInDir = readdir(rootDir)) != NULL) {
        if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) {
            stat(fileInDir->d_name, &dirAttributes);
            if ((int)dirAttributes.st_mtime > newestDirTime) {
                newestDirTime = (int) dirAttributes.st_mtime;
                memset(newestDirName, '\0', sizeof(newestDirName));
                strcpy(newestDirName, fileInDir->d_name);
            }
        }
    }

    if (chdir(newestDirName) != TRUE) {
        printf("ERROR: Failed to change directories. Exiting. \n");
        exit(1);
    }

    closedir(rootDir);
}

/*
 * https://www.tutorialspoint.com/cprogramming/c_file_io
 * https://stackoverflow.com/a/11737506
 */
struct Room* getRoomInfo() {
    DIR* roomsDir;
    FILE* filePtr;
    struct dirent* fileInDir;
    char buffer[255];
    int roomIndex = 0;

    rooms = malloc(NUM_ROOMS * sizeof(struct Room));

    roomsDir = opendir(".");

    if (roomsDir == NULL) {
        printf("ERROR: Could not open root directory. Exiting.\n");
        exit(1);
    }

    while ((fileInDir = readdir(roomsDir))) {
        if (!strcmp(fileInDir->d_name, ".")) {
            continue;
        }
        if (!strcmp(fileInDir->d_name, "..")) {
            continue;
        }

        memset(buffer, '\0', 255);

        filePtr = fopen(fileInDir->d_name, "r");
        if (filePtr == NULL) {
            printf("ERROR: Failed to open file %s. Exiting. \n", fileInDir->d_name);
            exit(1);
        }

        memset(rooms[roomIndex].roomName, '\0', 12);
        memset(rooms[roomIndex].roomType, '\0', 11);
        rooms[roomIndex].numConnections = 0;
        rooms[roomIndex].index = roomIndex;

        if (fgets(buffer, 255, filePtr) == NULL) {
            printf("ERROR: Failed to read file %s. Exiting. \n");
            exit(1);
        }
        int length = strcspn(buffer, "\n");
        strncpy(rooms[roomIndex].roomName, buffer + 11, length - 11);

        int connectionIndex = 0;
        while (strstr(fgets(buffer, 255, (FILE*)filePtr), "CONNECTION") != NULL) {
            rooms[roomIndex].connections[connectionIndex] = malloc(12 * sizeof(char));
            memset(rooms[roomIndex].connections[connectionIndex], '\0', 12);
            length = strcspn(buffer, "\n");
            strncpy(rooms[roomIndex].connections[connectionIndex], buffer + 14, length - 14);
            rooms[roomIndex].numConnections++;
            connectionIndex++;
        }

        length = strcspn(buffer, "\n");
        strncpy(rooms[roomIndex].roomType, buffer + 11, length - 11);

        if (strcmp(rooms[roomIndex].roomType, "START_ROOM") == 0) {
            startRoomIndex = roomIndex;
        } else if (strcmp(rooms[roomIndex].roomType, "END_ROOM") == 0) {
            endRoomIndex = roomIndex;
        }

        fclose(filePtr);
        roomIndex++;
    }

    closedir(roomsDir);
    chdir("..");

    return rooms;
}

void printRoomInfo(struct Room* currentRoom) {
    printf("\nCURRENT LOCATION: %s\n", currentRoom->roomName);

    printf("POSSIBLE CONNECTIONS: ");
    int i;
    for (i = 0; i < currentRoom->numConnections; i++) {
        if (i == currentRoom->numConnections - 1) {
            printf("%s.\n", currentRoom->connections[i]);
        } else {
            printf("%s, ", currentRoom->connections[i]);
        }
    }

    printf("WHERE TO? >");
}

int checkRoomIsValid(char* roomName) {
    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        if (strcmp(rooms[i].roomName, roomName) == TRUE) {
            return i;
        }
    }

    return -1;
}

int getUserInput() {
    int roomIndex = -1;
    char buffer[256];
    char roomChoice[256];
    memset(buffer, '\0', sizeof(buffer));
    memset(roomChoice, '\0', sizeof(roomChoice));

    if (fgets(buffer, 256, stdin) == NULL) {
        printf("ERROR: Failed to read user input. Exiting. \n");
        exit(1);
    }

    int length = strcspn(buffer, "\n");
    strncpy(roomChoice, buffer, length);

    roomIndex = checkRoomIsValid(roomChoice);

    if (roomIndex == -1) {
        printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
    }

    return roomIndex;
}

void printFinalStats() {
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", path.pathUsed);
    int i;
    for (i = 0; i < path.pathUsed; i++) {
        printf("%s\n", rooms[path.path[i]].roomName);
    }
}

void runRoomProgram() {
    int currentRoomIndex = startRoomIndex;
    int requestedRoomIndex = -1;

    initPath();

    while (currentRoomIndex != endRoomIndex) {
        do {
            printRoomInfo(&rooms[currentRoomIndex]);
            requestedRoomIndex = getUserInput();
        } while (requestedRoomIndex == -1);

        currentRoomIndex = requestedRoomIndex;
        addToPath(currentRoomIndex);
    }

    printf("\nYOU'VE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printFinalStats();
}

int main() {
    getRoomsDirectory();
    getRoomInfo();

/*    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        printf("ROOM NAME: %s\n", rooms[i].roomName);
        printf("ROOM TYPE: %s\n", rooms[i].roomType);
        printf("ROOM INDEX: %d\n", rooms[i].index);
        printf("# CONNECTIONS: %d\n", rooms[i].numConnections);

        int j;
        for (j = 0; j < rooms[i].numConnections; j++) {
            printf("CONNECTION %d: %s\n", j + 1, rooms[i].connections[j]);
        }
    }
*/

    runRoomProgram();

    int i, j;
    for (i = 0; i < NUM_ROOMS; i++) {
        for (j = 0; j < rooms[i].numConnections; j++) {
            free(rooms[i].connections[j]);
        }
    }
    free(rooms);
    free(path.path);
    return 0;
}