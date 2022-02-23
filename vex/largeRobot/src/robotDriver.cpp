// include
#include "robotDriver.h"
// define
#define ERROR_BOUND_DRIVE 0.001
#define ERROR_BOUND_TURN 0.5
// class
RobotDriver::RobotDriver(int8_t frontLeftMotorPort, int8_t frontRightMotorPort, int8_t backLeftMotorPort, int8_t backRightMotorPort, int8_t gyroPort, double wheelRad)
 : frontLeftMotor(frontLeftMotorPort), frontRightMotor(frontRightMotorPort), backLeftMotor(backLeftMotorPort), backRightMotor(backRightMotorPort),
 gyro(gyroPort), controller(pros::E_CONTROLLER_MASTER)
{
  //use okapi chasis for drive PID
  chassis = okapi::ChassisControllerBuilder()
  .withMotors({frontLeftMotorPort, backLeftMotorPort}, {(int8_t)-frontRightMotorPort, (int8_t)-backRightMotorPort})
#ifndef ARDUINO_MIDDLE_ENCODER
  .withSensors(std::make_shared<ArduinoEncoder>(arduino_encoder_create(ARDUINO_LEFT_ENCODER)),
    std::make_shared<ArduinoEncoder>(arduino_encoder_create(ARDUINO_RIGHT_ENCODER)))
#else
  .withSensors(std::make_shared<ArduinoEncoder>(arduino_encoder_create(ARDUINO_LEFT_ENCODER)),
    std::make_shared<ArduinoEncoder>(arduino_encoder_create(ARDUINO_RIGHT_ENCODER)),
    std::make_shared<ArduinoEncoder>(arduino_encoder_create(ARDUINO_MIDDLE_ENCODER)))
#endif
  .withDimensions(AbstractMotor::gearset::green, {{4.1_in, 14.6_in}, imev5GreenTPR})
  .build();
  //compute wheel circumference
  wheelCircumference = wheelRad * M_PI;
  //calibrate gyro
  // gyro.reset();
  //size vectors
  // this->armButtonPorts.resize(1);
}
// utility functions
double clamp(double val, double min, double max) {
 double sign = 1;
 if (val < 0) {
   sign = -1;
 }
 if (std::abs(val) < min) {
   return min * sign;
 } else if (std::abs(val) > max) {
   return max * sign;
 } else {
   return val;
 }
}
// getter/setter methods
void RobotDriver::configTurnPID(double kP, double kI, double kD, double dT, int min, int max) {
  this->turnPIDdT = dT;
  this->turnPIDkP = kP;
  this->turnPIDkI = kI;
  this->turnPIDkD = kD;
  this->turnPIDMinSpeed = min;
  this->turnPIDMaxSpeed = max;
}
void RobotDriver::configPositionPID(double kP, double kI, double kD, double dT) {
  this->positionPIDdT = dT;
  this->positionPIDkP = kP;
  this->positionPIDkI = kI;
  this->positionPIDkD = kD;
}

