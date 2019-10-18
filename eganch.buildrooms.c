/*
 * PROGRAM NAME: eganch.buildrooms.c
 * DESCRIPTION: This program generates files that will be used by the
 * eganch.adventure program. The files represent seven "rooms" and hold
 * information about their name, rooms they are connected to, and the
 * start and end rooms.
 * AUTHOR: Chelsea Egan (eganch@oregonstate.edu)
 */

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 4096
#define MIN_NUM_CONNECTIONS 3
#define NUM_CONNECTIONS 6
#define NUM_ROOMS 7
#define NUM_ROOM_NAMES 10
#define TRUE 0
#define FALSE 1

enum roomTypes {START_ROOM, END_ROOM, MID_ROOM};
char* directoryName;

struct Room {
    char* roomName;
    enum roomTypes type;
    int connections[NUM_CONNECTIONS];
    int index;
    int numConnections;
};

/*
 * NAME: makeDirectory
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Creates directory to hold room files
 */
void makeDirectory() {
    int pid = getpid();
    char* mypid = malloc(21);

    sprintf(mypid, "%d", pid);
    directoryName = malloc(35);
    strcpy(directoryName, "eganch.rooms.");
    strcat(directoryName, mypid);

    int result = mkdir(directoryName, 0755);
    assert(result == TRUE);

    free(mypid);
}

/*
 * NAME: randomizeFileNames
 * PARAMS: none
 * RETURN: Array of pointers to strings holding room names
 * DESCRIPTION: Creates an array filled with potential room names and
 * randomizes the order
 */
char** randomizeFileNames() {
    const int NAME_LENGTH = 9;
    char** roomNames = malloc(NUM_ROOM_NAMES * sizeof(char*));

    int i;
    for (i = 0; i < NUM_ROOM_NAMES; i++) {
        roomNames[i] = malloc(NAME_LENGTH * sizeof(char));
    }

    strncpy(roomNames[0], "Reptiles", NAME_LENGTH);
    strncpy(roomNames[1], "Cats", NAME_LENGTH);
    strncpy(roomNames[2], "Dogs", NAME_LENGTH);
    strncpy(roomNames[3], "Monkeys", NAME_LENGTH);
    strncpy(roomNames[4], "Birds", NAME_LENGTH);
    strncpy(roomNames[5], "Insects", NAME_LENGTH);
    strncpy(roomNames[6], "Rodents", NAME_LENGTH);
    strncpy(roomNames[7], "Turtles", NAME_LENGTH);
    strncpy(roomNames[8], "Octopi", NAME_LENGTH);
    strncpy(roomNames[9], "Humans", NAME_LENGTH);

    for (i = 0; i < NUM_ROOM_NAMES - 1; i++) {
        int c = rand() / (RAND_MAX/(NUM_ROOM_NAMES - i) + 1);
        char* t = roomNames[i];
        roomNames[i] = roomNames[i + c];
        roomNames[i + c] = t;
    }

    return roomNames;
}

/*
 * NAME: setRoomInfo
 * PARAMS: Array of pointers to strings holding room names
 * RETURN: Array of structs containing info about the rooms
 * DESCRIPTION: Creates an array filled with structs holding room
 * information.
 */
struct Room* setRoomInfo(char** fileNames) {
    int endRoomIndex,
        isDuplicate = TRUE,
        startRoomIndex;

    struct Room* rooms = malloc(NUM_ROOMS * sizeof(struct Room));

    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        rooms[i].roomName = fileNames[i];
        rooms[i].type = MID_ROOM;
        rooms[i].index = i;
        rooms[i].numConnections = 0;

        int j;
        for (j = 0; j < NUM_CONNECTIONS; j++) {
            rooms[i].connections[j] = -1;
        }
    }

    startRoomIndex = rand() % NUM_ROOMS;

    while (isDuplicate == TRUE) {
        endRoomIndex = rand() % NUM_ROOMS;
        if (endRoomIndex != startRoomIndex) {
            isDuplicate = FALSE;
        }
    }

    rooms[startRoomIndex].type = START_ROOM;
    rooms[endRoomIndex].type = END_ROOM;

    return rooms;
}

/*
 * NAME: createFilePath
 * PARAMS: Pointer to a filename and pointer to a file path
 * RETURN: void
 * DESCRIPTION: Constructs the file path
 */
void createFilePath(char* fileName, char* pathFile) {
    memset(pathFile, '\0', MAX_PATH_LENGTH);
    strcpy(pathFile, directoryName);
    strcat(pathFile, "/");
    strcat(pathFile, fileName);
}

/*
 * NAME: createFiles
 * PARAMS: Pointer to an array of filenames
 * RETURN: void
 * DESCRIPTION: For each room, creates a file in the room directory
 */
void createFiles(char** fileNames) {
    char* pathFile = malloc(MAX_PATH_LENGTH * sizeof(char));

    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        createFilePath(fileNames[i], pathFile);
        int fileDescriptor = open(pathFile, O_CREAT, 0644);
        close(fileDescriptor);
    }

    free(pathFile);
}

