#pragma config(Sensor, port3,  armGyro,        sensorVexIQ_Gyro)
#pragma config(Sensor, port5,  distRight,      sensorVexIQ_Distance)
#pragma config(Sensor, port6,  distLeft,       sensorVexIQ_Distance)
#pragma config(Sensor, port8,  colorCenter,    sensorVexIQ_ColorHue)
#pragma config(Sensor, port9,  colorRight,     sensorVexIQ_ColorHue)
#pragma config(Sensor, port10, bumpSwitch,     sensorVexIQ_Touch)
#pragma config(Motor,  motor2,          armMotor,      tmotorVexIQ, PIDControl, encoder)
#pragma config(Motor,  motor7,          leftMotor,     tmotorVexIQ, PIDControl, reversed, driveLeft, encoder)
#pragma config(Motor,  motor12,         rightMotor,    tmotorVexIQ, PIDControl, driveRight, encoder)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//




/*
* Grid Tracker Begin
*/
//TODO
// Documentation


/*Setup*/
#define GRID_WIDTH 2
#define GRID_LENGTH 2
#define GRID_THRESHHOLD_FOLLOWER 120
#define GRID_THRESHHOLD_DETECTOR 120
#define GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER  colorCenter
#define GRID_PORT_COLOR_SENSOR_LINE_DETECTOR  colorRight
#define GRID_PORT_GYRO  armGyro
#define GRID_GYRO_THRESHOLD 2
#define GRID_MOTOR_RIGHT rightMotor
#define GRID_MOTOR_LEFT leftMotor
#define GRID_MOTOR_TRAVEL_PER_TURN_IN_MM 200

int GRID_SPEED_PRIMARY = 75;
int GRID_SPEED_SECONDARY = 25;
int GRID_TURN_SPEED_FAST = 50;
int GRID_TURN_SPEED_SLOW = 0;
bool GRID_DEBUG = true;


/*Local variables*/
int GridX = 0;
int GridY = 0;
int TargetX = 1;
int TargetY = 1;
int TargetDir = 0;
bool gridPause=true;

#define GRID_DIR_NORTH 0
#define GRID_DIR_EAST  270
#define GRID_DIR_SOUTH  180
#define GRID_DIR_WEST 90

int GridDirection = GRID_DIR_NORTH;
string sGridStatus = "";
bool bLineDetected = false;
//#define GRID_NO_MOVE

void GridStopAllMotors()
{
	stopMotor(GRID_MOTOR_RIGHT);
	stopMotor(GRID_MOTOR_LEFT);
}
void GridSetLocation(int x,int y)
{
	GridX = x;
	GridY = y;
}
void GridStatus(const char * s)
{
	sGridStatus = s;
	writeDebugStreamLine(sGridStatus);
}
int GridGetGyroDegrees()
{
	int Degrees = getGyroDegrees(GRID_PORT_GYRO);
	if(Degrees < 0)
	{
		while(Degrees < 0)
			Degrees += 360;
	}
	if(Degrees > 360)
	{
		while(Degrees > 360)
			Degrees -= 360;
	}
	return Degrees;
}
int GridGetDistance(int currentposition,int targetposition)
{
	int Distance;
	if(currentposition-targetposition < -180)
	{
		Distance=(currentposition-targetposition+180) * -1;
	}
	else
	{
		Distance=currentposition-targetposition;
	}
	return Distance;
}
void GridUpdateStatus()
{
	if(GRID_DEBUG)
	{
		displayTextLine(line1,"%d:%d %d:%d Dir:%d:%d:%d",GridX,GridY,TargetX,TargetY,GridGetGyroDegrees(),TargetDir,GridDirection);
		displayTextLine(line2,"%d:%d  %d:%d",getDistanceValue(distLeft),getDistanceValue(distRight),getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER),getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR));
		displayText(line3,sGridStatus);
	}
}
task Display()
{
	repeat (forever) {
		GridUpdateStatus();
		sleep(25);
	}

}
void GridInit()
{
	startTask(Display);
	clearDebugStream();
	GridStatus("GridInit");
	short count = 200;
	startGyroCalibration( GRID_PORT_GYRO, gyroCalibrateSamples64 );
	// delay so calibrate flag can be set internally to the gyro
	wait1Msec(100);

	/* Initialize Color Sensors while gyro is calibrating*/
	if(getColorMode(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER) != colorTypeGrayscale_Reflected)
	{
		setColorMode(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER, colorTypeGrayscale_Reflected);
		while(!getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER))
		{
			sleep(25);
		}
	}
	if(getColorMode(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR) != colorTypeGrayscale_Reflected)
	{
		setColorMode(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR, colorTypeGrayscale_Reflected);
		while(!getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR))
		{
			sleep(25);
		}
	}
	// wait for calibration to finish or 2 seconds, whichever is longer
	while( getGyroCalibrationFlag(GRID_PORT_GYRO) && (count-- > 0) ) {
		char Status[64];
		sprintf(Status,"GridInit Testing Gyro %d",count);
		GridStatus(Status);
		wait1Msec(100);
	}
	// reset so this is 0 heading
	resetGyro(GRID_PORT_GYRO);
	GridStatus("GridInit Complete");
	sleep(1000);
}


