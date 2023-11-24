#include "SimCore.h"

#include <string>

namespace ss {
class ModuleBase : public ss::NoCopyable {
public:
  ModuleBase(const std::string &name = "BaseModule");
  virtual ~ModuleBase();

  virtual bool init() = 0;
  virtual bool step() = 0;
  virtual bool stop() = 0;

public:
  void setModuleName(const std::string &name);
  std::string getModuleName();

private:
  std::string m_module_name;
};
} // namespace ss