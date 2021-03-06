# RemoteMessage
RemoteMessage is an **asynchronous** inter-process communication C++ library.
Note: RemoteMessage **only support IPv4**. You may change code to support IPv6.

# Example
RMsgTestServer and RMsgTestClient are an console example based on RemoteMessage.
RMsgTest is an GUI chat example based on RemoteMessage.

# Usage
You can send raw message(Message class)/protobuf message to the peer.
If you will, you can use other information exchange format (such as thrift) to replace protobuf.
To use other information exchange format, you just need to add template methods in Session.h just as protobuf does.

The thread which calls **Session::Run** is called **message thread**.
Message thread should be worker thread if there is UI thread.
You can post messages to the peer process from any threads. **Thread safety** is ensured.

You can send raw/protobuf message with/without reply.

Message without reply is called **notice message**.
Use **Session::EnqueueNotice/EnqueuePbNotice** to post notice message.

Message with reply is called **request message**.
You use **Session::EnqueueRequest/EnqueuePbRequest** to post request message.
You must provide a **callback** to process the reply message. Usually reply message is notice message.

Non-reply message is classified by category and class.
Non-reply message must have nested message TypeInfo like this:

	message TypeInfo
	{
		optional int32 Category = 1 [default = 0];
		optional int32 Method = 2   [default = 0];
	}

You can register non-reply message handler by **Session::RegisterMessageHandler**.
RemoteMessage library use (Category, Method) to find non-reply message handler, so default value of (Category, Method) must be different for different non-reply message.

You can call **Session::RegisterDisconnect** to detect if socket is disconnected.

You can disable debug info to improve performance, or enable it to debug communication error.

    EnableDebugInfo(bool);
    
Machine byte order is not considered yet.

If you have good ideas or find bugs, please contact me(cdp97531@sina.com)
