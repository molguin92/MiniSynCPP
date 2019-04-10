/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
* 
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#ifndef MINISYNCPP_EXCEPTION_H
#define MINISYNCPP_EXCEPTION_H

#include <exception>

namespace MiniSync
{
    namespace Exceptions
    {
        class TimeoutException : public std::exception
        {
        public:
            const char* what() const noexcept final;
        };

        class SocketReadException : public std::exception
        {
        public:
            const char* what() const noexcept final;
        };

        class SocketWriteException : public std::exception
        {
        public:
            const char* what() const noexcept final;
        };

        class DeserializeMsgException : public std::exception
        {
        public:
            const char* what() const noexcept final;
        };

        class SerializeMsgException : public std::exception
        {
        public:
            const char* what() const noexcept final;
        };
    }
}


#endif //MINISYNCPP_EXCEPTION_H
