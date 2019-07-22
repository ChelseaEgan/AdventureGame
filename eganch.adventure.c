/*
 * CLASS: CS 344-400
 * ASSIGNMENT: Program 2
 * PROGRAM NAME: eganch.adventure.c
 * DESCRIPTION: This program uses the files generated by the eganch.buildrooms program.
 * It provides an interface for the user to navigate through the rooms from the start
 * room until they reach the end room at which point they "win." The user can also
 * request and receive the current local time that is printed to the screen and stored
 * in a file.
 * AUTHOR: Chelsea Egan (eganch@oregonstate.edu)
 */

#include <assert.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 256
#define NUM_ROOMS 7
#define ROOM_NAME_SIZE 12
#define ROOM_TYPE_SIZE 11
#define TIME_CODE 10
#define TIME_FILE_NAME "currentTime.txt"
#define TRUE 0
#define FALSE 1

int startRoomIndex = -1;
int endRoomIndex = -1;

pthread_mutex_t lock;
pthread_t threadID;

struct Room* rooms;
struct Room {
    char roomName[ROOM_NAME_SIZE];
    char roomType[ROOM_TYPE_SIZE];
    char* connections[6];
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
 * NAME: initPath
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Creates the struct that stores info about the user's path
 * SOURCE: https://stackoverflow.com/a/3536261
 */
void initPath(){
    path.path = (int *)malloc(10 * sizeof(int));
    path.pathUsed = 0;
    /* Start with a path size of 10, can grow as needed */
    path.pathSize = 10;
}

/*
 * NAME: addToPath
 * PARAMS: Int holding index of room to be added to path
 * RETURN: void
 * DESCRIPTION: Updates the user path with new room
 */
void addToPath(int roomIndex) {
    /* Resize path array if full */
    if (path.pathUsed == path.pathSize) {
        path.pathSize *= 2;
        path.path = (int *)realloc(path.path, path.pathSize * sizeof(int));
    }

    /* Update the path size and add the room to the path */
    path.path[path.pathUsed++] = roomIndex;
}

/*
 * NAME: getRoomsDirectory
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Find the directory that is holding the rooms file and change into it
 * SOURCE: https://oregonstate.instructure.com/courses/1729341/pages/2-dot-4-manipulating-directories
 */
void getRoomsDirectory() {
    DIR* rootDir;
    char targetDirPrefix[32] = "eganch.rooms.";
    char newestDirName[BUFFER_SIZE];
    struct dirent *fileInDir;
    struct stat dirAttributes;
    int newestDirTime = -1;

    /* Open the current directory */
    rootDir = opendir(".");
    assert(rootDir > 0);

    /* Loop through all of the files in the directory */
    while ((fileInDir = readdir(rootDir)) != NULL) {
        /* If the file contains the target directory's prefix - found! */
        if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) {

            /* Check if this file was created newer than previous */
            stat(fileInDir->d_name, &dirAttributes);
            if ((int)dirAttributes.st_mtime > newestDirTime) {
                newestDirTime = (int) dirAttributes.st_mtime;
                memset(newestDirName, '\0', sizeof(newestDirName));
                /* Store as the newest directory */
                strcpy(newestDirName, fileInDir->d_name);
            }
        }
    }

    /* Change into the newest directory with the matching prefix */
    int result = chdir(newestDirName);
    assert(result == TRUE);

    /* Close the previous current directory */
    closedir(rootDir);
}

/*
 * NAME: getRoomInfo
 * PARAMS: none
 * RETURN: Pointer to array of room structs
 * DESCRIPTION: Reads each room file and stores the info in the structs
 * SOURCES: https://stackoverflow.com/a/11737506
 * https://www.tutorialspoint.com/cprogramming/c_file_io
 */
struct Room* getRoomInfo() {
    DIR* roomsDir;
    FILE* filePtr;
    struct dirent* fileInDir;
    char buffer[BUFFER_SIZE];
    int roomIndex = 0;

    /* Allocate memory for the array of room structs */
    rooms = malloc(NUM_ROOMS * sizeof(struct Room));

    /* Open current directory */
    roomsDir = opendir(".");
    assert(roomsDir != NULL);