void GridPause()
{
	GridStatus("Pause");
	gridPause = true;
}
void GridResume()
{
	GridStatus("GridResume");
	gridPause = false;
}
void GridGoto(int x,int y)
{
	GridStatus("Goto");
	TargetX=x;
	TargetY=y;
	GridResume();
}


void GridSetDirection(int Direction)
{
	GridStatus("GridSetDirection");
	updateMotorDriveTrain();
	TargetDir=Direction;

	while(abs(GridGetGyroDegrees()-Direction) > GRID_GYRO_THRESHOLD)
	{
		int Distance = GridGetDistance(GridGetGyroDegrees(),Direction);
		if(Distance > 0 && Distance <= 180)
		{
			//Right
#ifndef GRID_NO_MOVE
			setMotorSpeeds(GRID_TURN_SPEED_FAST,GRID_TURN_SPEED_SLOW);
#endif
		}
		else
		{
			//Left
#ifndef GRID_NO_MOVE
			setMotorSpeeds(GRID_TURN_SPEED_SLOW,GRID_TURN_SPEED_FAST);
#endif
		}
	}
#ifndef GRID_NO_MOVE
	GridStopAllMotors();
#endif
	GridDirection = Direction;
}

void GridTurnToLine()
{
	bool bFound = false;
	bool bOnLine = false;
	bool bReverse=false;
	updateMotorDriveTrain();
#ifndef	GRID_NO_MOVE
	setMotorSpeeds(GRID_TURN_SPEED_FAST, GRID_TURN_SPEED_FAST*-1);
#endif
	GridStatus("GridToLine Right");
	/*Find the right edge, when turning right wait until we cross the whole line*/
	int leftLimit = GridDirection+45;
	int rightLimit;
	if(GridDirection-45+360 >  360)
		rightLimit=GridDirection-45;
	else
		rightLimit=GridDirection-45+360;

	/*Get past zero if going to the right */
	if(rightLimit == 315)
	{
		while(GridGetGyroDegrees() < 340)
		{
			sleep(10);
		}
	}

	while(!bFound && !bReverse)
	{
		if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER) < GRID_THRESHHOLD_DETECTOR)
		{
			bOnLine = true;
		}
		else if(bOnLine)
		{
			bFound = true;
		}
		if(!bOnLine)
		{
			if(GridGetGyroDegrees() < rightLimit)
			{
#ifndef GRID_NO_MOVE
				GridStopAllMotors();
#endif
				bReverse=true;
			}
		}
	}
	if(!bFound)
	{
		GridStatus("GridToLine Left");
		bReverse=false;
#ifndef GRID_NO_MOVE
		setMotorSpeeds(GRID_TURN_SPEED_FAST * -1, GRID_TURN_SPEED_FAST);
#endif
		GridStatus("GridTurnToLine Left Restting to straight");
		while(abs(GridGetGyroDegrees()-GridDirection) > 3)
		{
			sleep(25);
		}

		/*Get past zero if going to the left */
		if(rightLimit == 45)
		{
			while(GridGetGyroDegrees() > 340)
			{
				sleep(10);
			}
		}
		GridStatus("GridTurnToLine Left");
		while(!bFound && !bReverse)
		{

			if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER) < GRID_THRESHHOLD_DETECTOR)
			{
				bFound = true;
			}
			if(GridGetGyroDegrees() > leftLimit)
			{
				bReverse=true;
			}
		}
	}
