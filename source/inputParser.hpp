#pragma once

// Base controller class
// by eien86

#include <cstdint>
#include <jaffarCommon/exceptions.hpp>
#include <jaffarCommon/json.hpp>
#include <string>
#include <sstream>
#include <input_hw/input.h>

namespace jaffar
{
  
typedef uint16_t port_t;

struct input_t
{
  bool power = false;
  bool reset = false;
  bool pause = false;
  bool previousDisc = false;
  bool nextDisc = false;
  port_t port1 = 0;
  port_t port2 = 0;
};

class InputParser
{
public:

  enum system_t { genesis, gamegear, sms, segacd, sg1000 };
  enum controller_t { none, gamepad2b, gamegear2b, gamepad3b, gamepad6b,  };

  InputParser(const nlohmann::json &config)
  {
    // Parsing system type
    {
      bool isTypeRecognized = false;
      const auto systemType = jaffarCommon::json::getString(config, "System Type");

      if (systemType == "Sega Genesis")       { _systemType = system_t::genesis; isTypeRecognized = true; }
      if (systemType == "Sega Game Gear")     { _systemType = system_t::gamegear;  isTypeRecognized = true; }
      if (systemType == "Sega Master System") { _systemType = system_t::sms; isTypeRecognized = true; }
      if (systemType == "Sega CD")            { _systemType = system_t::segacd; isTypeRecognized = true; }
      if (systemType == "Sega SG-1000")       { _systemType = system_t::sg1000; isTypeRecognized = true; }

      if (isTypeRecognized == false) JAFFAR_THROW_LOGIC("Input type not recognized: '%s'\n", systemType.c_str());
    }

    // Parsing controller 1 type
    {
      bool isTypeRecognized = false;
      const auto controller1Type = jaffarCommon::json::getString(config, "Controller 1 Type");

      if (controller1Type == "None")       { _controller1Type = controller_t::none; isTypeRecognized = true; }
      if (controller1Type == "GameGear2B") { _controller1Type = controller_t::gamegear2b;  isTypeRecognized = true; }
      if (controller1Type == "Gamepad2B")  { _controller1Type = controller_t::gamepad2b; isTypeRecognized = true; }
      if (controller1Type == "Gamepad3B")  { _controller1Type = controller_t::gamepad3b; isTypeRecognized = true; }
      if (controller1Type == "Gamepad6B")  { _controller1Type = controller_t::gamepad6b; isTypeRecognized = true; }
      
      if (isTypeRecognized == false) JAFFAR_THROW_LOGIC("Controller 1 type not recognized: '%s'\n", controller1Type.c_str()); 
   }

    // Parsing controller 2 type
    {
      bool isTypeRecognized = false;
      const auto controller2Type = jaffarCommon::json::getString(config, "Controller 2 Type");

      if (controller2Type == "None")       { _controller2Type = controller_t::none; isTypeRecognized = true; }
      if (controller2Type == "GameGear2B") { _controller2Type = controller_t::gamegear2b;  isTypeRecognized = true; }
      if (controller2Type == "Gamepad2B")  { _controller2Type = controller_t::gamepad2b; isTypeRecognized = true; }
      if (controller2Type == "Gamepad3B")  { _controller2Type = controller_t::gamepad3b; isTypeRecognized = true; }
      if (controller2Type == "Gamepad6B")  { _controller2Type = controller_t::gamepad6b; isTypeRecognized = true; }
      
      if (isTypeRecognized == false) JAFFAR_THROW_LOGIC("Controller 2 type not recognized: '%s'\n", controller2Type.c_str()); 
    }
  }

  inline input_t parseInputString(const std::string &inputString) const
  {
    // Storage for the input
    input_t input;

    // Converting input into a stream for parsing
    std::istringstream ss(inputString);

    // Start separator
    if (ss.get() != '|') reportBadInputString(inputString);

    // Parsing console inputs
    parseConsoleInputs(input, _systemType, ss, inputString);
    
    // Parsing controller 1 inputs
    parseControllerInputs(_controller1Type, input.port1, ss, inputString);

    // Parsing controller 2 inputs
    parseControllerInputs(_controller2Type, input.port2, ss, inputString);

    // End separator
    if (ss.get() != '|') reportBadInputString(inputString);

    // If its not the end of the stream, then extra values remain and its invalid
    ss.get();
    if (ss.eof() == false) reportBadInputString(inputString);
    
    // Returning input
    return input;
  };

