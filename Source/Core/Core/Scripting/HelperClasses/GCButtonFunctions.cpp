#include <string_view>

#include "Common/StringUtil.h"

#include "Core/Scripting/CoreScriptContextFiles/Enums/GCButtonNameEnum.h"
#include "Core/Scripting/HelperClasses/GCButtonFunctions.h"

#include "string.h"

int ParseGCButton_impl(const char* button_name)
{
  if (button_name == nullptr)
    return static_cast<int>(ScriptingEnums::GcButtonNameEnum::UnknownButton);

  size_t button_string_length = strlen(button_name);

  if (button_string_length == 0)
    return static_cast<int>(ScriptingEnums::GcButtonNameEnum::UnknownButton);

  if (button_string_length == 1)
  {
    switch (button_name[0])
    {
    case 'a':
    case 'A':
      return (int)ScriptingEnums::GcButtonNameEnum::A;

    case 'b':
    case 'B':
      return (int)ScriptingEnums::GcButtonNameEnum::B;

    case 'x':
    case 'X':
      return (int)ScriptingEnums::GcButtonNameEnum::X;

    case 'y':
    case 'Y':
      return (int)ScriptingEnums::GcButtonNameEnum::Y;

    case 'z':
    case 'Z':
      return (int)ScriptingEnums::GcButtonNameEnum::Z;

    case 'l':
    case 'L':
      return (int)ScriptingEnums::GcButtonNameEnum::L;

    case 'r':
    case 'R':
      return (int)ScriptingEnums::GcButtonNameEnum::R;

    default:
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;
    }
  }

  switch (button_name[0])
  {
  case 'd':
  case 'D':
    if (Common::CaseInsensitiveEquals(button_name, "dPadUp"))
      return (int)ScriptingEnums::GcButtonNameEnum::DPadUp;

    else if (Common::CaseInsensitiveEquals(button_name, "dPadDown"))
      return (int)ScriptingEnums::GcButtonNameEnum::DPadDown;

    else if (Common::CaseInsensitiveEquals(button_name, "dPadLeft"))
      return (int)ScriptingEnums::GcButtonNameEnum::DPadLeft;

    else if (Common::CaseInsensitiveEquals(button_name, "dPadRight"))
      return (int)ScriptingEnums::GcButtonNameEnum::DPadRight;

    else if (Common::CaseInsensitiveEquals(button_name, "disc"))
      return (int)ScriptingEnums::GcButtonNameEnum::Disc;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 'a':
  case 'A':
    if (Common::CaseInsensitiveEquals(button_name, "analogStickX"))
      return (int)ScriptingEnums::GcButtonNameEnum::AnalogStickX;

    else if (Common::CaseInsensitiveEquals(button_name, "analogStickY"))
      return (int)ScriptingEnums::GcButtonNameEnum::AnalogStickY;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 'c':
  case 'C':
    if (Common::CaseInsensitiveEquals(button_name, "cStickX"))
      return (int)ScriptingEnums::GcButtonNameEnum::CStickX;

    else if (Common::CaseInsensitiveEquals(button_name, "cStickY"))
      return (int)ScriptingEnums::GcButtonNameEnum::CStickY;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 't':
  case 'T':
    if (Common::CaseInsensitiveEquals(button_name, "triggerL"))
      return (int)ScriptingEnums::GcButtonNameEnum::TriggerL;

    else if (Common::CaseInsensitiveEquals(button_name, "triggerR"))
      return (int)ScriptingEnums::GcButtonNameEnum::TriggerR;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 'r':
  case 'R':
    if (Common::CaseInsensitiveEquals(button_name, "reset"))
      return (int)ScriptingEnums::GcButtonNameEnum::Reset;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 's':
  case 'S':
    if (Common::CaseInsensitiveEquals(button_name, "start"))
      return (int)ScriptingEnums::GcButtonNameEnum::Start;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 'g':
  case 'G':
    if (Common::CaseInsensitiveEquals(button_name, "getOrigin"))
      return (int)ScriptingEnums::GcButtonNameEnum::GetOrigin;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  case 'i':
  case 'I':
    if (Common::CaseInsensitiveEquals(button_name, "isConnected"))
      return (int)ScriptingEnums::GcButtonNameEnum::IsConnected;

    else
      return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;

  default:
    return (int)ScriptingEnums::GcButtonNameEnum::UnknownButton;
  }
}

