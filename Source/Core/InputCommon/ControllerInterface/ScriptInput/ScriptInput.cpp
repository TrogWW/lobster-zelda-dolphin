/***************************************************************************************
 * ScriptInput.cpp
 *
 * This file implements a custom input backend called ScriptInput. It uses a separate
 * PipeServer (implemented in PipeServer.{h,cpp}) to accept named pipe connections from
 * an external client (e.g. C#). It then reads binary GCPadStatus structures (12 bytes)
 * from the pipe every frame. The ScriptPipeDevice exposes sub-objects (buttons, analog
 * axes, and triggers) that Dolphin maps via its Controller Configuration.
 *
 * Ensure that the names used below exactly match your GCPadNew.ini bindings.
 ***************************************************************************************/

#include "InputCommon/ControllerInterface/ScriptInput/ScriptInput.h"
#include <windows.h>
#include <memory>
#include <vector>

#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/ControllerInterface/CoreDevice.h"  // For ciface::Core::Device, DeviceRemoval, etc.
#include "InputCommon/ControllerInterface/ScriptInput/PipeServer.h"

// Adjust the include path below as needed for your GCPadStatus definition.
#include <InputCommon/GCPadStatus.h>
#include "Core/System.h"
// #include "Common/Logging/Log.h"  // Uncomment if you have logging; otherwise, you can use printf.

using namespace ScriptInput;  // Use our ScriptInput namespace

// 1) A static pointer to the pipe server instance.
static std::unique_ptr<PipeServer> s_server;

// 2) Define our minimal custom device.
// We create both digital inputs (buttons) and analog inputs (for sticks and triggers).
namespace
{
// Digital Button sub-object
class Button final : public ciface::Core::Device::Input
{
public:
  Button(const GCPadStatus& pad_ref, const std::string& name, u16 mask)
      : m_pad(pad_ref), m_name(name), m_mask(mask)
  {
  }

  std::string GetName() const override { return m_name; }
  ControlState GetState() const override { return (m_pad.button & m_mask) ? 1.0 : 0.0; }

private:
  const GCPadStatus& m_pad;
  const std::string m_name;
  const u16 m_mask;
};

// Analog Trigger sub-object: returns a value from 0 to 1.
class Trigger final : public ciface::Core::Device::Input
{
public:
  Trigger(const u8& ref, const std::string& name) : m_ref(ref), m_name(name) {}

  std::string GetName() const override { return m_name; }
  ControlState GetState() const override { return static_cast<float>(m_ref) / 255.0f; }

private:
  const u8& m_ref;
  const std::string m_name;
};

// Analog Axis sub-object for the positive direction: returns [0,1] when deflection is above center.
class AxisPositive final : public ciface::Core::Device::Input
{
public:
  // invert: if true, raw value is flipped (useful for some axes)
  AxisPositive(const u8& ref, const std::string& name, bool invert)
      : m_ref(ref), m_name(name), m_invert(invert)
  {
  }

  std::string GetName() const override { return m_name; }
  ControlState GetState() const override
  {
    int raw = static_cast<int>(m_ref);
    if (m_invert)
      raw = 255 - raw;
    if (raw <= 128)
      return 0.0f;
    float scaled = (raw - 128) / 127.0f;
    if (scaled > 1.0f)
      scaled = 1.0f;
    return scaled;
  }

private:
  const u8& m_ref;
  std::string m_name;
  bool m_invert;
};

// Analog Axis sub-object for the negative direction: returns [0,1] when deflection is below center.
class AxisNegative final : public ciface::Core::Device::Input
{
public:
  AxisNegative(const u8& ref, const std::string& name, bool invert)
      : m_ref(ref), m_name(name), m_invert(invert)
  {
  }

  std::string GetName() const override { return m_name; }
  ControlState GetState() const override
  {
    int raw = static_cast<int>(m_ref);
    if (m_invert)
      raw = 255 - raw;
    if (raw >= 128)
      return 0.0f;
    float scaled = (128 - raw) / 128.0f;
    if (scaled > 1.0f)
      scaled = 1.0f;
    return scaled;
  }

private:
  const u8& m_ref;
  std::string m_name;
  bool m_invert;
};

// Our main device class.
class ScriptPipeDevice final : public ciface::Core::Device
{
public:
  ScriptPipeDevice();

