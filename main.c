#include <asuro-lib/asuro.h>
#include <stdio.h>

enum DrivingState {STOPPED, MOVING};
enum Color {BLACK, WHITE};
enum StateIndex {ONBLACK, LINELEFT, LINERIGHT, SEARCHLEFT, SEARCHRIGHT, SEARCHLEFT2, SEARCHRIGHT2, TURNLEFTTOGAP, TURNRIGHTTOGAP, 
	MOVEFORWARDTOGAP, MOVEAWAYFROMOBSTACLE, TURNLEFTOBSTACLE, MOVEFORWARDOBSTACLE, TURNRIGHTOBSTACLE, MOVEFORWARDOBSTACLE2, TURNRIGHTOBSTACLE2, AVOIDOBSTACLE};

const char NUMSTATES = 17;

struct Transition{
	enum StateIndex nextState;
	char pause;
	char resetTicks;
};

struct State{
	struct Transition onBlackBlack;
	struct Transition onBlackWhite;
	struct Transition onWhiteBlack;
	struct Transition onWhiteWhite;
	struct Transition onTrigger;
	struct Transition onTicks;
	unsigned char tickThreshold;
	unsigned char direction[2];
	unsigned char speed[2];
};

const unsigned char BASESPEED = 80;
const unsigned char SPEEDBOOST = 25;

void initializeStates(struct State *states);
void checkTrigger(struct State *state, struct Transition *transition);
void checkTicks(struct State *state, struct Transition *transition, int *ticks);
void checkLineData(struct State *state, struct Transition *transition);
void updateOdometry(int *ticks);
void schmittTrigger(unsigned int y, enum Color *color);
void setSpeed(unsigned char *speed, unsigned char left, unsigned char right);
void frictionBoost(enum DrivingState *drivingState, unsigned char *direction, unsigned char *speed);

//die beiden Methoden werden nicht benutzt.
//Sie dienen nur der schnellen Überprüfung der Lichtverhältnisse
void testLED(void);
void testEncoder(int *ticks);

