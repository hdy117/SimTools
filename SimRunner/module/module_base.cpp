#include "module_base.h"

namespace ss {
	ModuleBase::ModuleBase(const std::string &name) {
		setModuleName(name);
	}
	ModuleBase::~ModuleBase() {
	
	}
	void ModuleBase::setModuleName(const std::string &name) {
		m_module_name = name;
	}
	std::string ModuleBase::getModuleName() {
		return m_module_name;
	}
}
