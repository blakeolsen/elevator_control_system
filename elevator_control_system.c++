/* Blake Olsen
 * Elevator Control System
 * 
 * Began: 11/8/15 - 7:10 PM
 * End: 11/8/15 -
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

///////////////
/// globals ///
///////////////

/* default variables*/
 #define DEFAULTELEVATORS		1
 #define DEFUALTFLOORS 			10

/* directions */
 #define UP  								1
 #define STOPPED 						0
 #define DOWN								-1

 /* request types */
 #define NONE               -100
 #define STATUS 	          1
 #define PICKUP    					2
 #define STEP               3 
 #define QUIT								4      

 /* elevator goalFloors */
 #define UNLOAD	            1
 #define LOAD								2

 /* other */
 #define MAXLINE            1024
 char prompt[] = "ECS> ";  

///////////////////////
/// Data Structures ///
///////////////////////

 /* We will use a linked list to represent the next floor */
 struct node {
 	int floorC;
 	int load;
 	struct node *next;
 };

 struct linkedList {
 		struct node *first;
 		struct node *last;
 };

/* data structure of the elevator */
 struct elevator {
 	int id;
 	int floorC;
 	int direction;
 	struct linkedList *goalFloors;
 };

 /* data structure of a request */
 struct request {
 		int command;
 		int floorC;
 		int dir;
 };

/////////////////
/// Functions ///
/////////////////

struct elevator **makeSystem(int numElevators);
void getRequest(struct request *request);
void printElevatorStatus(struct elevator **elevators, int num, 
												 struct request *request);
void pickup(struct elevator **elevators, int num, 
												 struct request *request);
void timeStep(struct elevator **elevators, int num);
void freeSystem(struct elevator **elevators, int num, struct request *request);

/* run - sets up the environment and executes background work
 *
 * Parameters:
 *   numElevators - the number of elevators in the environment
 *
 *   floors - the number of floors in the system
 *
 * Notes:
 * 	     * All of the elevators together are represented in a list
 */
 void run(int numElevators, int floors) {
   struct elevator **elevators = makeSystem(numElevators);
   struct request *request = (struct request*)malloc(sizeof(request));

   while (1) {
   	 getRequest(request);

 	 	 switch(request->command) {
 	 	 case STATUS:
 	 	 		printElevatorStatus(elevators, numElevators, request);
 	 	 		break;
 	 	 case PICKUP:
 	 	 		if ((request->floorC == NONE) || (request->dir == NONE)) {
 	 	 				printf("Incorrect pickup notation\n");
 	 	 				break;
 	 	 		}
 	 	 		pickup(elevators, numElevators, request);
 	 	 		break;
 	 	 case STEP:
 	 	 		printf("Stepping in time\n");
 	 	 		timeStep(elevators, numElevators);
 	 	 		break;
 	 	 case QUIT:
 	 	 		printf("Exiting Elevator Control System\n");
 	 	 		freeSystem(elevators, numElevators, request);
 	 	 		return;
 	 	 default:
 	 	 		printf("Unknown Command\n Try again\n");
 	 	 		break;
 	 	}
 	 }

   freeSystem(elevators, numElevators, request);
   return;
 }

 ////////////////////////
 /// Helper Functions ///
 ////////////////////////

 /* makeSystem - contructs the list of elevators
  *
  * Parameters:
  *   numElevators - the number of active elevators
  *
  * Note: the elevators will all automatically start on floor 1
  */
struct elevator **makeSystem(int numElevators) {
	int i; struct elevator *newE; struct linkedList *newL;

	struct elevator **elevators = 
			(struct elevator**)malloc(sizeof(elevator *)*numElevators);

	for(i = 0; i < numElevators; i++) {
			/* implement a new floor */
			newE = (struct elevator*)malloc(sizeof(elevator));
			newL = (struct linkedList*)malloc(sizeof(linkedList));
			newL->first = NULL;
			newL->last = NULL;
			newE->id = i+1;
			newE->floorC = 1;
			newE->direction = STOPPED;
			newE->goalFloors = newL;
			elevators[i] = newE;
			newE = NULL;
			newL = NULL;
			printf("Starting New Elevator ID:%d, on Floor:%d\n", elevators[i]->id, elevators[i]->floorC);
	}

	free(newE);
	return elevators;
}


void printPrompt(void);
void parseline(const char *cmdline, struct request *request);
/* getRequest - waits for the user to input a request and parses the result
 *
 * Note:
 *       * if no particular elevator's status is requested then print
 *         the status for all of the elevators
 *       * parser frame taken from inclass project for Spring 15213 tshlab
 */
void getRequest(struct request *request) {
		char line[MAXLINE];

		/* reset request */
		request->command = NONE;
		request->floorC = NONE;
		request->dir = NONE;

		printPrompt();
		if((fgets(line, MAXLINE, stdin) == NULL) && ferror(stdin)) {
				printf("Error with Fgets\n");
				getRequest(request);
				return;
		}

		line[strlen(line)-1] = '\0';

		/* passes line along to parser */
		parseline(line,request);
}


