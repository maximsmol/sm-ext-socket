#include "Extension.h"

#include <string>
#include <boost/asio.hpp>

#include "CallbackHandler.h"
#include "Callback.h"
#include "Socket.h"

using namespace boost::asio::ip;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
// todo: clean this up
Extension extension;
#pragma clang diagnostic pop
SMEXT_LINK(&extension);

void GameFrame(bool simulating);
// native bool:SocketIsConnected(Handle:socket);
cell_t SocketIsConnected(IPluginContext *pContext, const cell_t *params);
// native Handle:SocketCreate(SocketType:protocol=SOCKET_TCP, SocketErrorCB:efunc);
cell_t SocketCreate(IPluginContext *pContext, const cell_t *params);
// native SocketBind(Handle:socket, String:hostname[], port);
cell_t SocketBind(IPluginContext *pContext, const cell_t *params);
// native SocketConnect(Handle:socket, SocketConnectCB:cfunc, SocketReceiveCB:rfunc, SocketDisconnectCB:dfunc, String:hostname[], port);
cell_t SocketConnect(IPluginContext *pContext, const cell_t *params);
// native SocketDisconnect(Handle:socket);
cell_t SocketDisconnect(IPluginContext *pContext, const cell_t *params);
// native SocketListen(Handle:socket, SocketIncomingCB:ifunc);
cell_t SocketListen(IPluginContext *pContext, const cell_t *params);
// native SocketSend(Handle:socket, String:command[], size);
cell_t SocketSend(IPluginContext *pContext, const cell_t *params);
// native SocketSendTo(Handle:socket, const String:data[], size=-1, const String:hostname[], port);
cell_t SocketSendTo(IPluginContext *pContext, const cell_t *params);
// native SocketSetOption(Handle:socket, SocketOption:option, value)
cell_t SocketSetOption(IPluginContext *pContext, const cell_t *params);
// native SocketSetReceiveCallback(Handle:socket, SocketReceiveCB:rfunc);
cell_t SocketSetReceiveCallback(IPluginContext *pContext, const cell_t *params);
// native SocketSetSendqueueEmptyCallback(Handle:socket, SocketSendqueueEmptyCB:sfunc);
cell_t SocketSetSendqueueEmptyCallback(IPluginContext *pContext, const cell_t *params);
// native SocketSetDisconnectCallback(Handle:socket, SocketDisconnectCB:dfunc);
cell_t SocketSetDisconnectCallback(IPluginContext *pContext, const cell_t *params);
// native SocketSetErrorCallback(Handle:socket, SocketErrorCB:efunc);
cell_t SocketSetErrorCallback(IPluginContext *pContext, const cell_t *params);
// native SocketSetArg(Handle:socket, any:arg);
cell_t SocketSetArg(IPluginContext *pContext, const cell_t *params);
// native SocketGetHostName(String:dest[], destLen);
cell_t SocketGetHostName(IPluginContext *pContext, const cell_t *params);

void GameFrame(bool /*simulating*/) {
  callbackHandler.ExecuteQueuedCallbacks();
}

extern const sp_nativeinfo_t smsock_natives[];

bool Extension::SDK_OnLoad(char */*error*/, size_t /*err_max*/, bool /*late*/) {
  smutils->AddGameFrameHook(&GameFrame);

  sharesys->AddNatives(myself, smsock_natives);
  socketHandleType = handlesys->CreateType("Socket", this, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);

  //if (_debug) smutils->LogError(myself, "[Debug] Extension loaded");
  socketHandler.StartProcessing();

  return true;
}

void Extension::SDK_OnUnload() {
  smutils->RemoveGameFrameHook(&GameFrame);
  handlesys->RemoveType(socketHandleType, nullptr);

  socketHandler.Shutdown();
}

void Extension::OnHandleDestroy(HandleType_t type, void *object) {
  if (type == socketHandleType && object != nullptr) {
    socketHandler.DestroySocket(reinterpret_cast<SocketWrapper*>(object));
  }
}

SocketWrapper* Extension::GetSocketWrapperByHandle(Handle_t handle) {
  HandleSecurity sec;
  sec.pOwner = nullptr;
  sec.pIdentity = myself->GetIdentity();
  SocketWrapper* sw;

  if (handlesys->ReadHandle(handle, socketHandleType, &sec, reinterpret_cast<void**>(&sw)) != HandleError_None) return nullptr;

  return sw;
}


// native bool:SocketIsConnected(Handle:socket);
cell_t SocketIsConnected(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return reinterpret_cast<Socket<tcp>*>(sw->socket)->IsOpen();
      #pragma clang diagnostic pop
    case SM_SocketType_Udp:
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return reinterpret_cast<Socket<udp>*>(sw->socket)->IsOpen();
      #pragma clang diagnostic pop
  }
}


