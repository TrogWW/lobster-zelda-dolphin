#include "PipeServer.h"
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <thread>

// Include Dolphin's state and system headers (adjust paths as needed)
#include "Core/System.h"
#include <Core/State.h>
#include <Core/Core.h>

namespace ScriptInput
{

PipeServer::PipeServer() : m_pipeHandle(INVALID_HANDLE_VALUE), m_stop(false), m_connected(false)
{
  // Initialize m_latestPadStatus to zeros.
  ZeroMemory(&m_latestPadStatus, sizeof(m_latestPadStatus));
}

PipeServer::~PipeServer()
{
  Stop();
}

void PipeServer::Start()
{
  m_stop = false;
  m_thread = std::thread(&PipeServer::ServerLoop, this);
}

void PipeServer::Stop()
{
  m_stop = true;
  // Force-close the pipe if open so that any blocking calls unblock.
  if (m_pipeHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_pipeHandle);
    m_pipeHandle = INVALID_HANDLE_VALUE;
  }
  if (m_thread.joinable())
    m_thread.join();
  m_connected = false;
}

void PipeServer::ServerLoop()
{
  while (!m_stop)
  {
    // Create a new pipe instance with duplex access (so we can read & write)
    HANDLE pipe = CreateNamedPipeA(R"(\\.\pipe\DolphinMemoryTransfer)",
                                   PIPE_ACCESS_DUPLEX,  // changed to duplex access
                                   PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                   1,         // maximum instances
                                   0,         // output buffer size
                                   0,         // input buffer size
                                   0,         // default timeout
                                   nullptr);  // default security attributes

    if (pipe == INVALID_HANDLE_VALUE)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }

    // Wait for a client to connect.
    BOOL connected =
        ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected)
    {
      CloseHandle(pipe);
      continue;
    }

    // A client connected.
    m_pipeHandle = pipe;
    m_connected = true;

    // Monitor and process incoming messages until disconnect or stop.
    while (!m_stop)
    {
      DWORD bytesAvailable = 0;
      BOOL ok = PeekNamedPipe(m_pipeHandle, nullptr, 0, nullptr, &bytesAvailable, nullptr);
      if (!ok)
      {
        // Disconnection or error.
        break;
      }

      // Process messages if any bytes are waiting.
      while (bytesAvailable > 0)
      {
        if (!ProcessMessage())
        {
          // If processing the message fails, break out of the inner loop.
          break;
        }

        // Recalculate how many bytes are waiting.
        if (!PeekNamedPipe(m_pipeHandle, nullptr, 0, nullptr, &bytesAvailable, nullptr))
          break;
      }

      // For increased responsiveness, a short sleep is used (adjust as needed).
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup upon disconnection.
    if (m_pipeHandle != INVALID_HANDLE_VALUE)
    {
      DisconnectNamedPipe(m_pipeHandle);
      CloseHandle(m_pipeHandle);
      m_pipeHandle = INVALID_HANDLE_VALUE;
    }
    m_connected = false;
  }
}

/// ProcessMessage handles a single complete message from the pipe.
/// It reads a command byte, then dispatches according to the command.
bool PipeServer::ProcessMessage()
{
  BYTE command = 0;
  DWORD bytesRead = 0;
  if (!ReadFile(m_pipeHandle, &command, 1, &bytesRead, nullptr) || bytesRead != 1)
    return false;

  switch (command)
  {
  case 0:
    // Command 0: The following bytes represent a GCPadStatus.
    return UpdateLatestPadStatus();
  case 1:
  {
    return HandleLoadState();
  }
  case 2:
  {
    return HandleSaveState();
  }
  case 3:
  {
    Core::System& system = Core::System::GetInstance();
    Core::SetState(system, Core::State::Paused);
    BYTE response = 0;
    WriteFile(m_pipeHandle, &response, 1, &bytesRead, nullptr);
    return true;
  }
  case 4:
  {
    Core::System& system = Core::System::GetInstance();
    Core::SetState(system, Core::State::Running);
    BYTE response = 0;
    WriteFile(m_pipeHandle, &response, 1, &bytesRead, nullptr);
    return true;
  }
  default:
    // Unknown command - for now, ignore it.
    return false;
  }
}
bool PipeServer::HandleSaveState()
{
  DWORD bytesRead = 0;
  BYTE slotByte = 0;

  // Read the next byte representing the slot.
  if (!ReadFile(m_pipeHandle, &slotByte, 1, &bytesRead, nullptr) || bytesRead != 1)
    return false;

  int slot = static_cast<int>(slotByte);

  // Verify that the slot is in the valid range (e.g., 0-9).
  if (slot < 0 || slot > 9)
  {
    // Send an error response (e.g. 0xFF indicates an invalid slot).
    BYTE errorResponse = 0xFF;
    WriteFile(m_pipeHandle, &errorResponse, 1, &bytesRead, nullptr);
    return false;
  }

  // Load the save state.
  // Assumes that Core::System::GetInstance() returns the running system instance.
  Core::System& system = Core::System::GetInstance();
  State::Save(system, slot,false);
  
  // Send a success response back to the client (0 indicates success).
  BYTE response = 0;
  WriteFile(m_pipeHandle, &response, 1, &bytesRead, nullptr);

  return true;
}
bool PipeServer::HandleLoadState()
{
  DWORD bytesRead = 0;
  BYTE slotByte = 0;

  // Read the next byte representing the slot.
  if (!ReadFile(m_pipeHandle, &slotByte, 1, &bytesRead, nullptr) || bytesRead != 1)
    return false;

  int slot = static_cast<int>(slotByte);

  // Verify that the slot is in the valid range (e.g., 0-9).
  if (slot < 0 || slot > 9)
  {
    // Send an error response (e.g. 0xFF indicates an invalid slot).
    BYTE errorResponse = 0xFF;
    WriteFile(m_pipeHandle, &errorResponse, 1, &bytesRead, nullptr);
    return false;
  }

  // Load the save state.
  // Assumes that Core::System::GetInstance() returns the running system instance.
  Core::System& system = Core::System::GetInstance();
  State::Load(system, slot);

  // Send a success response back to the client (0 indicates success).
  BYTE response = 0;
  WriteFile(m_pipeHandle, &response, 1, &bytesRead, nullptr);

  return true;
}

/// UpdateLatestPadStatus reads a GCPadStatus object from the pipe and updates m_latestPadStatus.
bool PipeServer::UpdateLatestPadStatus()
{
  GCPadStatus tempStatus{};
  DWORD bytesRead = 0;
  if (!ReadFile(m_pipeHandle, &tempStatus, sizeof(tempStatus), &bytesRead, nullptr) ||
      bytesRead != sizeof(tempStatus))
  {
    return false;
  }
  m_latestPadStatus = tempStatus;
  return true;
}

GCPadStatus PipeServer::GetLatestPadStatus() const
{
  return m_latestPadStatus;
}

}  // namespace ScriptInput