/* printPrompt - prints the prompt for the next command 
 */
void printPrompt(void) {
	printf("%s", prompt);
	fflush(stdout);
}
 
/* parseline - parses the input finding key words
 */
void parseline(const char *cmdline, struct request *request) {
		int floors;

		static char array[MAXLINE];          /* holds local copy of command line */
    const char delims[10] = " \t\r\n";   /* argument delimiters (white-space) */
    char *buf = array;                   /* ptr that traverses command line */
    char *next;                          /* ptr to the end of the current arg */
    char *endbuf;                        /* ptr to end of cmdline string */

    if (cmdline == NULL) {
        printf("Error: command line is NULL\n");
        exit(1);
    }

    (void) strncpy(buf, cmdline, MAXLINE);
    endbuf = buf + strlen(buf);

    while (buf < endbuf) {
      /* Skip the white-spaces */
			buf += strspn (buf, delims);
			if (buf >= endbuf) break;

			next = buf + strcspn (buf, delims);
			if (next == NULL) {
            /* Returned by strchr(); this means that the closing
               quote was not found. */
            (void) fprintf (stderr, "Error: unmatched %c.\n", *(buf-1));
            return;
        }
			*next = '\0';

			/* handles possible command */
			if(!strcmp(buf, "status"))
					request->command = STATUS;
			else if (!strcmp(buf,"pickup")) 
					request->command = PICKUP;
			else if (!strcmp(buf,"step")) {
					request->command = STEP;
					return;
			}
			else if (!strcmp(buf,"quit")) {
				request->command = QUIT;
				return;
			}



			/* handles possible direction inputs */
			if(!strcmp(buf,"down"))
				request->dir = DOWN;
			else if (!strcmp(buf,"up"))
				request->dir = UP;




			/* handles possible floor inputs */
			strcat(buf, "\0");
			if((floors = atoi(buf)) != 0)
				request->floorC = floors;


			/* skip non-keyword */
			buf = next + 1;
		}
}





void printE(struct elevator *elevator);
/* printElevatorStatus - prints the elevator status of the requested elevator
 *
 * Note: if no specific elevator was requested, all elevators will be printed
 */
void printElevatorStatus(struct elevator **elevators, int num, 
												 struct request *request) {
	int i;
	/* in case where floorC represents the elevator id */
	if (request->floorC == NONE) {
			for(i = 0; i < num; i++) {
					printE(elevators[i]);
			}
	}
	else if (request->floorC <= 0) {
		printf("Cannot find Elevator\n");
		return;
	}
	else {
			if (request->floorC > num) {
				printf("Not a elevator number %d\n", request->floorC);
			}
			else
				printE(elevators[(request->floorC) - 1]);
	}
}
/* printE - prints the elevator information
 */
void printE(struct elevator *elevator){
	/* 20 letter word for direction */
	char *dir = (char *)malloc(sizeof(char)*20);
	if (elevator->direction == UP)
		strcat(dir, "moving UP");
	else if (elevator->direction == DOWN)
		strcat(dir, "moving DOWN");
	else if (elevator->direction == STOPPED)
		strcat(dir, "STOPPED");

	printf("Elevator Number %d is on floor %d and is %s\n", 
			elevator->id, elevator->floorC, dir);

	dir = NULL;
	free(dir);
}

int sign(int x);
void loadup(struct elevator *elevators);
/* pickup - sets the elevator to respond to the pickup request
 *
 * NOTE: the elevator of lowest id will of equal distance will pickup first
 */

void pickup(struct elevator **elevators, int num, 
												 struct request *request) {
	int distA; int distB; int dist; int i; int leastID; int leastDist;
	leastDist = INT_MAX;


/* distA is the distance between the elevator and pickup floor
 * distB is the additional distance the elevator must travel to pickup floor */
for(i = num-1; i >= 0; --i) {
	distA = (elevators[i]->floorC) - (request->floorC);

	// if there are no goalFloors then the elevator is stopped
	if (elevators[i]->direction == STOPPED)
		distB = 0;
	else if (elevators[i]->direction == UP) {
		/* elevator is below coming up */
		if (distA < 0) 
			distB = 0;
		/* elevator is below going down */
		else 
			distB = (elevators[i]->floorC) - (elevators[i]->goalFloors->first->floorC);			
	}
	else /* direction is Down */ {
		/* elevator is above coming down */
		if (distA > 0)
			distB = 0;
		/* elevator is above coming up */
		else
			distB = (elevators[i]->floorC) - (elevators[i]->goalFloors->first->floorC);	
	}

	dist = abs(distB) + abs(distA);
	if (dist < leastDist) {
		leastDist = dist;
		leastID = i;
	}
}

	struct node *newN = (node *)malloc(sizeof(node));
	newN->floorC = request->floorC;
	newN->load = LOAD;
	newN->next = NULL;

/* if the elevator is already on the floor then we can immediately load */ 
	if(elevators[leastID]->direction == STOPPED) {
		elevators[leastID]->direction = request->dir;
		elevators[leastID]->goalFloors->first = newN;
		elevators[leastID]->goalFloors->last = newN;
		loadup(elevators[leastID]);
	}
}