    /* Loop through all the files in the directory */
    while ((fileInDir = readdir(roomsDir))) {
        /* Skip files of current and parent diretory */
        if (!strcmp(fileInDir->d_name, ".")) {
            continue;
        }
        if (!strcmp(fileInDir->d_name, "..")) {
            continue;
        }

        /* Empty out buffer used to read from file */
        memset(buffer, '\0', BUFFER_SIZE);

        /* Open file for reading */
        filePtr = fopen(fileInDir->d_name, "r");
        assert(filePtr != NULL);

        /* Set up all the struct variables
         * Empty the strings
         * Set number of connections to 0
         * Set the room index */
        memset(rooms[roomIndex].roomName, '\0', ROOM_NAME_SIZE);
        memset(rooms[roomIndex].roomType, '\0', ROOM_TYPE_SIZE);
        rooms[roomIndex].numConnections = 0;
        rooms[roomIndex].index = roomIndex;

        /* Read the first line from the file */
        if (fgets(buffer, BUFFER_SIZE, filePtr) == NULL) {
            printf("ERROR: Failed to read file %s. Exiting. \n");
            exit(1);
        }

        /* Get the length of what was read from the file before newline */
        int length = strcspn(buffer, "\n");

        /* Copy the buffer starting after "ROOM NAME: " excluding the newline */
        strncpy(rooms[roomIndex].roomName, buffer + 11, length - 11);

        /* Get lines from the file as long as they start with "CONNECTION" */
        int connectionIndex = 0;
        while (strstr(fgets(buffer, BUFFER_SIZE, (FILE*)filePtr), "CONNECTION") != NULL) {
            /* Allocate memory for holding the connecting room's name then empty the string */
            rooms[roomIndex].connections[connectionIndex] = malloc(ROOM_NAME_SIZE * sizeof(char));
            memset(rooms[roomIndex].connections[connectionIndex], '\0', ROOM_NAME_SIZE);

            /* Get the length of what was read from the file before newline */
            length = strcspn(buffer, "\n");

            /* Copy the buffer starting after "CONNECTION #: " excluding the newline */
            strncpy(rooms[roomIndex].connections[connectionIndex], buffer + 14, length - 14);

            /* Increment the number of connections held by the room */
            rooms[roomIndex].numConnections++;
            connectionIndex++;
        }

        /* Get the length of what was read from the file before newline */
        length = strcspn(buffer, "\n");

        /* Copy the buffer starting after "ROOM TYPE: " excluding the newline */
        strncpy(rooms[roomIndex].roomType, buffer + 11, length - 11);

        /* If the room type is start or end, set the index */
        if (strcmp(rooms[roomIndex].roomType, "START_ROOM") == TRUE) {
            startRoomIndex = roomIndex;
        } else if (strcmp(rooms[roomIndex].roomType, "END_ROOM") == TRUE) {
            endRoomIndex = roomIndex;
        }

        fclose(filePtr);
        roomIndex++;
    }

    /* Close the rooms directory and change back to the parent */
    closedir(roomsDir);
    chdir("..");

    return rooms;
}

/*
 * NAME: printRoomInfo
 * PARAMS: Int holding the current room index
 * RETURN: void
 * DESCRIPTION: Prints the name and connections of the current room.
 * Then prompts for next room.
 */
void printRoomInfo(int roomIndex) {
    /* Print the room info */
    printf("\nCURRENT LOCATION: %s\n", rooms[roomIndex].roomName);

    printf("POSSIBLE CONNECTIONS: ");
    int i;
    for (i = 0; i < rooms[roomIndex].numConnections; i++) {
        /* If it's the last connection, print with a period instead of a comma */
        if (i == rooms[roomIndex].numConnections - 1) {
            printf("%s.\n", rooms[roomIndex].connections[i]);
        } else {
            printf("%s, ", rooms[roomIndex].connections[i]);
        }
    }

    /* Prompt for next "move" */
    printf("WHERE TO? >");
}

/*
 * NAME: checkRoomIsValid
 * PARAMS: Pointer to char array holding the room name
 * RETURN: Int with room index if valid, else -1
 * DESCRIPTION: If the user entered "time" returns int indicating as such.
 * Otherwise, checks if the room name matches any in the room structs.
 * Returns the index if found, -1 if not found.
 */
int checkRoomIsValid(char* roomName, int currentRoomIndex) {
    /* User requested the time, not a room */
    if (strcmp("time", roomName) == TRUE) {
        return TIME_CODE;
    }

    /* Loop through all of the rooms connected to the current room and return the index
     * of the room from the struct array if a matching room name is found */
    int i;
    for (i = 0; i < rooms[currentRoomIndex].numConnections; i++) {
        if (strcmp(rooms[currentRoomIndex].connections[i], roomName) == TRUE) {
            int j;
            for (j = 0; j < NUM_ROOMS; j++) {
                if (strcmp(rooms[currentRoomIndex].connections[i], rooms[j].roomName) == TRUE) {
                    return j;
                }
            }
        }
    }

    /* Invalid room */
    return -1;
}

/*
 * NAME: getUserInput
 * PARAMS: none
 * RETURN: Int with room index if valid, else -1
 * DESCRIPTION: Reads in the user input and gets the index
 * if the room is valid. If invalid, prompts the user to
 * try again and returns -1.
 */
int getUserInput(int currentRoomIndex) {
    int roomIndex = -1;
    char buffer[BUFFER_SIZE];
    char roomChoice[BUFFER_SIZE];

    /* Clear out the arrays */
    memset(buffer, '\0', sizeof(buffer));
    memset(roomChoice, '\0', sizeof(roomChoice));

    /* Read in the user input */
    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
        printf("ERROR: Failed to read user input. Exiting. \n");
        exit(1);
    }

    /* Get the length of what was read in before newline */
    int length = strcspn(buffer, "\n");

    /* Copy into the roomChoice array */
    strncpy(roomChoice, buffer, length);

    /* Validate the user input */
    roomIndex = checkRoomIsValid(roomChoice, currentRoomIndex);

    /* If invalid input, prompt for new input */
    if (roomIndex == -1) {
        printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
    }

    return roomIndex;
}

