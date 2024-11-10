#include "precomp.h"
#include "Variable.h"

Framework::Data::Variable::Variable(const std::string& name, const std::string& value, const Format format) :
	mName(name),
	mValue(value),
	mFormat(format)
{
}
