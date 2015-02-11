#include "LifeSpan.h"

const char *LifeSpan::BR_NAMES[];
const char *LifeSpan::DR_NAMES[];

namespace __LifeSpan
{
	class ModuleInit
	{
	public:
		ModuleInit()
		{
#define BIRTH_TYPE(TYPE) LifeSpan::BR_NAMES[LifeSpan::BR_##TYPE] = #TYPE;
			BIRTH_METAENUM();
#undef BIRTH_TYPE

#define DEATH_TYPE(TYPE) LifeSpan::DR_NAMES[LifeSpan::DR_##TYPE] = #TYPE;
			DEATH_METAENUM();
#undef DEATH_TYPE
		}
	} module_init;
}

LifeSpan::LifeSpan()
{
	birth.step = death.step = -1;
	birth.reason = BR_INVALID;
	death.reason = DR_INVALID;
}

void LifeSpan::set_birth( long step,
						  BirthReason reason )
{
	birth.step = step;
	birth.reason = reason;
}

void LifeSpan::set_death( long step,
						  DeathReason reason )
{
	death.step = step;
	death.reason = reason;
}