/*
 * NAME: printFinalStats
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Prints the number of rooms visited and their names
 */
void printFinalStats() {
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", path.pathUsed);
    int i;
    for (i = 0; i < path.pathUsed; i++) {
        printf("%s\n", rooms[path.path[i]].roomName);
    }
}

/*
 * NAME: getCurrentTime
 * PARAMS: none
 * RETURN: void*
 * DESCRIPTION: Gets the current time, creates a file, and stores the time
 * SOURCE: https://linux.die.net/man/3/strftime
 */
void* getCurrentTime() {
    char timeString[BUFFER_SIZE];
    /* Format matches "1:03pm, Tuesday, September 13, 2016" */
    char* timeFormat = "%l:%M%P, %A, %B %e, %Y%n%n";
    FILE* filePtr;
    time_t currentTime;

    /* Locks the mutex. Will be blocked until the main thread unblocks */
    pthread_mutex_lock(&lock);

    /* Create a file to store the time. Will overwrite existing */
    filePtr = fopen(TIME_FILE_NAME, "w");
    assert(filePtr != NULL);

    /* Get the current time */
    currentTime = time(NULL);

    /* Format the time */
    if (strftime(timeString, sizeof(timeString), timeFormat, localtime(&currentTime)) == 0) {
        printf("\nERROR: Failed to get current time. Exiting.\n");
        exit(1);
    }

    /* Write the tim to the file */
    fprintf(filePtr, timeString);

    /* Close the file */
    fclose(filePtr);

    /* Returns the lock to the main thread and destroys the thread */
    pthread_mutex_unlock(&lock);
    pthread_exit(NULL);
}

/*
 * NAME: createTimeThread
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Creates the thread to be used by the time function
 * SOURCE: https://www.thegeekstuff.com/2012/05/c-mutex-examples/?refcom
 */
void createTimeThread() {
    int result;

    /* Locks the main thread */
    pthread_mutex_lock(&lock);

    /* Creates the thread that will execute getCurrentTimem */
    result = pthread_create(&threadID, NULL, &getCurrentTime, NULL);
    assert (result == TRUE);
}

/*
 * NAME: readTimeFromFile
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Reads the time that was stored in a file
 */
void readTimeFromFile() {
    FILE* filePtr;
    char buffer[BUFFER_SIZE];

    /* Empty out buffer used to read from file */
    memset(buffer, '\0', BUFFER_SIZE);

    /* Open file for reading */
    filePtr = fopen(TIME_FILE_NAME, "r");
    assert(filePtr != NULL);

    /* Read the time from the file */
    if (fgets(buffer, BUFFER_SIZE, filePtr) == NULL) {
        printf("ERROR: Failed to read file %s. Exiting. \n");
        exit(1);
    }

    fclose(filePtr);

    /* Print time to the console */
    printf("\n %s\n", buffer);
}

/*
 * NAME: runRoomProgram
 * PARAMS: none
 * RETURN: void
 * DESCRIPTION: Main loop for the program, which continues until the
 * user reaches the end room. At each new room, prints the room
 * info and gets the next user input. If the user requests the time,
 * creates a thread and transfers the lock to it. Upon finding the end
 * room, calls the function to print the stats.
 */
void runRoomProgram() {
    int currentRoomIndex = startRoomIndex;
    int requestedRoomIndex = -1;

    /* Set up struct to store path info */
    initPath();

    /* Loop until they enter the end room */
    while (currentRoomIndex != endRoomIndex) {
        /* Print current room info and get valid user input */
        do {
            printRoomInfo(currentRoomIndex);
            requestedRoomIndex = getUserInput(currentRoomIndex);
        } while (requestedRoomIndex == -1);

        /* If they requested the time, transfer the lock.
         * Else, if they requested a new room, updates the path*/
        if (requestedRoomIndex == TIME_CODE) {
            createTimeThread();
            pthread_mutex_unlock(&lock);
            pthread_join(threadID, NULL);
            readTimeFromFile();
            requestedRoomIndex = currentRoomIndex;
        } else if (currentRoomIndex != requestedRoomIndex){
            addToPath(requestedRoomIndex);
        }

        currentRoomIndex = requestedRoomIndex;
    }

    /* Upon entering the end room, print stats of the path */
    printf("\nYOU'VE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printFinalStats();
}

int main() {
    /* Create a mutex for thread synchronization*/
    int result = pthread_mutex_init(&lock, NULL);
    assert(result == TRUE);

    /* Set up necessary structs */
    getRoomsDirectory();
    getRoomInfo();

    /* Run the main loop */
    runRoomProgram();

    /* Free allocated memory */
    int i, j;
    for (i = 0; i < NUM_ROOMS; i++) {
        for (j = 0; j < rooms[i].numConnections; j++) {
            free(rooms[i].connections[j]);
        }
    }
    free(rooms);
    free(path.path);

    /* Destroy the mutex */
    pthread_mutex_destroy(&lock);

    return 0;
}