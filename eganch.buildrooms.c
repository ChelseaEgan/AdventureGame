/*
 * CLASS: CS 344-400
 * ASSIGNMENT: Program 2
 * PROGRAM NAME: eganch.buildrooms.c
 * DESCRIPTION: This program generates files that will be used by the
 * eganch.adventure program. The files represent seven "rooms" and hold
 * information about their name, rooms they are connected to, and the
 * start and end rooms.
 * AUTHOR: Chelsea Egan (eganch@oregonstate.edu)
 * SOURCE FOR GENERATING CONNECTIONS: https://oregonstate.instructure.com/courses/1729341/pages/2-dot-2-program-outlining-in-program-2
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
 * SOURCE: https://stackoverflow.com/a/53230284
 */
void makeDirectory() {
    int pid = getpid();
    char* mypid = malloc(21);

    /* Get process ID and append to directory name */
    sprintf(mypid, "%d", pid);
    directoryName = malloc(35);
    strcpy(directoryName, "eganch.rooms.");
    strcat(directoryName, mypid);

    /* Create directory with permissions */
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
 * SOURCE: http://c-faq.com/lib/shuffle.html
 */
char** randomizeFileNames() {
    const int NAME_LENGTH = 9;
    char** roomNames = malloc(NUM_ROOM_NAMES * sizeof(char*));

    /* Allocate memory for roomNames array */
    int i;
    for (i = 0; i < NUM_ROOM_NAMES; i++) {
        roomNames[i] = malloc(NAME_LENGTH * sizeof(char));
    }

    /* Fill array with possible room names */
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

    /* Randomize order of array so that can select first seven and get random room names*/
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

    /* Allocate memory for 7 room structs */
    struct Room* rooms = malloc(NUM_ROOMS * sizeof(struct Room));

    /* Set each struct with a random file name, as a MID_ROOM, an
     * index, and initialize its number of connections to 0. Then
     * fill each connection in the array with -1 to indicate its empty */
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

    /* Choose a random room to be the start room */
    startRoomIndex = rand() % NUM_ROOMS;

    /* Choose a random room that does not match the start room to be
     * the end room */
    while (isDuplicate == TRUE) {
        endRoomIndex = rand() % NUM_ROOMS;
        if (endRoomIndex != startRoomIndex) {
            isDuplicate = FALSE;
        }
    }

    /* Update the room types for start and end rooms */
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
    /* Allocate memory for the path file string */
    char* pathFile = malloc(MAX_PATH_LENGTH * sizeof(char));

    /* Create the files for the 7 rooms */
    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        createFilePath(fileNames[i], pathFile);
        int fileDescriptor = open(pathFile, O_CREAT, 0644);
        /* Close for now until needed later */
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

    /* Loop through the rooms and get their number of connections */
    for (i = 0; i < NUM_ROOMS; i++) {
        /* Increment connected counter if it has at least 3 connections */
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
    /* Get random room index and return the corresponding room */
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
        /* Checks if the index stored in the x room connection matches room y */
        if (x->connections[i] == y->index) {
            return TRUE;
        }
    }

    /* No connection was found */
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

    /* Loops through the 6 elements in each room's connections array to find an "empty slot" */
    for (i = 0; i < NUM_CONNECTIONS; i++) {
        /* If room x has not been connected and this connection has not been set */
        if (xConnected == FALSE && x->connections[i] == -1) {
            /* Indicate that room x is now connected */
            xConnected = TRUE;
            /* Set the element in the connections array with the index of room y */
            x->connections[i] = y->index;
            /* Increment its number of connections */
            x->numConnections++;
        }
        /* If room y has not been connected and this connection has not been set */
        if (yConnected == FALSE && y->connections[i] == -1) {
            /* Indicate that room y is now connected */
            yConnected = TRUE;
            /* Set the element in the connections array with the index of room x */
            y->connections[i] = x->index;
            /* Increment its number of connections */
            y->numConnections++;
        }

        /* If both rooms are connected to each other, stop looping */
        if (xConnected == TRUE && yConnected == TRUE) {
            break;
        }
    }

    /* If either room failed to connect there has been a failure */
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

    /* Loop until a room with less than 6 connections is found */
    while (FALSE) {
        roomA = getRandomRoom(rooms);

        if (roomA->numConnections < NUM_CONNECTIONS) {
            break;
        }
    }

    /* Loop until a second room is found that has less than 6 connections, is not
     * the same room as roomA, and is not already connected to roomA */
    do {
        roomB = getRandomRoom(rooms);
    } while(roomB->numConnections >= NUM_CONNECTIONS
            || isSameRoom(roomA, roomB) == TRUE
            || connectionAlreadyExists(roomA, roomB) == TRUE);

    /* Make the connection */
    connectRoom(roomA, roomB);
}

/*
 * NAME: storeInfoToFiles
 * PARAMS: Pointer to array of room structs
 * RETURN: void
 * DESCRIPTION: Print the info from the room structs into the files
 * SOURCES: https://www.programiz.com/c-programming/c-file-input-output
 * https://oregonstate.instructure.com/courses/1729341/pages/2-dot-4-manipulating-directories
 */
void storeInfoToFiles(struct Room* rooms) {
    /* Labels to correspond to the room types enum */
    const char * roomTypesLabel[] = {"START_ROOM", "END_ROOM", "MID_ROOM"};
    /* Allocate memory for the file path */
    char* pathFile = malloc(MAX_PATH_LENGTH * sizeof(char));
    FILE* filePtr;

    /* Loop through all 7 rooms */
    int i;
    for (i = 0; i < NUM_ROOMS; i++) {
        /* Get the file path based on the name of the room and the room directory */
        createFilePath(rooms[i].roomName, pathFile);

        /* Open the file for writing */
        filePtr = fopen(pathFile, "w");
        assert(filePtr != NULL);

        /* Insert the room name into the file */
        fprintf(filePtr, "ROOM NAME: %s\n", rooms[i].roomName);

        /* Insert all of the room's connections into the file */
        int j;
        for (j = 0; j < rooms[i].numConnections; j++) {
            fprintf(filePtr, "CONNECTION %d: %s\n", j + 1, rooms[rooms[i].connections[j]].roomName);
        }

        /* Insert the room type into the file */
        fprintf(filePtr, "ROOM TYPE: %s\n", roomTypesLabel[rooms[i].type]);

        fclose(filePtr);
    }

    free(pathFile);
}

int main() {
    time_t t;
    /* Seed the rand function */
    srand((unsigned) time(&t));

    /* Create a directory in which the files will be stored */
    makeDirectory();

    /* Craete the room names that will be used as file names */
    char** fileNames = randomizeFileNames();

    /* Generate the files */
    createFiles(fileNames);

    /* Generate the room info including name, connections, and type */
    struct Room* rooms = setRoomInfo(fileNames);

    /* Create all connections in graph */
    while (getNumConnectedRooms(rooms) != NUM_ROOMS) {
        addRandomConnection(rooms);
    }

    /* Write the room information to the files */
    storeInfoToFiles(rooms);

    /* Free all allocated memory */
    int i;
    for (i = 0; i < NUM_ROOM_NAMES; i++) {
        free(fileNames[i]);
    }
    free(fileNames);
    free(rooms);
    free(directoryName);

    return 0;
}