  private:

  static inline void reportBadInputString(const std::string &inputString)
  {
    JAFFAR_THROW_LOGIC("Could not decode input string: '%s'\n", inputString.c_str());
  }

  static bool parseGamePad6BInput(uint16_t& code, std::istringstream& ss, const std::string& inputString)
  {
    // Currently read character
    char c;

    // Cleaning code
    code = 0;

    c = ss.get();
    if (c != '.' && c != 'U') return false;
    if (c == 'U') code |= INPUT_UP;

    c = ss.get();
    if (c != '.' && c != 'D') return false;
    if (c == 'D') code |= INPUT_DOWN;

    c = ss.get();
    if (c != '.' && c != 'L') return false;
    if (c == 'L') code |= INPUT_LEFT;

    c = ss.get();
    if (c != '.' && c != 'R') return false;
    if (c == 'R') code |= INPUT_RIGHT;

    c = ss.get();
    if (c != '.' && c != 'A') return false;
    if (c == 'A') code |= INPUT_A;

    c = ss.get();
    if (c != '.' && c != 'B') return false;
    if (c == 'B') code |= INPUT_B;

    c = ss.get();
    if (c != '.' && c != 'C') return false;
    if (c == 'C') code |= INPUT_C;

    c = ss.get();
    if (c != '.' && c != 'S') return false;
    if (c == 'S') code |= INPUT_START;

    c = ss.get();
    if (c != '.' && c != 'X') return false;
    if (c == 'X') code |= INPUT_X;

    c = ss.get();
    if (c != '.' && c != 'Y') return false;
    if (c == 'Y') code |= INPUT_Y;

    c = ss.get();
    if (c != '.' && c != 'Z') return false;
    if (c == 'Z') code |= INPUT_Z;

    c = ss.get();
    if (c != '.' && c != 'M') return false;
    if (c == 'M') code |= INPUT_MODE;

    return true;
  }

  static bool parseGamePad3BInput(uint16_t& code, std::istringstream& ss, const std::string& inputString)
  {
    // Currently read character
    char c;

    // Cleaning code
    code = 0;

    c = ss.get();
    if (c != '.' && c != 'U') return false;
    if (c == 'U') code |= INPUT_UP;

    c = ss.get();
    if (c != '.' && c != 'D') return false;
    if (c == 'D') code |= INPUT_DOWN;

    c = ss.get();
    if (c != '.' && c != 'L') return false;
    if (c == 'L') code |= INPUT_LEFT;

    c = ss.get();
    if (c != '.' && c != 'R') return false;
    if (c == 'R') code |= INPUT_RIGHT;

    c = ss.get();
    if (c != '.' && c != 'A') return false;
    if (c == 'A') code |= INPUT_A;

    c = ss.get();
    if (c != '.' && c != 'B') return false;
    if (c == 'B') code |= INPUT_B;

    c = ss.get();
    if (c != '.' && c != 'C') return false;
    if (c == 'C') code |= INPUT_C;

    c = ss.get();
    if (c != '.' && c != 'S') return false;
    if (c == 'S') code |= INPUT_START;


    return true;
  }


  static bool parseGameGearInput(uint16_t& code, std::istringstream& ss, const std::string& inputString)
  {
    // Currently read character
    char c;

    // Cleaning code
    code = 0;

    c = ss.get();
    if (c != '.' && c != 'U') return false;
    if (c == 'U') code |= INPUT_UP;

    c = ss.get();
    if (c != '.' && c != 'D') return false;
    if (c == 'D') code |= INPUT_DOWN;

    c = ss.get();
    if (c != '.' && c != 'L') return false;
    if (c == 'L') code |= INPUT_LEFT;

    c = ss.get();
    if (c != '.' && c != 'R') return false;
    if (c == 'R') code |= INPUT_RIGHT;

    c = ss.get();
    if (c != '.' && c != '1') return false;
    if (c == '1') code |= INPUT_BUTTON1;

    c = ss.get();
    if (c != '.' && c != '2') return false;
    if (c == '2') code |= INPUT_BUTTON2;

    c = ss.get();
    if (c != '.' && c != 'S') return false;
    if (c == 'S') code |= INPUT_START;


    return true;
  }