#ifndef GRID_NO_MOVE
	GridStopAllMotors();
#endif
	GridStatus("GridTurnToLine Done");
	if(!bFound)
	{
		playSound(soundCarAlarm4);
		sleep(2000);
	}
}

void GridProcess()
{
	updateMotorDriveTrain();

	if((GridX != TargetX || GridY != TargetY) && !gridPause)
	{
		if(GridY != TargetY)
		{
			if(GridY < TargetY)
			{
				if(GridDirection != GRID_DIR_NORTH)
				{
					GridStatus("Turning North");
					GridSetDirection(GRID_DIR_NORTH);
					GridTurnToLine();
				}
			}
			else
			{
				GridStatus("Turning South");
				if(GridDirection != GRID_DIR_SOUTH)
				{
					GridSetDirection(GRID_DIR_SOUTH);
					GridTurnToLine();
				}
			}
		}
		else if(GridX != TargetX)
		{
			if(GridX < TargetX)
			{
				GridStatus("Turning East");
				if(GridDirection != GRID_DIR_EAST)
				{
					GridSetDirection(GRID_DIR_EAST);
					GridTurnToLine();
				}
			}
			else
			{
				GridStatus("Turning West");
				if(GridDirection != GRID_DIR_WEST)
				{
					GridSetDirection(GRID_DIR_WEST);
					GridTurnToLine();
				}
			}
		}
		if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_FOLLOWER) > GRID_THRESHHOLD_FOLLOWER)
		{
			GridStatus("Off the line");
#ifndef GRID_NO_MOVE
			setMotorSpeeds(GRID_SPEED_SECONDARY, GRID_SPEED_PRIMARY);
#endif
		}
		else
		{
			GridStatus("On the line");
#ifndef GRID_NO_MOVE
			setMotorSpeeds(GRID_SPEED_PRIMARY, GRID_SPEED_SECONDARY);
#endif
		}

		if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR) < GRID_THRESHHOLD_DETECTOR && bLineDetected == false)
		{
			GridStatus("Line Detected");
			//playSound(soundTollBooth);
			if(GridDirection == GRID_DIR_NORTH)
				GridY++;
			else if(GridDirection == GRID_DIR_SOUTH)
				GridY--;
			else if(GridDirection == GRID_DIR_EAST)
				GridX++;
			else if(GridDirection == GRID_DIR_WEST)
				GridX--;

			bLineDetected = true;
		}
		if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR) > GRID_THRESHHOLD_DETECTOR)
		{
			bLineDetected = false;
		}

	}
	else
	{
		/*Reached the target*/
		if(!gridPause)
		{
#ifndef GRID_NO_MOVE
			GridStopAllMotors();
#endif
			GridPause();
		}

	}
}

void GridFindLine(bool bFarSide)
{
	bool bFound = false;
	GridStatus("Finding Line");
#ifndef GRID_NO_MOVE
	setMotorSpeeds(GRID_SPEED_PRIMARY, GRID_SPEED_PRIMARY);
#endif
	while(!bFound)
	{
		if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR) < GRID_THRESHHOLD_DETECTOR)
		{
			bFound = true;
		}
	}
	if(bFarSide)
	{
		bFound=false;
		while(!bFound)
		{
			if(getColorGrayscale(GRID_PORT_COLOR_SENSOR_LINE_DETECTOR) < GRID_THRESHHOLD_DETECTOR)
			{
				bFound = true;
			}
		}
	}
#ifndef GRID_NO_MOVE
	GridStopAllMotors();
#endif
}

