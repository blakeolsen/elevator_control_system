### Elevator Control System

A C++ application for simulating an Elevator Control System. The Elevator Control System can handle up to 16 different elevators, can report the state of the any of the elevators, add a pickup request, and step in time.


## Environment Assumptions

* For each time step an elevator can travel at most one floor in any direction
* Requesting the state of an elevator will give the current state and will not change time
* A pickup request will be responded to in the least amount of time (the closest elevator not traveling away from the requested floor)

## Personal Additions to the Interface

* A pickup request includes: The pickup floor and the direction to travel. Then when the elevator reaches the intended floor a goal floor is requested (must be in direction of travel)
* The user can request the state of any particular elevator or all of the elevator
* floor numbers begin at floor 1 and count up to the number of floors specified
* elevator numbers begin at 1 and count up to the number of elevators specified
* all elevators begin on floor 1

## Syntax

execute the Elevator Control System by:

./elevator_control_system [ARGS...]

possible arguments:
* `-e elevators` - `elevators` is the number of elevators in the system specified by an int (0 < `elevators` <= 16), default = 1
* `-f floors` - `floors` is the number of floors in the system (0 < `floors`), default = 10

After executing the Elevator Control System, the control system has 4 different commands:
* `status [elevator]` - by typing status the user can request the status of all of the elevators or the user can specify a particular `elevator` by elevator id number
* `pickup floor dir` - the user can request an elevator pickup on the specified `floor` and in a given direction, once the elevator reaches the floor then it requests a goal floor in the form of an integer
* `step` - Simulates the movement of the elevators for one time cycle
* `quit` - exits the simulation

## Notes to Reviewer
* I chose to use C++ since the class I am currently taking is taught in C, so I had a working parser and main function I just reused
* I actually had many troubles with the parser, having multiple instances of argument misses, which took my a large portion (1 hr) to solve and fix
* I had to for go many of the functions of the Controller, limitations include
	* an elevator can only be requested if there is already an elevator on that floor
	* minimal saftey checks
	* limited testing (wasn't able to make an automized test)
	* difficult UI

## Example
`
bash> ./elevator_control_system -f 1000
Starting New Elevator ID:1, on Floor:1
ECS> pickup 1 up
Elevator 1 arrived at 1, where would you like to go >  3
ECS> status
Elevator Number 1 is on floor 1 and is moving UP
ECS> step
Stepping in time
ECS> status
Elevator Number 1 is on floor 2 and is moving UP
ECS> step
Stepping in time
Elevator 1 dropped off on floor 3
ECS> status
Elevator Number 1 is on floor 3 and is STOPPED
ECS> pickup 3 down
Elevator 1 arrived at 3, where would you like to go >  1
ECS> status
Elevator Number 1 is on floor 3 and is moving DOWN
ECS> step
Stepping in time
ECS> status
Elevator Number 1 is on floor 2 and is moving DOWN
ECS> step
Stepping in time
Elevator 1 dropped off on floor 1
ECS> status
Elevator Number 1 is on floor 1 and is STOPPED
`



