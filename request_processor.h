//
// Created by eric on 5/17/20.
//

#ifndef TGCLIC_REQUEST_PROCESSOR_H
#define TGCLIC_REQUEST_PROCESSOR_H

#include "tg.h"
#include "utils.h"
#include "info_pool.h"

namespace tgclc {

    class RequestProcessor {
    public:

        void send(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler) {
            auto query_id =  infoPool.createHandler(std::move(handler));
            infoPool.getClient().send({query_id, std::move(f)});
        }

    private:
        InfoPool& infoPool = singleton<InfoPool>::instance();

    };

}


#endif //TGCLIC_REQUEST_PROCESSOR_H