void GridMoveForward(int mm)
{
	int Rotation = (((float)mm/(float)GRID_MOTOR_TRAVEL_PER_TURN_IN_MM) * 360);
	moveMotorTarget(GRID_MOTOR_RIGHT,Rotation,GRID_SPEED_PRIMARY);
	moveMotorTarget(GRID_MOTOR_LEFT,Rotation,GRID_SPEED_PRIMARY);
	waitUntilMotorStop(GRID_MOTOR_RIGHT);
	waitUntilMotorStop(GRID_MOTOR_LEFT);
}

void GridMoveBackward(int mm)
{
	int Rotation = (((float)mm/(float)GRID_MOTOR_TRAVEL_PER_TURN_IN_MM) * 360);
	moveMotorTarget(GRID_MOTOR_RIGHT,Rotation*-1,GRID_SPEED_PRIMARY);
	moveMotorTarget(GRID_MOTOR_LEFT,Rotation*-1,GRID_SPEED_PRIMARY);
	waitUntilMotorStop(GRID_MOTOR_RIGHT);
	waitUntilMotorStop(GRID_MOTOR_LEFT);
}

/*
* Grid Tracker End
*/
bool bBumper1Pressed = false;
task CheckBumper()
{

	bool bMotorOverlimit = false;
	repeat (forever) {
		if(getBumperValue(bumpSwitch) && bBumper1Pressed == false)
		{
			setMotorTarget(armMotor, -130, 20);
			bBumper1Pressed = true;
		}

		if(!getBumperValue(bumpSwitch) && bBumper1Pressed)
		{
			setMotorTarget(armMotor,0, 20);
			bBumper1Pressed = false;
		}
		if(getMotorCurrent(armMotor) > 600 && !bMotorOverlimit)
		{
			bMotorOverlimit = true;
			clearTimer(T1);
		}
		else if(getMotorCurrent(armMotor) > 600 && bMotorOverlimit)
		{
			if(time1[T1] > 5000)
			{
				setMotorSpeed(armMotor,0);
				setMotorBrakeMode(armMotor, motorCoast);
			}
		}
		else
		{
			bMotorOverlimit = false;
		}
	}

}

void GotoPoll()
{
	  bool done = false;
		while(!done)
		{
			displayTextLine(line5,"%d %d",getDistanceValue(distLeft),getDistanceValue(distRight));
			if(getDistanceValue(distLeft) > 50 && getDistanceValue(distRight) > 50)
			{
				updateMotorDriveTrain();
				if(getDistanceValue(distLeft) > getDistanceValue(distRight))
					setMotorSpeeds(10,0);
				else
					setMotorSpeeds(0,10);
				}
				else
				{
					stopAllMotors();
				}
				if(bBumper1Pressed)
				{
					playSound(soundTada);
					sleep(2000);
					GridMoveBackward(100);
					done=true;
			  }
		}
		playSound(soundTada);
		sleep(2000);
}

