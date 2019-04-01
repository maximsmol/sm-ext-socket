#include "Callback.h"

#include <assert.h>
#include <string>
#include <boost/asio.hpp>

#include "Define.h"
#include "Extension.h"
#include "Socket.h"
#include "SocketHandler.h"

using namespace boost::asio::ip;

Callback::Callback(CallbackEvent arg_callbackEvent,
				   const void* socket) : callbackEvent(arg_callbackEvent) {
	assert(callbackEvent == CallbackEvent_Connect || callbackEvent == CallbackEvent_Disconnect || callbackEvent == CallbackEvent_SendQueueEmpty);

	socketWrapper = socketHandler.GetSocketWrapper(socket);
}

Callback::Callback(CallbackEvent arg_callbackEvent,
				   const void* socket,
				   const char* data,
				   size_t dataLength) : callbackEvent(arg_callbackEvent) {
	assert(callbackEvent == CallbackEvent_Receive);

	socketWrapper = socketHandler.GetSocketWrapper(socket);
	additionalData[0] = new std::string(data, dataLength);
}

Callback::Callback(CallbackEvent arg_callbackEvent,
				   const void* socket,
				   const void* newSocket,
	   			   const tcp::endpoint& remoteEndPoint) : callbackEvent(arg_callbackEvent) {
	assert(callbackEvent == CallbackEvent_Incoming);

	socketWrapper = socketHandler.GetSocketWrapper(socket);
	additionalData[0] = socketHandler.GetSocketWrapper(newSocket);
	additionalData[1] = new tcp::endpoint(remoteEndPoint);
}

Callback::Callback(CallbackEvent arg_callbackEvent,
				   const void* socket,
				   SM_ErrorType errorType,
				   int errorNumber) : callbackEvent(arg_callbackEvent) {
	assert(callbackEvent == CallbackEvent_Error);

	socketWrapper = socketHandler.GetSocketWrapper(socket);
	additionalData[0] = new SM_ErrorType(errorType);
	additionalData[1] = new int(errorNumber);
}

Callback::~Callback() {
	if (callbackEvent == CallbackEvent_Receive) {
		delete reinterpret_cast<const std::string*>(additionalData[0]);
	} else if (callbackEvent == CallbackEvent_Incoming) {
		delete reinterpret_cast<const tcp::endpoint*>(additionalData[1]);
	} else if (callbackEvent == CallbackEvent_Error) {
		delete reinterpret_cast<const SM_ErrorType*>(additionalData[0]);
		delete reinterpret_cast<const int*>(additionalData[1]);
	}
}

bool Callback::IsExecutable() {
	if (!socketWrapper) return false;

	switch (socketWrapper->socketType) {
		case SM_SocketType_Tcp: {
			Socket<tcp>* socket = reinterpret_cast<Socket<tcp>*>(socketWrapper->socket);

			switch (callbackEvent) {
				case CallbackEvent_Connect:
					return (socket->connectCallback != nullptr);
				case CallbackEvent_Disconnect:
					return (socket->disconnectCallback != nullptr);
				case CallbackEvent_Incoming:
					return (socket->incomingCallback != nullptr);
				case CallbackEvent_Receive:
					return (socket->receiveCallback != nullptr);
				case CallbackEvent_SendQueueEmpty:
					return (socket->sendqueueEmptyCallback != nullptr);
				case CallbackEvent_Error:
					return (socket->errorCallback != nullptr);
			}
		}
		case SM_SocketType_Udp: {
			Socket<udp>* socket = reinterpret_cast<Socket<udp>*>(socketWrapper->socket);

			switch (callbackEvent) {
				case CallbackEvent_Connect:
					return (socket->connectCallback != nullptr);
				case CallbackEvent_Disconnect:
					return (socket->disconnectCallback != nullptr);
				case CallbackEvent_Incoming:
					return (socket->incomingCallback != nullptr);
				case CallbackEvent_Receive:
					return (socket->receiveCallback != nullptr);
				case CallbackEvent_SendQueueEmpty:
					return (socket->sendqueueEmptyCallback != nullptr);
				case CallbackEvent_Error:
					return (socket->errorCallback != nullptr);
			}
		}
	}

	return false;
}