// void RobotDriver::addArmButton(int port1, int port2, int upButton, int downButton) {
//   ArmButtonPorts newArmButton;
//   newArmButton.port1 = port1;
//   newArmButton.port2 = port2;
//   newArmButton.upButton = upButton;
//   newArmButton.downButton = downButton;
//   this->armButtonPorts.push_back(newArmButton);
// }
// driving functions
void RobotDriver::turnPID(double desiredTurnAngle) {
  // constants for PID calculations
  const int final_iterations =  3; //how many times to run within the error bound
  // initialize values to track between loops
  double error = ERROR_BOUND_TURN * 2;
  double error_prior = 0;
  double integral_prior = 0;
  double extra_iterations = final_iterations;
  // everything from here on out is measured in degrees
  double initial = gyro.get_yaw();
  while (extra_iterations > 0) {
    if (std::abs(error) < ERROR_BOUND_TURN) {
      extra_iterations -= 1;
    } else {
      extra_iterations = final_iterations;
    }
    // calculate known distances
    double actual = gyro.get_yaw() - initial;
    error = desiredTurnAngle - actual;
    //sign correct error
    while (error < -180) {
      error += 360;
    }
    while (error > 180) {
      error -= 360;
    }
    // calculate I and D
    double integral = integral_prior + (error*this->turnPIDdT); // sum of error
    double derivative = (error - error_prior)/this->turnPIDdT; // change in error over time
    // using PID constants, calculate output
    double output = this->turnPIDkP * error + this->turnPIDkI * integral + this->turnPIDkD * derivative;
    // pros::lcd::set_text(2, "initial: " + std::to_string(initial));
    // pros::lcd::set_text(3, "actual: " + std::to_string(actual));
    // pros::lcd::set_text(4, "desired: " + std::to_string(desiredTurnAngle));
    // pros::lcd::set_text(5, "Error: " + std::to_string(error));
    // pros::lcd::set_text(6, "Output: " + std::to_string(output));
    // clamp output to motor-compatible values
    output = clamp(output, this->turnPIDMinSpeed, this->turnPIDMaxSpeed);
    // pros::lcd::set_text(7, "Clamped: " + std::to_string(output));
    // set motors to calculated output
    frontLeftMotor = output;
    backLeftMotor = output;
    frontRightMotor = output;
    backRightMotor = output;
    // record new prior values
    error_prior = error;
    integral_prior = integral;
    // delay by dT
    pros::delay(this->turnPIDdT);
  }
}
void RobotDriver::turnPIDAndRecalibrate(double desiredTurnAngle) {
  this->turnPID(desiredTurnAngle);
  this->recalibrateGyro();
}
void RobotDriver::recalibrateGyro() {
  this->gyro.reset();
}
void RobotDriver::positionPID(double desired_dist_inches) {
	 // constants for PID calculations
	 const double maxSpeed = 128;
	 // initialize values to track between loops
	 double error = ERROR_BOUND_DRIVE * 2;
	 double error_prior = 0;
	 double integral_prior = 0;
	 // everything from here on out is measured in revolutions
	 double desired = desired_dist_inches / this->wheelCircumference;
	 this->frontLeftMotor.set_encoder_units(pros::E_MOTOR_ENCODER_ROTATIONS);
	 double initialMotorPosition = this->frontLeftMotor.get_position();
	 while (abs((int)error) > ERROR_BOUND_DRIVE) {
		 // calculate known distances
		 double actual = (this->frontLeftMotor.get_position()) - initialMotorPosition;
		 error = desired - actual;
		 // calculate I and D
		 double integral = integral_prior + (error*this->positionPIDdT); // sum of error
		 double derivative = (error - error_prior)/this->positionPIDdT; // change in error over time
		 // using PID constants, calculate output
		 double output = this->positionPIDkP * error + this->positionPIDkI * integral + this->positionPIDkD * derivative;
		 pros::lcd::set_text(2, "integral: " + std::to_string(integral));
		 pros::lcd::set_text(3, "derivative: " + std::to_string(derivative));
		 pros::lcd::set_text(4, "Error: " + std::to_string(error));
		 pros::lcd::set_text(5, "Output: " + std::to_string(output));
		 // clamp output to motor-compatible values
		 output = fmin(fmax(output, -maxSpeed), maxSpeed);
		 pros::lcd::set_text(6, "Clamped: " + std::to_string(output));
		 // set motors to calculated output
		 this->frontLeftMotor = output;
		 this->backLeftMotor = output;
		 this->frontRightMotor = -output;
		 this->backRightMotor = -output;
		 // record new prior values
		 error_prior = error;
		 integral_prior = integral;
		 // delay by dT
		 pros::delay(this->positionPIDdT);
	 }
}

void RobotDriver::tankDrive() {
  int left = this->controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
  int right = -this->controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_Y);
  this->frontLeftMotor = left;
  this->backLeftMotor = left;
  this->frontRightMotor = right;
  this->backRightMotor = right;
}
void RobotDriver::arcadeDrive() {
  int vertical = this->controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
  int horizontal = this->controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
  this->frontLeftMotor = horizontal - vertical;
  this->backLeftMotor = horizontal - vertical;
  this->frontRightMotor = vertical + horizontal;
  this->backRightMotor = vertical + horizontal;
}

// void RobotDriver::armButtons() {
//   int numButtons = this->armButtonPorts.size();
//   for (int i = 0; i < numButtons; i++) {
//
//   }
// }

pros::Controller *RobotDriver::getController() {
  return &(this->controller);
}