task main()
{
	GridInit();
	bool done = false;

//#define GRID_TEST_SET_DIRECTION
#ifdef GRID_TEST_SET_DIRECTION
	GridSetDirection(GRID_DIR_EAST);
	sleep(2000);
	GridSetDirection(GRID_DIR_SOUTH);
	sleep(2000);
	GridSetDirection(GRID_DIR_WEST);
	sleep(2000);
	GridSetDirection(GRID_DIR_SOUTH);
	sleep(2000);
	GridSetDirection(GRID_DIR_EAST);
	sleep(2000);
	GridSetDirection(GRID_DIR_NORTH);
	sleep(2000);
	GridSetDirection(GRID_DIR_SOUTH);
	sleep(2000);
	done = true;
#endif

//#define GRID_TEST_TURN_TO_LINE
#ifdef GRID_TEST_TURN_TO_LINE
	GridTurnToLine();
	sleep(2000);

  GridSetDirection(GRID_DIR_EAST);
	GridTurnToLine();
	sleep(2000);

  GridSetDirection(GRID_DIR_SOUTH);
	GridTurnToLine();
	sleep(2000);


  GridSetDirection(GRID_DIR_WEST);
	GridTurnToLine();
	sleep(2000);

	done = true;
#endif

//#define GRID_TEST_MOVE_DISTANCE
#ifdef GRID_TEST_MOVE_DISTANCE
	GridMoveForward(200);
	sleep(2000);
	GridMoveBackward(200);
	sleep(2000);

	GridMoveForward(400);
	sleep(2000);
	GridMoveBackward(400);
	sleep(2000);

	GridMoveForward(100);
	sleep(2000);
	GridMoveBackward(100);
	sleep(2000);

	done = true;
#endif

//#define TEST_DISTANCE
#ifdef TEST_DISTANCE
		startTask(CheckBumper);
		while(!done)
		{
			displayTextLine(line5,"%d %d",getDistanceValue(distLeft),getDistanceValue(distRight));
			if(getDistanceValue(distLeft) > 50 && getDistanceValue(distRight) > 50)
			{
				updateMotorDriveTrain();
				if(getDistanceValue(distLeft) > getDistanceValue(distRight))
					setMotorSpeeds(10,0);
				else
					setMotorSpeeds(0,10);
				}
				else
				{
					stopAllMotors();
				}
				if(bBumper1Pressed)
				{
					playSound(soundTada);
					sleep(2000);
					GridMoveBackward(100);
					done=true;
			  }
		}
		playSound(soundTada);
		sleep(2000);
#endif
	int job=1;
	int sleeptime=0;
	displayText(line5,"job1");
	startTask(CheckBumper);
	while (!done) {

		GridProcess();
		if(gridPause)
		{
			switch(job)
			{
			case 1:
				{
					displayTextLine(line5,"Job %d",job);
					GridFindLine(false);
					sleep(sleeptime);
					GridSetLocation(2,1);
					TargetX = 2;
					TargetY = 1;
					job++;
				}
				break;
			case 2:
				{
					displayTextLine(line5,"Job %d",job);
					playSound(soundTada);
					sleep(sleeptime);
					/*Backup a bit to be further from line*/
					GridSetDirection(GRID_DIR_WEST);
					GridMoveBackward(100);
					GridGoto(0,1);
					job++;
				}
				break;

			case 3:
				{
					displayTextLine(line5,"Job %d",job);
					playSound(soundTada);
					sleep(sleeptime);
					GridGoto(0,2);
					job++;
					/*				while(!done)
					{
					if(getDistanceValue(distLeft) > 150 && getDistanceValue(distRight) > 150)
					{
					updateMotorDriveTrain();
					if(getDistanceValue(distLeft) > getDistanceValue(distRight))
					setMotorSpeeds(10,0);
					else
					setMotorSpeeds(0,10);
					}
					else
					{
					stopAllMotors();
					done=true;
					}
					}
					job++;
					playSound(soundTada);
					sleep(2000);
					*/
				}
				break;
			case 4:
				{
					displayTextLine(line5,"Job %d",job);
					playSound(soundTada);
					sleep(sleeptime);
					GridGoto(2,2);
					job++;
				}
				break;
			case 5:
				{
					displayTextLine(line5,"Job %d",job);
					playSound(soundTada);
					sleep(sleeptime);
					GridGoto(2,3);
					job++;
				}
				break;
			case 6:
				{
					GridSetDirection(GRID_DIR_EAST);
					GridMoveBackward(200);
					GotoPoll();
					GridSetDirection(GRID_DIR_SOUTH);
					GridMoveForward(600);
					done = true;
				}
				break;
			}
		}

		/*	  if(getDistanceValue(distLeft) > 100 && getDistanceValue(distRight) > 100)
		{
		updateMotorDriveTrain();
		if(getDistanceValue(distLeft) > getDistanceValue(distRight))
		setMotorSpeeds(10,0);
		else
		setMotorSpeeds(0,10);
		}
		else
		stopAllMotors();

		displayTextLine(line5,"%d:%d:%d:%d ",getDistanceValue(distLeft),getDistanceValue(distRight),getDistanceSecondStrongest(distLeft),getDistanceSecondStrongest(distRight));*/
		/*
		displaySensorValues(line2, distLeft);
		displaySensorValues(line3, distRight);*/

	}
	displayText(line5,"done");
	playSound(soundPowerOff2);
	sleep(2000);
}
