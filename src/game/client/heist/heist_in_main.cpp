#include "cbase.h"
#include "input.h"

class CHeistInput : public CInput
{
public:
};

static CHeistInput g_Input;
IInput *input = (IInput *)&g_Input;
