#pragma once

typedef  uint8_t uint8;
typedef  uint16_t uint16;
typedef  int8_t int16;

#include "../gpgxInstanceBase.hpp"
#include <string>
#include <vector>
#include <jaffarCommon/serializers/contiguous.hpp>
#include <jaffarCommon/deserializers/contiguous.hpp>

#define DUMMY_SIZE 1048576

extern "C"
{ 
 int state_load(unsigned char *state);
 int state_save(unsigned char *state);
 void initialize();
 void initializeVideoOutput();
 void finalizeVideoOutput();
 void loadROM(const char* filePath);
 void renderFrame();
 void advanceFrame(const uint16_t controller1, const uint16_t controller2);
 uint8_t* getWorkRamPtr();
}

namespace gpgx
{

class EmuInstance : public EmuInstanceBase
{
 public:

 uint8_t* _baseMem;
 uint8_t* _apuMem;

 EmuInstance(const nlohmann::json &config) : EmuInstanceBase(config)
 {
 }

  virtual void initialize() override
  {
    ::initialize();
  }

  virtual bool loadROMImpl(const std::string &romFilePath) override
  {
    ::loadROM(romFilePath.c_str());

    return true;
  }

  void initializeVideoOutput() override
  {
    ::initializeVideoOutput();
  }

  void finalizeVideoOutput() override
  {
    ::finalizeVideoOutput();
  }

  void enableRendering() override
  {
  }

  void disableRendering() override
  {
  }

  void serializeState(jaffarCommon::serializer::Base& s) const override
  {
    auto buffer = (uint8_t*) malloc(DUMMY_SIZE);
    auto size = ::state_save(buffer);
    s.push(buffer, size);
    free (buffer);
  }

  void deserializeState(jaffarCommon::deserializer::Base& d) override
  {
    ::state_load((unsigned char*)d.getInputDataBuffer());
  }

  size_t getStateSizeImpl() const override
  {
    auto buffer = (uint8_t*) malloc(DUMMY_SIZE);
    auto size = ::state_save(buffer);
    free (buffer);
    return size;
  }

  void updateRenderer() override
  {
    ::renderFrame();
  }

  inline size_t getDifferentialStateSizeImpl() const override { return getStateSizeImpl(); }

  void enableLiteStateBlockImpl(const std::string& block)
  {
    // Nothing to do here
  }

  void disableLiteStateBlockImpl(const std::string& block)
  {
    // Nothing to do here
  }

  void doSoftReset() override
  {
  }
  
  void doHardReset() override
  {
  }

  std::string getCoreName() const override { return "GPGX Base"; }


  virtual void advanceStateImpl(const jaffar::port_t controller1, const jaffar::port_t controller2)
  {
     ::advanceFrame(controller1, controller2);
  }

  inline uint8_t* getWorkRamPointer() const override
  {
    return getWorkRamPtr();
  }

};

} // namespace gpgx