const char* ConvertButtonEnumToString_impl(int button)
{
  switch (button)
  {
  case ScriptingEnums::GcButtonNameEnum::A:
    return "A";

  case ScriptingEnums::GcButtonNameEnum::B:
    return "B";

  case ScriptingEnums::GcButtonNameEnum::X:
    return "X";

  case ScriptingEnums::GcButtonNameEnum::Y:
    return "Y";

  case ScriptingEnums::GcButtonNameEnum::Z:
    return "Z";

  case ScriptingEnums::GcButtonNameEnum::L:
    return "L";

  case ScriptingEnums::GcButtonNameEnum::R:
    return "R";

  case ScriptingEnums::GcButtonNameEnum::Start:
    return "Start";

  case ScriptingEnums::GcButtonNameEnum::Reset:
    return "Reset";

  case ScriptingEnums::GcButtonNameEnum::DPadUp:
    return "dPadUp";

  case ScriptingEnums::GcButtonNameEnum::DPadDown:
    return "dPadDown";

  case ScriptingEnums::GcButtonNameEnum::DPadLeft:
    return "dPadLeft";

  case ScriptingEnums::GcButtonNameEnum::DPadRight:
    return "dPadRight";

  case ScriptingEnums::GcButtonNameEnum::TriggerL:
    return "triggerL";

  case ScriptingEnums::GcButtonNameEnum::TriggerR:
    return "triggerR";

  case ScriptingEnums::GcButtonNameEnum::AnalogStickX:
    return "analogStickX";

  case ScriptingEnums::GcButtonNameEnum::AnalogStickY:
    return "analogStickY";

  case ScriptingEnums::GcButtonNameEnum::CStickX:
    return "cStickX";

  case ScriptingEnums::GcButtonNameEnum::CStickY:
    return "cStickY";

  case ScriptingEnums::GcButtonNameEnum::Disc:
    return "disc";

  case ScriptingEnums::GcButtonNameEnum::GetOrigin:
    return "getOrigin";

  case ScriptingEnums::GcButtonNameEnum::IsConnected:
    return "isConnected";

  default:
    return "";
  }
}

int IsValidButtonEnum_impl(int button)
{
  if (button >= 0 && button < ScriptingEnums::GcButtonNameEnum::UnknownButton)
    return 1;
  else
    return 0;
}

int IsDigitalButton_impl(int raw_button_val)
{
  ScriptingEnums::GcButtonNameEnum button_name = (ScriptingEnums::GcButtonNameEnum)raw_button_val;

  switch (button_name)
  {
  case ScriptingEnums::GcButtonNameEnum::A:
  case ScriptingEnums::GcButtonNameEnum::B:
  case ScriptingEnums::GcButtonNameEnum::Disc:
  case ScriptingEnums::GcButtonNameEnum::DPadDown:
  case ScriptingEnums::GcButtonNameEnum::DPadLeft:
  case ScriptingEnums::GcButtonNameEnum::DPadRight:
  case ScriptingEnums::GcButtonNameEnum::DPadUp:
  case ScriptingEnums::GcButtonNameEnum::GetOrigin:
  case ScriptingEnums::GcButtonNameEnum::IsConnected:
  case ScriptingEnums::GcButtonNameEnum::L:
  case ScriptingEnums::GcButtonNameEnum::R:
  case ScriptingEnums::GcButtonNameEnum::Reset:
  case ScriptingEnums::GcButtonNameEnum::Start:
  case ScriptingEnums::GcButtonNameEnum::X:
  case ScriptingEnums::GcButtonNameEnum::Y:
  case ScriptingEnums::GcButtonNameEnum::Z:
    return 1;

  default:
    return 0;
  }
}
int IsAnalogButton_impl(int raw_button_val)
{
  ScriptingEnums::GcButtonNameEnum button_name = (ScriptingEnums::GcButtonNameEnum)raw_button_val;

  switch (button_name)
  {
  case ScriptingEnums::GcButtonNameEnum::AnalogStickX:
  case ScriptingEnums::GcButtonNameEnum::AnalogStickY:
  case ScriptingEnums::GcButtonNameEnum::CStickX:
  case ScriptingEnums::GcButtonNameEnum::CStickY:
  case ScriptingEnums::GcButtonNameEnum::TriggerL:
  case ScriptingEnums::GcButtonNameEnum::TriggerR:
    return 1;

  default:
    return 0;
  }
}