  std::string GetName() const override { return "ScriptPipeDevice"; }
  std::string GetSource() const override { return "ScriptInput"; }
  ciface::Core::DeviceRemoval UpdateInput() override;
  std::optional<int> GetPreferredId() const override { return 0; }

private:
  // The current controller state; updated each frame with new data from the pipe.
  GCPadStatus m_pad{};
};

ScriptPipeDevice::ScriptPipeDevice()
{
  // Digital buttons (names must match the names in your GCPadNew.ini bindings).
  AddInput(new Button(m_pad, "A", 0x0100));
  AddInput(new Button(m_pad, "B", 0x0200));
  AddInput(new Button(m_pad, "X", 0x0400));
  AddInput(new Button(m_pad, "Y", 0x0800));
  AddInput(new Button(m_pad, "Z", 0x0010));
  AddInput(new Button(m_pad, "Start", 0x1000));
  AddInput(new Button(m_pad, "Left", 0x0001));
  AddInput(new Button(m_pad, "Right", 0x0002));
  AddInput(new Button(m_pad, "Down", 0x0004));
  AddInput(new Button(m_pad, "Up", 0x0008));
  AddInput(new Button(m_pad, "L", 0x0040));
  AddInput(new Button(m_pad, "R", 0x0020));

  // Analog triggers:
  AddInput(new Trigger(m_pad.triggerLeft, "Axis Trigger L"));
  AddInput(new Trigger(m_pad.triggerRight, "Axis Trigger R"));

  // For the main stick, we now create four separate sub-objects:
  AddInput(new AxisPositive(m_pad.stickX, "Axis Main Stick X+", false));
  AddInput(new AxisNegative(m_pad.stickX, "Axis Main Stick X-", false));

  // Update: We want the Up/Down values inverted relative to our current output.
  // Instead of inverting (true), set invert to false for the Y axes.
  AddInput(new AxisPositive(m_pad.stickY, "Axis Main Stick Y+", false));
  AddInput(new AxisNegative(m_pad.stickY, "Axis Main Stick Y-", false));

  // For the C-stick, similarly create four separate sub-objects:
  AddInput(new AxisPositive(m_pad.substickX, "Axis C-Stick X+", false));
  AddInput(new AxisNegative(m_pad.substickX, "Axis C-Stick X-", false));
  AddInput(new AxisPositive(m_pad.substickY, "Axis C-Stick Y+", false));
  AddInput(new AxisNegative(m_pad.substickY, "Axis C-Stick Y-", false));
}

ciface::Core::DeviceRemoval ScriptPipeDevice::UpdateInput()
{
  if (!s_server || !s_server->IsConnected())
    return ciface::Core::DeviceRemoval::Keep;

  // Directly assign the most recent status from the server.
  m_pad = s_server->GetLatestPadStatus();

  // If necessary, you can add additional processing here.

  return ciface::Core::DeviceRemoval::Keep;
}

static bool s_inited = false;
static bool s_populated = false;
static std::vector<std::shared_ptr<ScriptPipeDevice>> s_devices;
}  // end anonymous namespace

namespace ciface::ScriptInput
{
void Init()
{
  s_server = std::make_unique<PipeServer>();
  s_server->Start();
  s_inited = true;
}

void PopulateDevices()
{
  if (!s_inited)// || s_populated)
    return;

  g_controller_interface.RemoveDevice([](auto* dev) { return dev->GetSource() == "ScriptInput"; });

  auto dev = std::make_shared<ScriptPipeDevice>();
  s_devices.push_back(dev);
  g_controller_interface.AddDevice(dev);

  s_populated = true;
}

void DeInit()
{
  if (s_server)
  {
    s_server->Stop();
    s_server.reset();
  }
  s_devices.clear();
  s_populated = false;
  s_inited = false;
}
}  // namespace ciface::ScriptInput
