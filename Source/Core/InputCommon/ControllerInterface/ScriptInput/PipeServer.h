#pragma once

#include <windows.h>
#include <InputCommon/GCPadStatus.h>
#include <atomic>
#include <thread>

// The PipeServer runs a thread that repeatedly creates a named pipe
// (\\.\pipe\DolphinMemoryTransfer), waits for clients, and disconnects them.

namespace ScriptInput
{
class PipeServer
{
public:
  PipeServer();
  ~PipeServer();

  // True if a client is currently connected.
  bool IsConnected() const { return m_connected; }

  // Retrieve the most recent GCPadStatus.
  GCPadStatus GetLatestPadStatus() const;

  // The named pipe handle to read from.
  HANDLE GetPipeHandle() const { return m_pipeHandle; }

  // Start/stop the server thread.
  void Start();
  void Stop();

private:
  void ServerLoop();

  // Processes a single message from the pipe.
  bool ProcessMessage();

  bool HandleMemoryRead();

  bool HandleMemoryWrite();

  bool HandleSaveState();

  bool HandleLoadState();

  // If the message command indicates a GCPadStatus update (command 0),
  // read the following bytes into the latest pad status.
  bool UpdateLatestPadStatus();

  std::atomic<bool> m_stop{false};
  std::atomic<bool> m_connected{false};
  std::thread m_thread;
  HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;
  // Holds the most recent GCPadStatus received.
  GCPadStatus m_latestPadStatus{};
};
}  // namespace ScriptInput