/*
 * NAME: getNumConnectedRooms
 * PARAMS: Pointer to the array of room structs
 * RETURN: Number of rooms with 3+ connections
 * DESCRIPTION: Returns the number of rooms that have 3-6 outbound connections
 */
int getNumConnectedRooms(struct Room *rooms) {
    int i,
        connected = 0;

    for (i = 0; i < NUM_ROOMS; i++) {
        if (rooms[i].numConnections >= MIN_NUM_CONNECTIONS) {
            connected++;
        }
    }
    return connected;
}

/*
 * NAME: getRandomRoom
 * PARAMS: Pointer to an array of room structs
 * RETURN: Pointer to a single room struct
 * DESCRIPTION: Returns a random Room, does NOT validate if connection can be added
 */
struct Room* getRandomRoom(struct Room *rooms) {
    int roomIndex = rand() % NUM_ROOMS;
    return &rooms[roomIndex];
}

/*
 * NAME: connectionAlreadyExists
 * PARAMS: Two pointers to room structs
 * RETURN: Int indicating connection
 * DESCRIPTION: Returns 0 if a connection from Room x to Room y already exists, 1 otherwise
 */
int connectionAlreadyExists(struct Room *x, struct Room *y) {
    int i;
    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (x->connections[i] == y->index) {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * NAME: connectRoom
 * PARAMS: Two pointers to room structs
 * RETURN: void
 * DESCRIPTION: Connects Rooms x and y together, does not check if this connection is valid
 */
void connectRoom(struct Room* x, struct Room* y) {
    int i,
        xConnected = FALSE,
        yConnected = FALSE;

    for (i = 0; i < NUM_CONNECTIONS; i++) {
        if (xConnected == FALSE && x->connections[i] == -1) {
            xConnected = TRUE;
            x->connections[i] = y->index;
            x->numConnections++;
        }
        if (yConnected == FALSE && y->connections[i] == -1) {
            yConnected = TRUE;
            y->connections[i] = x->index;
            y->numConnections++;
        }

        if (xConnected == TRUE && yConnected == TRUE) {
            break;
        }
    }

    assert(xConnected == TRUE && yConnected == TRUE);
}

/*
 * NAME: isSameRoom
 * PARAMS: Two pointers to room structs
 * RETURN: Int indicating if they are the same room
 * DESCRIPTION: Returns 0 if Rooms x and y are the same Room, 1 otherwise
 */
int isSameRoom(struct Room* x, struct Room* y) {
    return (x->roomName == y->roomName) ? TRUE : FALSE;
}

/*
 * NAME: addRandomConnection
 * PARAMS: Pointer to array of room structs
 * RETURN: void
 * DESCRIPTION: Adds a random, valid outbound connection from a Room to another Room
 */
void addRandomConnection(struct Room* rooms) {
    struct Room* roomA;
    struct Room* roomB;

    while (FALSE) {
        roomA = getRandomRoom(rooms);

        if (roomA->numConnections < NUM_CONNECTIONS) {
            break;
        }
    }

    do {
        roomB = getRandomRoom(rooms);
    } while(roomB->numConnections >= NUM_CONNECTIONS
            || isSameRoom(roomA, roomB) == TRUE
            || connectionAlreadyExists(roomA, roomB) == TRUE);

    connectRoom(roomA, roomB);
}

/*
 * NAME: storeInfoToFiles
 * PARAMS: Pointer to array of room structs
 * RETURN: void
 * DESCRIPTION: Print the info from the room structs into the files
] */
void storeInfoToFiles(struct Room* rooms) {
    const char * roomTypesLabel[] = {"START_ROOM", "END_ROOM", "MID_ROOM"};
    char* pathFile = malloc(MAX_PATH_LENGTH * sizeof(char));
    FILE* filePtr;

    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        createFilePath(rooms[i].roomName, pathFile);

        filePtr = fopen(pathFile, "w");
        assert(filePtr != NULL);

        fprintf(filePtr, "ROOM NAME: %s\n", rooms[i].roomName);

        int j;
        for (j = 0; j < rooms[i].numConnections; j++) {
            fprintf(filePtr, "CONNECTION %d: %s\n", j + 1, rooms[rooms[i].connections[j]].roomName);
        }

        fprintf(filePtr, "ROOM TYPE: %s\n", roomTypesLabel[rooms[i].type]);

        fclose(filePtr);
    }

    free(pathFile);
}

int main() {
    time_t t;
    srand((unsigned) time(&t));

    makeDirectory();

    char** fileNames = randomizeFileNames();

    createFiles(fileNames);

    struct Room* rooms = setRoomInfo(fileNames);

    while (getNumConnectedRooms(rooms) != NUM_ROOMS) {
        addRandomConnection(rooms);
    }

    storeInfoToFiles(rooms);

    int i;
    for (i = 0; i < NUM_ROOM_NAMES; i++) {
        free(fileNames[i]);
    }
    free(fileNames);
    free(rooms);
    free(directoryName);

    return 0;
}