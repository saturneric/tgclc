//
// Created by eric on 5/17/20.
//

#ifndef TGCLIC_TG_H
#define TGCLIC_TG_H

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

using namespace td;

using object = td_api::Object;
using object_p = td_api::object_ptr<object>;
using object_r = td_api::Object &;


using Object = object_p;

#endif //TGCLIC_TG_H
