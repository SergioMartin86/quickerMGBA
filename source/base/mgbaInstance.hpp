#pragma once

#include "../mgbaInstanceBase.hpp"
#include <string>
#include <vector>
#include <jaffarCommon/exceptions.hpp>
#include <jaffarCommon/json.hpp>
#include <jaffarCommon/serializers/contiguous.hpp>
#include <jaffarCommon/deserializers/contiguous.hpp>

namespace mgba
{

class EmuInstance : public EmuInstanceBase
{
 public:


 EmuInstance(const nlohmann::json &config) : EmuInstanceBase(config)
 {
 }

 ~EmuInstance()
 {
 }

  virtual void initialize() override
  {
  }

  virtual bool loadROMImpl(const std::string &romFilePath) override
  {
    return true;
  }

  void initializeVideoOutput() override
  {
  }

  void finalizeVideoOutput() override
  {
  }

  void enableRendering() override
  {
  }

  void disableRendering() override
  {
  }

  void serializeState(jaffarCommon::serializer::Base& s) const override
  {
    // auto size = ::state_save(_dummyBuffer);
    // s.push(_dummyBuffer, size);
  }

  void deserializeState(jaffarCommon::deserializer::Base& d) override
  {
    // d.pop(_dummyBuffer, _stateSize);
    // ::state_load(_dummyBuffer);
  }

  size_t getStateSizeImpl() const override
  {
    return 0;
  }

  void updateRenderer() override
  {
  }

  inline size_t getDifferentialStateSizeImpl() const override { return 0; }

  void setWorkRamSerializationSizeImpl(const size_t size) override
  {
  }

  void enableStateBlockImpl(const std::string& block) override
  {
    // if (block == "VRAM") { ::enableVRAMBlock(); return; }
    // if (block == "SATM") { ::enableSATMBlock(); return; }

    // JAFFAR_THROW_LOGIC("State block name: '%s' not found.", block.c_str());
  }

  void disableStateBlockImpl(const std::string& block) override
  {
    // if (block == "VRAM") { ::disableVRAMBlock(); return; }
    // if (block == "SATM") { ::disableSATMBlock(); return; }

    // JAFFAR_THROW_LOGIC("State block name: '%s' not found", block.c_str());
  }

  void doSoftReset() override
  {
  }
  
  void doHardReset() override
  {
  }

  std::string getCoreName() const override { return "MGBA Base"; }


  virtual void advanceStateImpl(const jaffar::port_t controller1, const jaffar::port_t controller2)
  {
  }

  inline uint8_t* getVideoRamPointer() const override
  {
    return nullptr;
  }

  inline uint8_t* getWorkRamPointer() const override
  {
    return nullptr;
  }

  inline size_t getWorkRamSize() const 
  {
    return 0;
  }

  private:


};

} // namespace mgba