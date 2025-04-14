#pragma once

namespace ciface::ScriptInput
{
// Called once at startup to create pipe server
void Init();

// Create the ScriptInput device and add it to Dolphin
void PopulateDevices();

// Stop the server and remove the device
void DeInit();
}  // namespace ciface::ScriptInput
