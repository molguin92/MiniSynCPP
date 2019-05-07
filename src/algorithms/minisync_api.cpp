/*
* Author: Manuel Olguín Muñoz <manuel@olguin.se>
*
* Copyright© 2019 Manuel Olguín Muñoz
* See LICENSE file included in the root directory of this project for licensing and copyright details.
*/

#include "minisync_api.h"
#include "minisync.h"


std::shared_ptr<MiniSync::API::Algorithm> MiniSync::API::Factory::createTinySync()
{
    return std::shared_ptr<MiniSync::API::Algorithm>(new MiniSync::Algorithms::TinySync());
}

std::shared_ptr<MiniSync::API::Algorithm> MiniSync::API::Factory::createMiniSync()
{
    return std::shared_ptr<MiniSync::API::Algorithm>(new MiniSync::Algorithms::MiniSync());
}
