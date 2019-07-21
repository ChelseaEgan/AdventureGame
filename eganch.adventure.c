#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define NAME_LENGTH 9
#define NUM_CONNECTIONS 6
#define NUM_ROOMS 7
#define TRUE 0
#define FALSE 1

int startRoomIndex = -1;
int endRoomIndex = -1;

pthread_mutex_t lock;
pthread_t threadID;

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

void printRoomInfo(int roomIndex) {
    if (roomIndex != 10) {
        printf("\nCURRENT LOCATION: %s\n", rooms[roomIndex].roomName);

        printf("POSSIBLE CONNECTIONS: ");
        int i;
        for (i = 0; i < rooms[roomIndex].numConnections; i++) {
            if (i == rooms[roomIndex].numConnections - 1) {
                printf("%s.\n", rooms[roomIndex].connections[i]);
            } else {
                printf("%s, ", rooms[roomIndex].connections[i]);
            }
        }
    }

    printf("WHERE TO? >");
}

int checkRoomIsValid(char* roomName) {
    if (strcmp("time", roomName) == TRUE) {
        return 10;
    }

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

/*
 * https://linux.die.net/man/3/strftime
 */
void* getCurrentTime() {
    char timeString[256];
    char* timeFormat = "%n %l:%M%P, %A, %B %e, %Y%n%n";
    time_t currentTime;

    pthread_mutex_lock(&lock);

    FILE* filePtr;
    filePtr = fopen("currentTime.txt", "w");

    if (filePtr == NULL) {
        printf("\nERROR: Failed to create file. Exiting.\n");
        exit(1);
    }

    currentTime = time(NULL);

    if (strftime(timeString, sizeof(timeString), timeFormat, localtime(&currentTime)) == 0) {
        printf("\nERROR: Failed to get current time. Exiting.\n");
        exit(1);
    }

    printf(timeString);
    fprintf(filePtr, timeString);

    fclose(filePtr);

    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

/*
 * https://www.thegeekstuff.com/2012/05/c-mutex-examples/?refcom
 */
void createTimeThread() {
    int result;

    pthread_mutex_lock(&lock);

    result = pthread_create(&threadID, NULL, &getCurrentTime, NULL);
    if (result != TRUE) {
        printf("\nERROR: Failed to create thread. Exiting. \n");
        exit(1);
    }
}

void runRoomProgram() {
    int currentRoomIndex = startRoomIndex;
    int requestedRoomIndex = -1;

    initPath();

    while (currentRoomIndex != endRoomIndex) {
        do {
            printRoomInfo(currentRoomIndex);
            requestedRoomIndex = getUserInput();
        } while (requestedRoomIndex == -1);

        currentRoomIndex = requestedRoomIndex;

        if (requestedRoomIndex == 10) {
            createTimeThread();
            pthread_mutex_unlock(&lock);
            pthread_join(threadID, NULL);
        } else {
            addToPath(currentRoomIndex);
        }
    }

    printf("\nYOU'VE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printFinalStats();
}

int main() {
    if (pthread_mutex_init(&lock, NULL) != TRUE) {
        printf("\nERROR: Failed to create mutex. Exiting.\n");
        exit(1);
    }

    getRoomsDirectory();
    getRoomInfo();
    runRoomProgram();

    int i, j;
    for (i = 0; i < NUM_ROOMS; i++) {
        for (j = 0; j < rooms[i].numConnections; j++) {
            free(rooms[i].connections[j]);
        }
    }
    free(rooms);
    free(path.path);
    pthread_mutex_destroy(&lock);
    return 0;
}