// native Handle:SocketCreate(SocketType:protocol=SOCKET_TCP, SocketErrorCB:efunc);
cell_t SocketCreate(IPluginContext *pContext, const cell_t *params) {
  if (params[1] != SM_SocketType_Tcp && params[1] != SM_SocketType_Udp) return pContext->ThrowNativeError("Invalid protocol specified");
  if (!pContext->GetFunctionById(static_cast<funcid_t>(params[2])))
    return pContext->ThrowNativeError("Invalid error callback specified");

  cell_t handle = -1;

  switch (params[1]) {
    case SM_SocketType_Tcp: {
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      Socket<tcp>* socket = socketHandler.CreateSocket<tcp>(SM_SocketType_Tcp);
      #pragma clang diagnostic pop
      SocketWrapper* sw = socketHandler.GetSocketWrapper(socket);

      handle = static_cast<cell_t>(handlesys->CreateHandle(
        extension.socketHandleType,
        sw,
        pContext->GetIdentity(),
        myself->GetIdentity(),
        nullptr
      ));

      socket->smHandle = handle;
      socket->errorCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));

      break;
    }
    case SM_SocketType_Udp: {
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      Socket<udp>* socket = socketHandler.CreateSocket<udp>(SM_SocketType_Udp);
      #pragma clang diagnostic pop
      SocketWrapper* sw = socketHandler.GetSocketWrapper(socket);

      handle = static_cast<cell_t>(handlesys->CreateHandle(
        extension.socketHandleType,
        sw,
        pContext->GetIdentity(),
        myself->GetIdentity(),
        nullptr
      ));

      socket->smHandle = handle;
      socket->errorCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));

      break;
    }
  }

  return handle;
}

// native SocketBind(Handle:socket, String:hostname[], port);
cell_t SocketBind(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);
  if (params[3] < 0 || params[3] > 65535) return pContext->ThrowNativeError("Invalid port specified");

  char *hostname = nullptr;
  pContext->LocalToString(params[2], &hostname);

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return reinterpret_cast<Socket<tcp>*>(sw->socket)->Bind(hostname, static_cast<uint16_t>(params[3]), false);
      #pragma clang diagnostic pop
    case SM_SocketType_Udp:
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return reinterpret_cast<Socket<udp>*>(sw->socket)->Bind(hostname, static_cast<uint16_t>(params[3]), false);
      #pragma clang diagnostic pop
  }
}

// native SocketConnect(Handle:socket, SocketConnectCB:cfunc, SocketReceiveCB:rfunc, SocketDisconnectCB:dfunc, String:hostname[], port);
cell_t SocketConnect(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);
  //if (socket->shouldListen()) return pContext->ThrowNativeError("You can't connect a listening socket");
  if (!pContext->GetFunctionById(static_cast<funcid_t>(params[2]))) return pContext->ThrowNativeError("Invalid connect callback specified");
  if (!pContext->GetFunctionById(static_cast<funcid_t>(params[3]))) return pContext->ThrowNativeError("Invalid receive callback specified");
  if (!pContext->GetFunctionById(static_cast<funcid_t>(params[4]))) return pContext->ThrowNativeError("Invalid disconnect callback specified");
  if (params[6] < 0 || params[6] > 65535) return pContext->ThrowNativeError("Invalid port specified");

  char *hostname = nullptr;
  pContext->LocalToString(params[5], &hostname);

  switch (sw->socketType) {
    case SM_SocketType_Tcp: {
      Socket<tcp>* socket = reinterpret_cast<Socket<tcp>*>(sw->socket);
      if (socket->IsOpen()) return pContext->ThrowNativeError("Socket is already connected");

      socket->connectCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      socket->receiveCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[3]));
      socket->disconnectCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[4]));

      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Connect(hostname, static_cast<uint16_t>(params[6]));
      #pragma clang diagnostic pop
    }
    case SM_SocketType_Udp: {
      Socket<udp>* socket = reinterpret_cast<Socket<udp>*>(sw->socket);
      if (socket->IsOpen()) return pContext->ThrowNativeError("Socket is already connected");

      socket->connectCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      socket->receiveCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[3]));
      socket->disconnectCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[4]));

      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Connect(hostname, static_cast<uint16_t>(params[6]));
      #pragma clang diagnostic pop
    }
  }
}

// native SocketDisconnect(Handle:socket);
cell_t SocketDisconnect(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  switch (sw->socketType) {
    case SM_SocketType_Tcp: {
      Socket<tcp>* socket = reinterpret_cast<Socket<tcp>*>(sw->socket);
      if (!socket->IsOpen()) return pContext->ThrowNativeError("Socket is not connected/listening");
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Disconnect();
      #pragma clang diagnostic pop
    }
    case SM_SocketType_Udp: {
      Socket<udp>* socket = reinterpret_cast<Socket<udp>*>(sw->socket);
      if (!socket->IsOpen()) return pContext->ThrowNativeError("Socket is not connected/listening");
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Disconnect();
      #pragma clang diagnostic pop
    }
  }
}