/* returns the sign of the of the int */
int sign(int x) {
	return (x > 0) - (x < 0);
}

/* loadup - finds a goal floor of an elevator responding to a pickup
 *          by asking the controller
 *
 * NOTE: goal floor must be in the direction requested by pickup
 */
void loadup(struct elevator *elevator) {
	char line[MAXLINE];
	const char delims[10] = " \t\r\n";

	printf("Elevator %d arrived at %d, where would you like to go >  ",
					elevator->id, elevator->floorC);

	if((fgets(line, MAXLINE, stdin) == NULL) && ferror(stdin)) {
		fprintf(stderr, "Fgets error\n");
		exit(1);
	}

	const char *newFloor = (const char *)line;

	newFloor += strspn(newFloor,delims);
	int floors = atoi(newFloor);

	if(!floors) {
		printf("Could not distinguish line number try again\n");
		loadup(elevator);
		return;
	}
	else if(sign(floors - elevator->floorC) != elevator->direction) {
		printf("Floor not in direction of elevator try again\n");
		loadup(elevator);
		return;
	}
	else {
		struct node *newN = (node *)malloc(sizeof(node));
		newN->floorC = floors;
		newN->load = UNLOAD;

		elevator->goalFloors->first = newN;
		elevator->goalFloors->last = newN;
	}

	elevator->goalFloors->first->load = UNLOAD;
	
}


int getnextgoal(struct elevator *elevator);
void printFloorReached(struct elevator *elevator);
/* timeStep - steps 1 time cycle in advanced
 *
 * Notes: all elevators travel one step in the direction they are traveling
 */
void timeStep(struct elevator **elevators, int num) {
		int i = 0;
		struct elevator *curr;
		struct node *temp;

		for(i = 0; i < num; i++) {
			curr = elevators[i];
			if (curr->direction == DOWN) {
					if(curr->floorC == 0) {
							fprintf(stderr, "Scheduling Mixup");
							return;
					}
					else {
							curr->floorC--;
							if (getnextgoal(curr) == curr->floorC) {
								temp = curr->goalFloors->first->next;
								curr->goalFloors->first = temp;
								printFloorReached(curr);
								if(curr->goalFloors->first == NULL) {
										curr->direction = STOPPED;
										curr->goalFloors->last = NULL;
								}
							}
					}
			}
			else if (curr->direction == UP) {
					curr->floorC++;
					if (getnextgoal(curr) == curr->floorC) {
						temp = curr->goalFloors->first->next;
						curr->goalFloors->first = temp;
						printFloorReached(curr);
							if(curr->goalFloors->first == NULL) {
								curr->direction = STOPPED;
								curr->goalFloors->last = NULL;
								}
					}
			}
		}
}

/* getnextgoal - returns the floor of the next goal
 */
int getnextgoal(struct elevator *elevator) {
	if (elevator->goalFloors->first != NULL)
		return elevator->goalFloors->first->floorC;
	return NONE;
}

/* printFloorReached - tells user requested floor was reached
 */
void printFloorReached(struct elevator *elevator) {
	printf("Elevator %d dropped off on floor %d\n", elevator->id, elevator->floorC);
}

/* freeSystem - frees all objects
 */
void freeSystem(struct elevator **elevators, int num, struct request *request) {
	int i = 0;

	for (i = 0; i < num; i++){
			free(elevators[i]);
	}
	free(elevators);
	free(request);
}


/* main - parses function arguments and runs program
 */
 int main(int argc, char * argv[]) {

 		int elevators, floors, opt;
 		elevators = -1, floors = 0;

 		while ((opt = getopt(argc, argv, "e:f:")) >= 0) {
 				switch (opt) {
 						case 'e':
 							elevators = atoi(optarg);
 							break;
 						case 'f':
 							floors = atoi(optarg);
 							break;
 						default:
 							fprintf(stderr, "Could not resolve flag -%c.\n", opt);
 							return 1;
 				}
 		}

 		/* if the user did not set the number of elevators or 
 			 floors set to default */
 		if(elevators == -1) {
 				elevators = DEFAULTELEVATORS;
 		}
 		if(floors == 0) {
 				floors = DEFUALTFLOORS;
 		}

 		/* ensure that there is a legal number of elevators in the system */
 		if (!((elevators >= 0) && (elevators <= 16))) {
 				fprintf(stderr, "Illegal number of elevators, %d.\nThe Elevator can only handle 1 to 16 elevators.\n", elevators);
 		}

 		run(elevators, floors);
 		return(0);
 }