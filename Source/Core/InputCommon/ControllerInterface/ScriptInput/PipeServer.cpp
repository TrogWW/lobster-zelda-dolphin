#include "PipeServer.h"
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <thread>

// Include Dolphin's state and system headers (adjust paths as needed)
#include "Core/System.h"
#include <Core/State.h>
#include <Core/Core.h>
#include <VideoCommon/Present.h>
#include "Core/HW/Memmap.h"

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
    // Capture a snapshot of m_pipeHandle to use for the response.
    HANDLE pipe = m_pipeHandle;
    Core::QueueHostJob(
        [pipe](Core::System& system) {
          // Perform the frame step operation.
          Core::SetState(system, Core::State::Paused);

          // Send a response back to the client (0 for success)
          if (pipe != INVALID_HANDLE_VALUE)
          {
            BYTE response = 0;
            DWORD bytesWritten = 0;
            WriteFile(pipe, &response, 1, &bytesWritten, nullptr);
          }
        },
        /* run_during_stop = */ false);
    return true;
  }
  case 4:
  {
    // Capture a snapshot of m_pipeHandle to use for the response.
    HANDLE pipe = m_pipeHandle;
    Core::QueueHostJob(
        [pipe](Core::System& system) {
          // Perform the frame step operation.
          Core::SetState(system, Core::State::Running);

          // Send a response back to the client (0 for success)
          if (pipe != INVALID_HANDLE_VALUE)
          {
            BYTE response = 0;
            DWORD bytesWritten = 0;
            WriteFile(pipe, &response, 1, &bytesWritten, nullptr);
          }
        },
        /* run_during_stop = */ false);
    return true;
  }
  case 5:
  {
    // Capture a snapshot of m_pipeHandle to use for the response.
    HANDLE pipe = m_pipeHandle;
    Core::QueueHostJob(
        [pipe](Core::System& system) {
          // Perform the frame step operation.
          Core::DoFrameStep(system);

          // Send a response back to the client (0 for success)
          if (pipe != INVALID_HANDLE_VALUE)
          {
            BYTE response = 0;
            DWORD bytesWritten = 0;
            WriteFile(pipe, &response, 1, &bytesWritten, nullptr);
          }
        },
        /* run_during_stop = */ false);
    return true;
  }
  case 6: //Get Frame Count
  {
    // Capture a snapshot of m_pipeHandle to use for the response.
    HANDLE pipe = m_pipeHandle;
    Core::QueueHostJob(
        [pipe](Core::System& system) {
          DWORD bytesWritten = 0;
          if (g_presenter)
          {
            int frameCount = g_presenter->FrameCount();
            WriteFile(pipe, &frameCount, sizeof(frameCount), &bytesWritten, nullptr);
          }
          else
          {
            BYTE errorResponse = 0xFF;
            WriteFile(pipe, &errorResponse, 1, &bytesWritten, nullptr);
          }
        },
        /* run_during_stop = */ false);

    return true;
  }
  case 7: //Read value
  {
    return HandleMemoryRead();
  }
  case 8:
  {
    return HandleMemoryWrite();
  }
  default:
    // Unknown command - for now, ignore it.
    return false;
  }

}
bool PipeServer::HandleMemoryRead()
{
  DWORD bytesRead = 0, bytesWritten = 0;

  // Read the next byte for the memory read type.
  BYTE readType = 0;
  if (!ReadFile(m_pipeHandle, &readType, 1, &bytesRead, nullptr) || bytesRead != 1)
    return false;

  // Read the next 4 bytes as a u32 address.
  DWORD address = 0;
  if (!ReadFile(m_pipeHandle, &address, sizeof(address), &bytesRead, nullptr) || bytesRead != sizeof(address))
    return false;

  // Get a reference to the running system's memory manager.
  Core::System& system = Core::System::GetInstance();
  Memory::MemoryManager& memory = system.GetMemory();

  // Dispatch based on the read type.
  switch (readType)
  {
    case 0: // READ_U8
    {
      u8 value = memory.Read_U8(address);
      WriteFile(m_pipeHandle, &value, sizeof(value), &bytesWritten, nullptr);
      break;
    }
    case 1: // READ_U16
    {
      u16 value = memory.Read_U16(address);
      WriteFile(m_pipeHandle, &value, sizeof(value), &bytesWritten, nullptr);
      break;
    }
    case 2: // READ_U32
    {
      u32 value = memory.Read_U32(address);
      WriteFile(m_pipeHandle, &value, sizeof(value), &bytesWritten, nullptr);
      break;
    }
    case 3: // READ_F32
    {
      float value = memory.Read_F32(address);
      WriteFile(m_pipeHandle, &value, sizeof(value), &bytesWritten, nullptr);
      break;
    }
    case 4: // READ_U64
    {
      u64 value = memory.Read_U64(address);
      WriteFile(m_pipeHandle, &value, sizeof(value), &bytesWritten, nullptr);
      break;
    }
    case 5: // READ_STRING
    {
      // For strings, read an additional 4-byte integer which specifies the number of characters to read.
      int count = 0;
      if (!ReadFile(m_pipeHandle, &count, sizeof(count), &bytesRead, nullptr) || bytesRead != sizeof(count))
        return false;
      std::string value = memory.Read_String(address, count);
      // First send the string length.
      int len = static_cast<int>(value.size());
      WriteFile(m_pipeHandle, &len, sizeof(len), &bytesWritten, nullptr);
      // Then send the string data (if any).
      if (len > 0)
        WriteFile(m_pipeHandle, value.data(), len, &bytesWritten, nullptr);
      break;
    }
    default:
      // Unknown memory read type.
      return false;
  }

  return true;
}
/// Handle memory write (command 8).
bool PipeServer::HandleMemoryWrite()
{
  DWORD bytesRead = 0, bytesWritten = 0;

  // Read the write type (1 byte).
  BYTE writeType = 0;
  if (!ReadFile(m_pipeHandle, &writeType, 1, &bytesRead, nullptr) || bytesRead != 1)
    return false;

  // Read the next 4 bytes as a u32 address.
  u32 address = 0;
  if (!ReadFile(m_pipeHandle, &address, sizeof(address), &bytesRead, nullptr) ||
      bytesRead != sizeof(address))
    return false;

  Core::System& system = Core::System::GetInstance();
  Memory::MemoryManager& memory = system.GetMemory();

  bool success = false;
  switch (writeType)
  {
  case 0:  // Write_U8
  {
    u8 value = 0;
    if (!ReadFile(m_pipeHandle, &value, sizeof(value), &bytesRead, nullptr) ||
        bytesRead != sizeof(value))
      return false;
    memory.Write_U8(value, address);
    success = true;
    break;
  }
  case 1:  // Write_U16
  {
    u16 value = 0;
    if (!ReadFile(m_pipeHandle, &value, sizeof(value), &bytesRead, nullptr) ||
        bytesRead != sizeof(value))
      return false;
    memory.Write_U16(value, address);
    success = true;
    break;
  }
  case 2:  // Write_U32
  {
    u32 value = 0;
    if (!ReadFile(m_pipeHandle, &value, sizeof(value), &bytesRead, nullptr) ||
        bytesRead != sizeof(value))
      return false;
    memory.Write_U32(value, address);
    success = true;
    break;
  }
  case 3:  // Write_F32
  {
    float value = 0.0f;
    if (!ReadFile(m_pipeHandle, &value, sizeof(value), &bytesRead, nullptr) ||
        bytesRead != sizeof(value))
      return false;
    memory.Write_F32(address, value);
    success = true;
    break;
  }
  case 4:  // Write_U64
  {
    u64 value = 0;
    if (!ReadFile(m_pipeHandle, &value, sizeof(value), &bytesRead, nullptr) ||
        bytesRead != sizeof(value))
      return false;
    memory.Write_U64(value, address);
    success = true;
    break;
  }
  default:
    success = false;
    break;
  }

  // Send an acknowledgement back to the client: 0 = success, 0xFF = error.
  BYTE response = success ? 0 : 0xFF;
  WriteFile(m_pipeHandle, &response, 1, &bytesWritten, nullptr);

  return success;
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
    BYTE errorResponse = 0xFF;
    WriteFile(m_pipeHandle, &errorResponse, 1, &bytesRead, nullptr);
    return false;
  }
  m_latestPadStatus = tempStatus;
  BYTE response = 0;
  WriteFile(m_pipeHandle, &response, 1, &bytesRead, nullptr);
  return true;
}

GCPadStatus PipeServer::GetLatestPadStatus() const
{
  return m_latestPadStatus;
}

}  // namespace ScriptInput