// native SocketListen(Handle:socket, SocketIncomingCB:ifunc);
cell_t SocketListen(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);
  if (sw->socketType != SM_SocketType_Tcp) return pContext->ThrowNativeError("The socket must use the TCP/SOCK_STREAM protocol");
  if (!pContext->GetFunctionById(static_cast<funcid_t>(params[2]))) return pContext->ThrowNativeError("Invalid incoming callback specified");

  switch (sw->socketType) {
    case SM_SocketType_Tcp: {
      Socket<tcp>* socket = reinterpret_cast<Socket<tcp>*>(sw->socket);
      if (socket->IsOpen()) return pContext->ThrowNativeError("Socket is already open");
      socket->incomingCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Listen();
      #pragma clang diagnostic pop
    }
    case SM_SocketType_Udp: {
      Socket<udp>* socket = reinterpret_cast<Socket<udp>*>(sw->socket);
      if (socket->IsOpen()) return pContext->ThrowNativeError("Socket is already open");
      socket->incomingCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Listen();
      #pragma clang diagnostic pop
    }
  }
}

// native SocketSend(Handle:socket, String:command[], size);
cell_t SocketSend(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  char* dataTmp = nullptr;
  pContext->LocalToString(params[2], &dataTmp);

  std::string data;

  if (params[3] == -1) {
    data.assign(dataTmp);
  } else {
    data.assign(dataTmp, static_cast<std::string::size_type>(params[3]));
  }

  switch (sw->socketType) {
    case SM_SocketType_Tcp: {
      Socket<tcp>* socket = reinterpret_cast<Socket<tcp>*>(sw->socket);
      if (!socket->IsOpen()) return pContext->ThrowNativeError("Can't send, socket is not connected");
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Send(data);
      #pragma clang diagnostic pop
    }
    case SM_SocketType_Udp: {
      Socket<udp>* socket = reinterpret_cast<Socket<udp>*>(sw->socket);
      if (!socket->IsOpen()) return pContext->ThrowNativeError("Can't send, socket is not connected");
      socket->incomingCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->Send(data);
      #pragma clang diagnostic pop
    }
  }
}

// native SocketSendTo(Handle:socket, const String:data[], size=-1, const String:hostname[], port);
cell_t SocketSendTo(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);
  if (sw->socketType == SM_SocketType_Tcp)
    return pContext->ThrowNativeError("This native doesn't support connection orientated protocols");

  char* dataTmp = nullptr;
  pContext->LocalToString(params[2], &dataTmp);

  std::string data;

  if (params[3] == -1) {
    data.assign(dataTmp);
  } else {
    data.assign(dataTmp, static_cast<std::string::size_type>(params[3]));
  }

  char* hostname = nullptr;
  pContext->LocalToString(params[4], &hostname);

  switch (sw->socketType) {
    case SM_SocketType_Udp: {
      Socket<udp>* socket = reinterpret_cast<Socket<udp>*>(sw->socket);
      //if (!socket->IsOpen()) return pContext->ThrowNativeError("Can't send, socket is not connected");
      socket->incomingCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
      // todo: figure this out
      return socket->SendTo(data, hostname, static_cast<uint16_t>(params[5]));
      #pragma clang diagnostic pop
    }
    case SM_SocketType_Tcp:
      #ifdef NDEBUG
        return pContext->ThrowNativeError("This native doesn't support connection orientated protocols");
      #else
        assert(false);
        return false;
      #endif
  }
}

// native SocketSetOption(Handle:socket, SocketOption:option, value)
cell_t SocketSetOption(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (params[2] != SM_SO_ConcatenateCallbacks &&
    params[2] != SM_SO_ForceFrameLock &&
    params[2] != SM_SO_CallbacksPerFrame &&
    params[2] != SM_SO_DebugMode) {
    if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

    switch (sw->socketType) {
      case SM_SocketType_Tcp: {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wundefined-func-template"
        // todo: figure this out
        return reinterpret_cast<Socket<tcp>*>(sw->socket)->SetOption(static_cast<SM_SocketOption>(params[2]), params[3]);
        #pragma clang diagnostic pop
      }
      case SM_SocketType_Udp: {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wundefined-func-template"
        // todo: figure this out
        return reinterpret_cast<Socket<udp>*>(sw->socket)->SetOption(static_cast<SM_SocketOption>(params[2]), params[3]);
        #pragma clang diagnostic pop
      }
    }
  } else {
    return false;
  }

#if 0
  switch (params[2]) {
    case ConcatenateCallbacks:
      socket->setOption(ConcatenateCallbacks, value);
      return 1;
    case ForceFrameLock:
      callbacks->setOption(ForceFrameLock, value);
      return 1;
    case CallbacksPerFrame:
      if (value > 0) {
        callbacks->setOption(CallbacksPerFrame, value);
        return 1;
      } else {
        return 0;
      }
    case DebugMode:
      sockets._debug = value != 0;
      return 1;
...
  }
#endif
}


