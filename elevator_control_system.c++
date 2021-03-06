/* Blake Olsen
 * Elevator Control System
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <climits>


///////////////
/// globals ///
///////////////

/* default variables*/
#define DEFAULTELEVATORS   1
#define DEFUALTFLOORS 	   10

/* directions */
#define UP 		   1
#define STOPPED 	   0
#define DOWN	           -1

 /* request types */
#define NONE               -100
/* equivilent to OPTION programming (safe because no request/floor is -100) */

#define STATUS 	           1
#define PICKUP    	   2
#define STEP               3 
#define QUIT		   4      

 /* elevator goalFloors */
 #define UNLOAD	           1
 #define LOAD		   2
 #define BOTH              3

 /* other */
 #define MAXLINE           1024
 char prompt[] = "ECS>  ";  

///////////////////////
/// Data Structures ///
///////////////////////

/* request - the request data structure contains information about the 
 *           users input
 *
 * Possible Inputs -
 *     STATUS - prints the status of the elevator number
 *              floor - represents the elevator number
 *                      if NONE then prints all floors
 *              dir - undefined
 *    
 *     PICKUP - adds a pickup request to a specific floor
 *              floor - the floor requested to be picked up at
 *              dir - the direction the elevator must go once picked up at
 *              
 *     STEP - steps one instance in time
 *            floor - undefined
 *            dir - undefined
 *
 *     QUIT - exits the program controller loosing all state
 *            floor - undefined
 *            dir - undefined
 */
struct request {
  int command;
  int floor;
  int dir;
};

/* elevator movement - We will assign floors to the elevator with 
 *                     prioity queues (using linkedLists)
 *
 * NOTE: We are not using the C++ standard prioity queue class because 
 *       order depends on elevator position and direction which are not
 *       available in the standard library
 * 
 * each floor request includes a floor and whether it is to be loaded
 * or unloaded
 */
struct node {
  int floor;
  int loaded;
  struct node *next;
};

struct linkedList {
  struct node *first;
};

/* elevator - contains the information of each elevator including current    
 *            floor, direction traveling, and a priority queue of the next
 *            floor intendend to move to
 */
struct elevator {
  int id;
  int floor;
  int dir;
  struct linkedList *goalFloors;
};



/////////////////
/// Functions ///
/////////////////