void initializeStates(struct State *states){
	const static unsigned char TURNINGTICKSGAP = 40;
	const static unsigned char MOVEBACKOBSTACLETICKS = 50;
	const static unsigned char TURNLEFTOBSTACLETICKS = 25;
	const static unsigned char MOVEFORWARDOBSTACLETICKS = 140;
	const static unsigned char TURNRIGHTOBSTACLETICKS = 25; 
	const static unsigned char MOVEFORWARDOBSTACLE2TICKS = 70;
	const static unsigned char TURNRIGHTOBSTACLE2TICKS = 25;
	states[ONBLACK] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=SEARCHRIGHT, .pause=TRUE, .resetTicks=TRUE},
		.onTrigger = (struct Transition) {.nextState=MOVEAWAYFROMOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.onTicks = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.tickThreshold = 0,
		.direction = {FWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[LINELEFT] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=SEARCHLEFT, .pause=TRUE, .resetTicks=TRUE},
		.onTrigger = (struct Transition) {.nextState=MOVEAWAYFROMOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.onTicks = (struct Transition)  {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.tickThreshold = 0,
		.direction = {FWD, FWD},
		.speed = {BASESPEED, BASESPEED + SPEEDBOOST}
	};
	states[LINERIGHT] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=SEARCHRIGHT, .pause=TRUE, .resetTicks=TRUE},
		.onTrigger =(struct Transition)  {.nextState=MOVEAWAYFROMOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.onTicks = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.tickThreshold = 0,
		.direction = {FWD, FWD},
		.speed = {BASESPEED+SPEEDBOOST, BASESPEED}
	};
	states[SEARCHLEFT] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=TRUE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=SEARCHLEFT, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition)  {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=SEARCHRIGHT2, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNINGTICKSGAP,
		.direction = {RWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[SEARCHRIGHT] = (struct State){
		.onBlackBlack =(struct Transition) {.nextState=ONBLACK, .pause=TRUE, .resetTicks=FALSE},
		.onBlackWhite =(struct Transition) {.nextState=LINELEFT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteBlack =(struct Transition) {.nextState=LINERIGHT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteWhite =(struct Transition) {.nextState=SEARCHRIGHT, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=SEARCHLEFT2, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNINGTICKSGAP,
		.direction = {FWD, RWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[SEARCHLEFT2] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=TRUE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=LINELEFT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=LINERIGHT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=SEARCHLEFT2, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition)  {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=TURNRIGHTTOGAP, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = 2*TURNINGTICKSGAP,
		.direction = {RWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[SEARCHRIGHT2] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=TRUE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=LINELEFT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=LINERIGHT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=SEARCHRIGHT2, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition)  {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=TURNLEFTTOGAP, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = 2*TURNINGTICKSGAP,
		.direction = {FWD, RWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[TURNLEFTTOGAP] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=TRUE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=TURNLEFTTOGAP, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=MOVEFORWARDTOGAP, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNINGTICKSGAP,
		.direction = {RWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[TURNRIGHTTOGAP] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=TRUE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=TRUE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=TURNRIGHTTOGAP, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=MOVEFORWARDTOGAP, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNINGTICKSGAP,
		.direction = {FWD, RWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[MOVEFORWARDTOGAP] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=MOVEFORWARDTOGAP, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=SEARCHRIGHT, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = 40,
		.direction = {FWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[MOVEAWAYFROMOBSTACLE] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=MOVEAWAYFROMOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=MOVEAWAYFROMOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=MOVEAWAYFROMOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=MOVEAWAYFROMOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger =(struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=TURNLEFTOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = MOVEBACKOBSTACLETICKS,
		.direction = {RWD, RWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[TURNLEFTOBSTACLE] = (struct State){
		.onBlackBlack = (struct Transition)  {.nextState=TURNLEFTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=TURNLEFTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=TURNLEFTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=TURNLEFTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger =(struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNLEFTOBSTACLETICKS,
		.direction = {RWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[MOVEFORWARDOBSTACLE] = (struct State){
		.onBlackBlack = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger =(struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = MOVEFORWARDOBSTACLETICKS,
		.direction = {FWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[TURNRIGHTOBSTACLE] = (struct State){
		.onBlackBlack = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger =(struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE2, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNRIGHTOBSTACLETICKS,
		.direction = {FWD, RWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[MOVEFORWARDOBSTACLE2] = (struct State){
		.onBlackBlack = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=MOVEFORWARDOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger =(struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE2, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = MOVEFORWARDOBSTACLE2TICKS,
		.direction = {FWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[TURNRIGHTOBSTACLE2] = (struct State){
		.onBlackBlack = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition)  {.nextState=TURNRIGHTOBSTACLE2, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger =(struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition)  {.nextState=AVOIDOBSTACLE, .pause=TRUE, .resetTicks=TRUE},
		.tickThreshold = TURNRIGHTOBSTACLE2TICKS,
		.direction = {FWD, RWD},
		.speed = {BASESPEED, BASESPEED}
	};
	states[AVOIDOBSTACLE] = (struct State){
		.onBlackBlack = (struct Transition) {.nextState=ONBLACK, .pause=FALSE, .resetTicks=FALSE},
		.onBlackWhite = (struct Transition) {.nextState=LINELEFT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteBlack = (struct Transition) {.nextState=LINERIGHT, .pause=FALSE, .resetTicks=FALSE},
		.onWhiteWhite = (struct Transition) {.nextState=AVOIDOBSTACLE, .pause=FALSE, .resetTicks=FALSE},
		.onTrigger = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.onTicks = (struct Transition) {.nextState=255, .pause=FALSE, .resetTicks=FALSE},
		.tickThreshold = 0,
		.direction = {FWD, FWD},
		.speed = {BASESPEED, BASESPEED}
	};
}

void checkTrigger(struct State *state, struct Transition *transition){
	if(transition->nextState != 255) return;
	if(state->onTrigger.nextState == 255) return;
	for(int i=0; i<50; i++) PollSwitch();
	if(PollSwitch() == 0) return;
	*transition = state->onTrigger;
	MotorSpeed(0,0);
	StatusLED(RED);
	Msleep(200);
	StatusLED(GREEN);
}

void checkTicks(struct State *state, struct Transition *transition, int *ticks){
	if(transition->nextState != 255) return;
	if(state->onTicks.nextState == 255) return;
	if(ticks[LEFT]+ticks[RIGHT] >= state->tickThreshold) *transition = state->onTicks;
}

void checkLineData(struct State *state, struct Transition *transition){
	if(transition->nextState != 255) return;
	const static int BLACK_THRESHOLD = 600;
	const static int WHITE_THRESHOLD = 800;
	unsigned int lineData[2];
	LineData(lineData);
	int error = lineData[RIGHT] - lineData[LEFT];
	if(lineData[LEFT] <= BLACK_THRESHOLD && lineData[RIGHT] <= BLACK_THRESHOLD){
		*transition = state->onBlackBlack;
		}else if(lineData[LEFT] >= WHITE_THRESHOLD && lineData[RIGHT]>= WHITE_THRESHOLD){
		*transition = state->onWhiteWhite;
		}else if(error > 0){
		*transition = state->onBlackWhite;
		}else if(error <= 0){
		*transition = state->onWhiteBlack;
	}
}

void updateOdometry(int *ticks){
	static unsigned int odometryData[4][2];
	static unsigned char index = 0;
	static enum Color lastArea[2] = {BLACK, BLACK};
	OdometryData(odometryData[index]);
	index = (index+1)%4;
	unsigned int y_left = (odometryData[0][LEFT] + odometryData[1][LEFT] + odometryData[2][LEFT] + odometryData[3][LEFT])/4;
	unsigned int y_right = (odometryData[0][RIGHT] + odometryData[1][RIGHT] + odometryData[2][RIGHT] + odometryData[3][RIGHT])/4;
	enum Color newArea[2] = {lastArea[LEFT], lastArea[RIGHT]};
	schmittTrigger(y_left, &newArea[LEFT]);
	schmittTrigger(y_right, &newArea[RIGHT]);
	for(int side=0; side<2; side++){
		if(newArea[side] != lastArea[side]){
			ticks[side]++;
			lastArea[side] = newArea[side];
		}
	}
}

void schmittTrigger(unsigned int y, enum Color *color){
	const static unsigned int UPPERTHRESHOLD = 480;
	const static unsigned int LOWERTHRESHOLD = 470;
	if(y > UPPERTHRESHOLD) *color = BLACK;
	if(y < LOWERTHRESHOLD) *color = WHITE;
}

void setSpeed(unsigned char *speed, unsigned char left, unsigned char right){
	speed[LEFT] = left;
	speed[RIGHT] = right >= 5 ? right-5 : right;
}

void frictionBoost(enum DrivingState *drivingState, unsigned char *direction, unsigned char *speed){
	if(*drivingState == MOVING){
		return;
	}
	MotorDir(direction[LEFT], direction[RIGHT]);
	MotorSpeed(200, 200);
	*drivingState = MOVING;
	Msleep(10);
	MotorSpeed(speed[LEFT], speed[RIGHT]);
}

int main(void)
{
	Init();
	struct State states[NUMSTATES];
	initializeStates(states);
	const static int TICKRATEMOTOR = 70;
	const static int TICKRATESENSOR = 2;
	enum StateIndex currentStateIndex = ONBLACK;
	enum DrivingState drivingState = STOPPED;
	int ticks[2] = {0,0};
	int pauseCyclesLeft = 0;
	unsigned char speed[2], direction[2];
	direction[LEFT] = direction[RIGHT] = FWD;
	setSpeed(speed, BASESPEED, BASESPEED);
	FrontLED(ON);
	frictionBoost(&drivingState, direction, speed);
	Msleep(500);
	unsigned long nextTickMotor = Gettime();
	unsigned long nextTickSensor = Gettime();
	while(1){
		if(Gettime() >= nextTickSensor){
			nextTickSensor = Gettime() + TICKRATESENSOR;
			updateOdometry(ticks);
		}
		if(Gettime() >= nextTickMotor){
			nextTickMotor = Gettime() + TICKRATEMOTOR;
			if(pauseCyclesLeft > 0){
				pauseCyclesLeft--;
				continue;
			}
			struct Transition transition = {.nextState=255, .pause=0,.resetTicks=0};
			//testLED();
			//testEncoder(ticks);
			checkTrigger(&states[currentStateIndex], &transition);
			checkTicks(&states[currentStateIndex], &transition, ticks);
			checkLineData(&states[currentStateIndex], &transition);
			currentStateIndex = transition.nextState;
			if(transition.resetTicks){
			ticks[LEFT] = ticks[RIGHT] = 0;
			}
			if(transition.pause){
			pauseCyclesLeft = 3;
			direction[LEFT] = direction[RIGHT] = BREAK;
			setSpeed(speed, 0, 0);
			drivingState = STOPPED;
			}else{
			direction[LEFT] = states[currentStateIndex].direction[LEFT];
			direction[RIGHT] = states[currentStateIndex].direction[RIGHT];
			setSpeed(speed, states[currentStateIndex].speed[LEFT], states[currentStateIndex].speed[RIGHT]);
			frictionBoost(&drivingState, direction, speed);
			}
			MotorDir(direction[LEFT], direction[RIGHT]);
			MotorSpeed(speed[LEFT], speed[RIGHT]);
		}
	}
	return 0;
}

/*void testEncoder(int *ticks){
	char buf[15];
	sprintf(buf, "%d, %d | \n", ticks[LEFT], ticks[RIGHT]);
	SerPrint(buf);
}

void testLED(void){
	const static int BLACK_THRESHOLD = 600;
	const static int WHITE_THRESHOLD = 800;
	unsigned int lineData[2];
	LineData(lineData);
	int error = lineData[RIGHT] - lineData[LEFT];
	if(lineData[LEFT] <= BLACK_THRESHOLD && lineData[RIGHT] <= BLACK_THRESHOLD){
		BackLED(OFF, OFF);
		}else if(lineData[LEFT] >= WHITE_THRESHOLD && lineData[RIGHT]>= WHITE_THRESHOLD){
		BackLED(ON, ON);
		}else if(error > 0){
		BackLED(OFF, ON);
		}else if(error <= 0){
		BackLED(ON, OFF);
	}
}*/