// native SocketSetReceiveCallback(Handle:socket, SocketReceiveCB:rfunc);
cell_t SocketSetReceiveCallback(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      reinterpret_cast<Socket<tcp>*>(sw->socket)->receiveCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      break;
    case SM_SocketType_Udp:
      reinterpret_cast<Socket<udp>*>(sw->socket)->receiveCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      break;
  }

  return true;
}

// native SocketSetSendqueueEmptyCallback(Handle:socket, SocketSendqueueEmptyCB:sfunc);
cell_t SocketSetSendqueueEmptyCallback(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  bool forceSendqueueEmptyCallback = false;

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      reinterpret_cast<Socket<tcp>*>(sw->socket)->sendqueueEmptyCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      if (!reinterpret_cast<Socket<tcp>*>(sw->socket)->sendQueueLength) forceSendqueueEmptyCallback = true;
      break;
    case SM_SocketType_Udp:
      reinterpret_cast<Socket<udp>*>(sw->socket)->sendqueueEmptyCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      if (!reinterpret_cast<Socket<udp>*>(sw->socket)->sendQueueLength) forceSendqueueEmptyCallback = true;
      break;
  }

  if (forceSendqueueEmptyCallback) {
    callbackHandler.AddCallback(new Callback(CallbackEvent_SendQueueEmpty, sw->socket));
  }

  return true;
}

// native SocketSetDisconnectCallback(Handle:socket, SocketDisconnectCB:dfunc);
cell_t SocketSetDisconnectCallback(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      reinterpret_cast<Socket<tcp>*>(sw->socket)->disconnectCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      break;
    case SM_SocketType_Udp:
      reinterpret_cast<Socket<udp>*>(sw->socket)->disconnectCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      break;
  }

  return true;
}

// native SocketSetErrorCallback(Handle:socket, SocketErrorCB:efunc);
cell_t SocketSetErrorCallback(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      reinterpret_cast<Socket<tcp>*>(sw->socket)->errorCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      break;
    case SM_SocketType_Udp:
      reinterpret_cast<Socket<udp>*>(sw->socket)->errorCallback = pContext->GetFunctionById(static_cast<funcid_t>(params[2]));
      break;
  }

  return true;
}


// native SocketSetArg(Handle:socket, any:arg);
cell_t SocketSetArg(IPluginContext *pContext, const cell_t *params) {
  SocketWrapper* sw = extension.GetSocketWrapperByHandle(static_cast<Handle_t>(params[1]));
  if (sw == nullptr) return pContext->ThrowNativeError("Invalid handle: %i", params[1]);

  switch (sw->socketType) {
    case SM_SocketType_Tcp:
      reinterpret_cast<Socket<tcp>*>(sw->socket)->smCallbackArg = params[2];
      break;
    case SM_SocketType_Udp:
      reinterpret_cast<Socket<udp>*>(sw->socket)->smCallbackArg = params[2];
      break;
  }

  return true;
}

// native SocketGetHostName(String:dest[], destLen);
cell_t SocketGetHostName(IPluginContext *pContext, const cell_t *params) {
  char* dest = nullptr;
  pContext->LocalToString(params[1], &dest);

  boost::system::error_code errorCode;
  std::string hostName = host_name(errorCode);

  if (!errorCode) {
    size_t len = hostName.copy(dest, static_cast<std::string::size_type>(params[2]-1));
    dest[len] = '\0';
    return true;
  } else {
    dest[0] = '\0';
    return false;
  }
}

const sp_nativeinfo_t smsock_natives[] = {
  {"SocketIsConnected",   SocketIsConnected},

  {"SocketCreate",      SocketCreate},
  {"SocketBind",        SocketBind},
  {"SocketConnect",     SocketConnect},
  {"SocketDisconnect",    SocketDisconnect},
  {"SocketListen",      SocketListen},
  {"SocketSend",        SocketSend},
  {"SocketSendTo",      SocketSendTo},
  {"SocketSetOption",     SocketSetOption},

  {"SocketSetReceiveCallback",    SocketSetReceiveCallback},
  {"SocketSetSendqueueEmptyCallback", SocketSetSendqueueEmptyCallback},
  {"SocketSetDisconnectCallback",   SocketSetDisconnectCallback},
  {"SocketSetErrorCallback",      SocketSetErrorCallback},

  {"SocketSetArg",      SocketSetArg},

  {"SocketGetHostName",   SocketGetHostName},

  {nullptr,           nullptr},
};