struct elevator **makeSystem(int numElevators, int floors);
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
   struct elevator **elevators = makeSystem(numElevators, floors);
   struct request *request = (struct request*)malloc(sizeof(request));
   
   while (1) {
     getRequest(request);
     
     switch(request->command) {
     case STATUS:
       printElevatorStatus(elevators, numElevators, request);
       break;
     case PICKUP:
       if ((request->floor == NONE) || (request->dir == NONE)) {
	 printf("Incorrect pickup notation\n");
	 break;
       }
       else if (((request->floor == 1) && (request->dir == DOWN)) ||
		((request->floor == floors) && (request->dir == UP))) {
	 printf("Cannot Go in that direction");
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
   /* should never happen */
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
struct elevator **makeSystem(int numElevators, int floors) {
  int i; struct elevator *newE; struct linkedList *newPQ;
  
  /* initiate an array of elevators */
  struct elevator **elevators = 
    (struct elevator**)malloc(sizeof(elevator *)*numElevators);
  
  for(i = 0; i < numElevators; i++) {
    newPQ = (struct linkedList *)malloc(sizeof(struct linkedList));
    newPQ->first = NULL;

    /* implement the elevator */
    newE = (struct elevator*)malloc(sizeof(elevator));
    newE->id = i+1;
    newE->floor = (rand() % floors) + 1;
    newE->dir = STOPPED;
    newE->goalFloors = newPQ;
    elevators[i] = newE;

    /* clean up */
    newE = NULL;
    newPQ = NULL;
    printf("Starting New Elevator ID:%d, on Floor:%d\n", elevators[i]->id, 
	   elevators[i]->floor);
  }
  
  free(newPQ);
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
 */
void getRequest(struct request *request) {
  char line[MAXLINE];
  
  /* reset request */
  request->command = NONE;
  request->floor = NONE;
  request->dir = NONE;

  printPrompt();
  if((fgets(line, MAXLINE, stdin) == NULL) && ferror(stdin)) {
    printf("Error with Fgets\n");
    fflush(stdout);
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
 *
 * NOTE: parser frame taken from inclass project for Spring 15213 tshlab 
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
      request->floor = floors;
    
    
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
  /* in case where floor represents the elevator id */
  if (request->floor == NONE) {
    for(i = 0; i < num; i++) {
      printE(elevators[i]);
    }
  }
  else if (request->floor <= 0) {
    printf("Cannot find Elevator\n");
    return;
  }
  else {
    if (request->floor > num) {
      printf("Not a elevator number %d\n", request->floor);
    }
    else
      printE(elevators[(request->floor) - 1]);
  }
}
/* printE - prints the elevator information
 */
void printE(struct elevator *elevator){
  /* 20 letter word for direction */
  char *dir = (char *)malloc(sizeof(char)*20);
  if (elevator->dir == UP)
    strcat(dir, "moving UP");
  else if (elevator->dir == DOWN)
    strcat(dir, "moving DOWN");
  else if (elevator->dir == STOPPED)
    strcat(dir, "STOPPED");
  
  /* prints the elevator name and location */
  printf("Elevator Number %d is on floor %d and is %s\n", 
	 elevator->id, elevator->floor, dir);
  

  /* 20 letter word for load */
  char *load = (char *)malloc(sizeof(char)*20);
  /* prints the intended goal floors */
  struct node *curr = elevator->goalFloors->first;

  while(curr != NULL) {
    if (curr->loaded == UNLOAD)
      strcat(load, "UNLOAD");
    else if (curr->loaded == LOAD)
      strcat(load, "LOAD");
    else
      strcat(load,"LOAD AND UNLOAD");
    
    printf("\t Stopping on %d, to %s\n", curr->floor,
	   load);
    
    load = strcpy(load,"");
    curr = curr->next;
  }
 
  dir = NULL;
  load = NULL;
  free(dir);
  free(load);
}

int sign(int x);
void loadup(struct elevator *elevators);
/* pickup - sets the elevator to respond to the pickup request
 *
 * NOTE: the elevator of lowest id will of equal distance will pickup first
 */

void pickup(struct elevator **elevators, int num, 
	    struct request *request) {
  int distA; int distB; int dist; int i; int leastID; int leastDist; int most;
  leastDist = INT_MAX;
  struct node* curr;
  
/* distA is the distance between the elevator and pickup floor
 * distB is the additional distance the elevator must travel to pickup floor */
  for(i = num-1; i >= 0; --i) {
    distA = (elevators[i]->floor) - (request->floor);
    
    /* if there are no goalFloors then the elevator is stopped */
    if (elevators[i]->dir == STOPPED)
      distB = 0;
    else if (elevators[i]->dir == UP) {
      /* elevator is below going up */
      if (distA <= 0) 
        distB = 0;
      /* elevator is above going up */
      else {
	/* if the last goal floor is already below then we must travel to the
         * highest floor then back down
         */
	curr = elevators[i]->goalFloors->first;
	if (curr != NULL) {
	  most = curr->floor;
	  while(curr->next != NULL) {
	    if (curr->next->floor < most)
	      break;
	    curr = curr->next;
	    most = curr->floor;
	  }
	}
	distB = (elevators[i]->floor) - most;
      }
    }
    else /* direction is Down */ {
      /* elevator is above coming down */
      if (distA >= 0)
	distB = 0;
      /* elevator is above coming up */
      else {
	/* if the last goal floor is already below then we must travel to the
	 * highest floor then back down
	 */
        curr = elevators[i]->goalFloors->first;
	if (curr != NULL) {
	  most = curr->floor;
	    while(curr->next != NULL) {
	      if (curr->next->floor > most)
		break;
	      curr = curr->next;
	      most = curr->floor;
	    }
	}
	distB = (elevators[i]->floor) - most;
      }
    }
    
    dist = (2*abs(distB)) + abs(distA);
    if (dist < leastDist) {
      leastDist = dist;
      leastID = i;
    }
  }
   
  struct node *newN = (struct node*)malloc(sizeof(node));
  newN->floor = request->floor;
  newN->loaded = LOAD;
  newN->next = NULL;

  /* load the new request floor */
  struct node *temp;
  curr = elevators[leastID]->goalFloors->first;
  if (curr == NULL) {
    elevators[leastID]->goalFloors->first = newN;
    elevators[leastID]->dir = sign(newN->floor - elevators[leastID]->floor);
  }
  else {
    temp = curr->next;
    while (temp != NULL) {
      /* check if the new floor is the first goal floor */
      if (elevators[leastID]->dir == sign((newN->floor) - (curr->floor))) {
	newN->next = temp;
	curr->next = newN;
	break;
      }
      /* if the floor is already registered and will be unloaded then         
	 it will be BOTH unloaded and loaded */
      else if (((curr->floor) - (newN->floor)) == 0) {
	if (curr->loaded == UNLOAD)
	  curr->loaded = BOTH;
	break;
      }
      curr = temp;
      temp = temp->next;
    }
    curr->next = newN;
  }
  if(elevators[leastID]->floor == 
     elevators[leastID]->goalFloors->first->floor) 
    loadup(elevators[leastID]);
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
	 elevator->id, elevator->floor);
  
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
  else if((sign(floors - elevator->floor) != elevator->dir) && 
	  (elevator->dir != 0)) {
    printf("Floor not in direction of elevator try again\n");
    loadup(elevator);
    return;
  }
  else {
    /* deq the first element */
    struct node *pickedUp = elevator->goalFloors->first;
    elevator->goalFloors->first = elevator->goalFloors->first->next;
    pickedUp->next = NULL;

    /* reuse the floor for the pick up floor */
    pickedUp->floor = floors;
    pickedUp->loaded = UNLOAD;
    
    struct node *curr; struct node *temp;
    curr = elevator->goalFloors->first;

    if (curr == NULL) {
      elevator->goalFloors->first = pickedUp;
      if(sign(elevator->floor - pickedUp->floor) == DOWN)
	elevator->dir--;
      else
	elevator->dir++;
    }   
    else {
      /* check if the new floor is the first goal floor */
      if ((elevator->dir) == sign((pickedUp->floor) - (curr->floor))) {
	pickedUp->next = curr;
	elevator->goalFloors->first = pickedUp;
	return;
      }
      /* if the floor is already registered and will be unloaded then 
         it will be BOTH unloaded and loaded */
      else if (((curr->floor) - (pickedUp->floor)) == 0) {
	if (curr->loaded == UNLOAD)
	  curr->loaded = BOTH;
	return;
      }
      temp = curr->next;
      while (curr != NULL) {
	/* check if the new floor is the first goal floor */
	if ((elevator->dir) == sign((pickedUp->floor) - (curr->floor))) {
	  pickedUp->next = temp;
	  curr->next = pickedUp;
	  return;
	}
	/* if the floor is already registered and will be unloaded then   
	   it will be BOTH unloaded and loaded */
	else if (((curr->floor) - (pickedUp->floor)) == 0) {
	  if (curr->loaded == UNLOAD)
	    curr->loaded = BOTH;
	  return;
	}
	curr = temp;
	temp = temp->next;
      }
      pickedUp->next = temp;
      curr->next = pickedUp;
    }
  }
}


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
    if (curr->dir == DOWN) {
      if(curr->floor == 0) {
	fprintf(stderr, "Scheduling Mixup");
	return;
      }
      else {
        curr->floor--;
	/* we have reached a goal floor */
        if (curr->goalFloors->first->floor == curr->floor) {
          temp = curr->goalFloors->first;
	  if(temp->loaded == LOAD) 
	    loadup(curr);
	  else if (temp->loaded == UNLOAD)
	    printFloorReached(curr);
	  else {
	    loadup(curr);
	    printFloorReached(curr);
	  }
          if(curr->goalFloors->first == NULL)
	    curr->dir = STOPPED;
	  else if((curr->dir) != sign(curr->goalFloors->first->floor - 
				      curr->floor))
	    curr->dir = ~curr->dir+1;
	}
      }
    }
    else if (curr->dir == UP) {
      curr->floor++;
      /* we have reached a goal floor */
      if (curr->goalFloors->first->floor == curr->floor) {
	temp = curr->goalFloors->first;
	if(temp->loaded == LOAD)
	  loadup(curr);
	else if (temp->loaded == UNLOAD)
	  printFloorReached(curr);
	else {
	  loadup(curr);
	  printFloorReached(curr);
	}
	if(curr->goalFloors->first == NULL)
	  curr->dir = STOPPED;
	else if((curr->dir) != sign(curr->goalFloors->first->floor - 
				    curr->floor))
	  curr->dir = ~curr->dir+1;
      }
    }
  }
}

/* printFloorReached - tells user requested floor was reached
 */
void printFloorReached(struct elevator *elevator) {
  elevator->goalFloors->first = elevator->goalFloors->first->next;
  printf("Elevator %d dropped off on floor %d\n", elevator->id, 
	 elevator->floor);
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