bool Callback::IsValid() {
	if (!socketWrapper) return false;
	if (callbackEvent == CallbackEvent_Incoming && (!additionalData[0] || !additionalData[1])) return false;

	return true;
}

void Callback::Execute() {
	switch (socketWrapper->socketType) {
		case SM_SocketType_Tcp: {
			ExecuteHelper<tcp>();
			break;
		}
		case SM_SocketType_Udp: {
			ExecuteHelper<udp>();
			break;
		}
	}
}

template<class SocketType>
void Callback::ExecuteHelper() {
	if (!IsValid()) return;

	Socket<SocketType>* socket = reinterpret_cast<Socket<SocketType>*>(socketWrapper->socket);

	switch (callbackEvent) {
		case CallbackEvent_Connect:
			if (!socket->connectCallback) return;

			socket->connectCallback->PushCell(socket->smHandle);
			socket->connectCallback->PushCell(socket->smCallbackArg);
			socket->connectCallback->Execute(nullptr);

			return;
		case CallbackEvent_Disconnect:
			if (!socket->disconnectCallback) return;

			socket->disconnectCallback->PushCell(socket->smHandle);
			socket->disconnectCallback->PushCell(socket->smCallbackArg);
			socket->disconnectCallback->Execute(nullptr);

			return;
		case CallbackEvent_Incoming: {
			if (!socket->incomingCallback) return;

			Socket<SocketType>* socket2 = reinterpret_cast<Socket<SocketType>*>(reinterpret_cast<const SocketWrapper*>(additionalData[0])->socket);
			socket2->smHandle = static_cast<int32_t>(handlesys->CreateHandle(
				extension.socketHandleType,
				socketHandler.GetSocketWrapper(socket2),
				socket->incomingCallback->GetParentContext()->GetIdentity(),
				myself->GetIdentity(),
				nullptr
			));

			socket->incomingCallback->PushCell(socket->smHandle);
			socket->incomingCallback->PushCell(socket2->smHandle);
			socket->incomingCallback->PushString(reinterpret_cast<const typename SocketType::endpoint*>(additionalData[1])->address().to_string().c_str());
			socket->incomingCallback->PushCell(reinterpret_cast<const typename SocketType::endpoint*>(additionalData[1])->port());
			socket->incomingCallback->PushCell(socket->smCallbackArg);
			socket->incomingCallback->Execute(nullptr);

			return;
		}
		case CallbackEvent_Receive: {
			if (!socket->receiveCallback) return;

			size_t strLen = reinterpret_cast<const std::string*>(additionalData[0])->length();
			char* tmp = new char[strLen+1];
			memcpy(tmp, reinterpret_cast<const std::string*>(additionalData[0])->c_str(), strLen+1);

			socket->receiveCallback->PushCell(socket->smHandle);
			socket->receiveCallback->PushStringEx(tmp, strLen+1, SM_PARAM_STRING_COPY|SM_PARAM_STRING_BINARY, 0);
			socket->receiveCallback->PushCell(static_cast<cell_t>(strLen));
			socket->receiveCallback->PushCell(socket->smCallbackArg);
			socket->receiveCallback->Execute(nullptr);

			delete[] tmp;
			return;
		}
		case CallbackEvent_SendQueueEmpty:
			if (!socket->sendqueueEmptyCallback) return;

			socket->sendqueueEmptyCallback->PushCell(socket->smHandle);
			socket->sendqueueEmptyCallback->PushCell(socket->smCallbackArg);
			socket->sendqueueEmptyCallback->Execute(nullptr);

			return;
		case CallbackEvent_Error:
			if (!socket->errorCallback) return;

			socket->errorCallback->PushCell(socket->smHandle);
			socket->errorCallback->PushCell(*reinterpret_cast<const SM_ErrorType*>(additionalData[0]));
			socket->errorCallback->PushCell(*reinterpret_cast<const int*>(additionalData[1]));
			socket->errorCallback->PushCell(socket->smCallbackArg);
			socket->errorCallback->Execute(nullptr);

			return;
	}
}
