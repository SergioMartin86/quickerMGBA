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

  std::string getCoreName() const override { return "QuickerMGBA"; }

  private:


};

} // namespace mgba