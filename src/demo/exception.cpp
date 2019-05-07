/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "exception.h"

const char* MiniSync::Exceptions::TimeoutException::what() const noexcept
{
    return "Operation timed out.";
}

const char* MiniSync::Exceptions::SocketReadException::what() const noexcept
{
    return "Error while trying to read from socket.";
}

const char* MiniSync::Exceptions::SocketWriteException::what() const noexcept
{
    return "Error while trying to write to socket.";
}

const char* MiniSync::Exceptions::DeserializeMsgException::what() const noexcept
{
    return "Error deserializing byte buffer into Protobuf Message.";
}

const char* MiniSync::Exceptions::SerializeMsgException::what() const noexcept
{
    return "Error serializing Protobuf Message into byte buffer.";
}
