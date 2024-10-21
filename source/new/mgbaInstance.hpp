#pragma once

#include "../gpgxInstanceBase.hpp"
#include <string>
#include <vector>
#include <jaffarCommon/exceptions.hpp>
#include <jaffarCommon/json.hpp>
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
 void enableVRAMBlock();
 void disableVRAMBlock();
 void enableSATMBlock();
 void disableSATMBlock();
 void setWorkRamSerializationSize(const size_t size);
 void setBiosFile(const char* biosFile);
 uint8_t* getVideoRamPtr();
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
  _dummyBuffer = (uint8_t*) malloc(DUMMY_SIZE);
  _biosFilePath = jaffarCommon::json::getString(config, "Bios File Path");
 }

 ~EmuInstance()
 {
  free(_dummyBuffer);
 }

  virtual void initialize() override
  {
    ::initialize();
    if (_biosFilePath != "") setBiosFile(_biosFilePath.c_str());
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
    auto size = ::state_save(_dummyBuffer);
    s.push(_dummyBuffer, size);
  }

  void deserializeState(jaffarCommon::deserializer::Base& d) override
  {
    // d.pop(_dummyBuffer, _stateSize);
    // ::state_load(_dummyBuffer);

    int size = ::state_load((unsigned char*)d.getInputDataBuffer());
    d.popContiguous(nullptr, size);
  }

  size_t getStateSizeImpl() const override
  {
    auto size = ::state_save(_dummyBuffer);
    return size;
  }

  void updateRenderer() override
  {
    ::renderFrame();
  }

  inline size_t getDifferentialStateSizeImpl() const override { return 0; }

  void setWorkRamSerializationSizeImpl(const size_t size) override
  {
    ::setWorkRamSerializationSize(size);
  }

  void enableStateBlockImpl(const std::string& block) override
  {
    if (block == "VRAM") { ::enableVRAMBlock(); return; }
    if (block == "SATM") { ::enableSATMBlock(); return; }

    JAFFAR_THROW_LOGIC("State block name: '%s' not found.", block.c_str());
  }

  void disableStateBlockImpl(const std::string& block) override
  {
    if (block == "VRAM") { ::disableVRAMBlock(); return; }
    if (block == "SATM") { ::disableSATMBlock(); return; }

    JAFFAR_THROW_LOGIC("State block name: '%s' not found", block.c_str());
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

  inline uint8_t* getVideoRamPointer() const override
  {
    return getVideoRamPtr();
  }

  inline uint8_t* getWorkRamPointer() const override
  {
    return getWorkRamPtr();
  }

  inline size_t getWorkRamSize() const 
  {
    return 0x10000;
  }

  private:

  uint8_t* _dummyBuffer;
  std::string _biosFilePath;

};

} // namespace gpgx