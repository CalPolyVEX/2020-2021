#include "api.h"
#include "okapi/api.hpp"
#include "okapi/impl/device/motor/motorGroup.hpp"
#include <vector>
// #include "arduinoSensors.hpp"

enum DriveMode { Tank, Arcade };
enum BindMode { Toggle, Step, Hold };

class CPRobotMotor {
private:
  int port;
  pros::Motor motor;
  int dir;
public:
  CPRobotMotor(int portNum);
  int getPort();
  int getDirection();
  void setSpeed(int speed);
  void moveTo(int position, int speed);
};

class CPRobotMotorSet {
private:
  std::vector<CPRobotMotor> motors;
public:
  CPRobotMotorSet(std::initializer_list<int> ports);
  std::string listMotors();
  void setSpeed(int speed);
  void moveTo(int position, int speed);
};

class CPRobotControllerBind {
private:
  CPRobotMotorSet *motorSet;
  pros::controller_digital_e_t buttonPrimary;
  pros::controller_digital_e_t buttonSecondary;
  std::vector<int> positions;
  int numPositions;
  int positionIndex;
  bool releasedButtonPrimary;
  bool releasedButtonSecondary;
  enum BindMode bindMode;
  int speed;
  void controlCycleToggle(bool pressedPrimary);
  void controlCycleStep(bool pressedPrimary, bool pressedSecondary);
  void controlCycleHold(bool pressedPrimary, bool pressedSecondary);
public:
  CPRobotControllerBind(CPRobotMotorSet *m, pros::controller_digital_e_t bp, pros::controller_digital_e_t bs, int i, std::vector<int> p, enum BindMode bm);
  void controlCycle(pros::Controller controller);
};

class CPRobotDriver {
private:
  CPRobotMotorSet *leftMotorSet;
  CPRobotMotorSet *rightMotorSet;
  DriveMode driveMode;
  pros::Controller controller;
  std::vector<CPRobotControllerBind *> controllerBinds;
public:
  CPRobotDriver(CPRobotMotorSet &left, CPRobotMotorSet &right, DriveMode mode, std::vector<CPRobotControllerBind *> cb);
  void setSpeed(int speed);
  void controlCycle();
};


// class CPRobotDriverBuilder {
// public:
//   CPRobotDriver Build();
//   CPRobotDriverBuilder WithMotors(int[]);
// };