  static bool parseGamePad2BInput(uint16_t& code, std::istringstream& ss, const std::string& inputString)
  {
    // Currently read character
    char c;

    // Cleaning code
    code = 0;

    c = ss.get();
    if (c != '.' && c != 'U') return false;
    if (c == 'U') code |= INPUT_UP;

    c = ss.get();
    if (c != '.' && c != 'D') return false;
    if (c == 'D') code |= INPUT_DOWN;

    c = ss.get();
    if (c != '.' && c != 'L') return false;
    if (c == 'L') code |= INPUT_LEFT;

    c = ss.get();
    if (c != '.' && c != 'R') return false;
    if (c == 'R') code |= INPUT_RIGHT;

    c = ss.get();
    if (c != '.' && c != '1') return false;
    if (c == '1') code |= INPUT_BUTTON1;

    c = ss.get();
    if (c != '.' && c != '2') return false;
    if (c == '2') code |= INPUT_BUTTON2;

    return true;
  }

  static void parseControllerInputs(const controller_t type, port_t& port, std::istringstream& ss, const std::string& inputString)
  {
    // If no controller assigned then, its port is all zeroes.
    if (type == controller_t::none) { port = 0; return; }

    // Controller separator
    if (ss.get() != '|') reportBadInputString(inputString);


    // If game gear input, parse 2 buttons and start
    if (type == controller_t::gamegear2b) 
    {
      // Storage for gamepad's code
      uint16_t code = 0;

      // Parsing gamepad code
      parseGameGearInput(code, ss, inputString);

      // Pushing input code into the port
      port = code;
    }


    // If its sms input, parse two buttons
    if (type == controller_t::gamepad2b) 
    {
      // Storage for gamepad's code
      uint16_t code = 0;

      // Parsing gamepad code
      parseGamePad2BInput(code, ss, inputString);

      // Pushing input code into the port
      port = code;
    }

    // If normal genesis, parse 3 buttons
    if (type == controller_t::gamepad3b) 
    {
      // Storage for gamepad's code
      uint16_t code = 0;

      // Parsing gamepad code
      parseGamePad3BInput(code, ss, inputString);

      // Pushing input code into the port
      port = code;
    }

    // If 6b genesis, parse its code now
    if (type == controller_t::gamepad6b) 
    {
      // Storage for gamepad's code
      uint16_t code = 0;

      // Parsing gamepad code
      parseGamePad6BInput(code, ss, inputString);

      // Pushing input code into the port
      port = code;
    }
  }

  static void parseConsoleInputs(input_t& input, const system_t type, std::istringstream& ss, const std::string& inputString)
  {
    // Currently read character
    char c;

    c = ss.get();
    if (c != '.' && c != 'P') reportBadInputString(inputString);
    if (c == 'P') input.power = true;
    if (c == '.') input.power = false;

    c = ss.get();
    if (c != '.' && c != 'r') reportBadInputString(inputString);
    if (c == 'r') input.reset = true;
    if (c == '.') input.reset = false;

    // If its segacd, parse power, reset, and disc selection buttons
    if (type == system_t::segacd)
    {
      c = ss.get();
      if (c != '.' && c != '<') reportBadInputString(inputString);
      if (c == '<') input.previousDisc = true;
      if (c == '.') input.previousDisc = false;

      c = ss.get();
      if (c != '.' && c != '>') reportBadInputString(inputString);
      if (c == '>') input.nextDisc = true;
      if (c == '.') input.nextDisc = false;
    }
  }

  input_t _input;
  system_t _systemType;
  controller_t _controller1Type;
  controller_t _controller2Type;
}; // class InputParser

} // namespace jaffar