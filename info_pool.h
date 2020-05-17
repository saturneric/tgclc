//
// Created by eric on 5/17/20.
//

#ifndef TGCLIC_INFO_POOL_H
#define TGCLIC_INFO_POOL_H

#include <functional>
#include <stdint.h>
#include <memory>
#include <map>

#include "tg.h"
#include "utils.h"

namespace tgclc {

    class InfoPool {
    public:

        InfoPool(){
            td::Client::execute({0, td_api::make_object<td_api::setLogVerbosityLevel>(1)});
            client_ = std::make_unique<td::Client>();
        }

        void setHandlerRespondObject(std::uint64_t id, object_p&& object) {
            auto it = handlers_.find(id);
            if (it != handlers_.end()) {
                it->second(std::move(object));
            }
        }

        auto createHandler(std::function<void(Object)> handler){
            auto query_id = get_new_query_id();
            if (handler) {
                handlers_.emplace(query_id, std::move(handler));
            }
            return query_id;
        }

        td::Client& getClient(){
            return *client_;
        }


    private:

        std::uint64_t current_query_id_{0};

        std::map<std::uint64_t, std::function<void(object_p)>> handlers_;

        std::unique_ptr<td::Client> client_;

        std::uint64_t get_new_query_id(){
            return ++current_query_id_;
        }
    };

}


#endif //TGCLIC_INFO_POOL_H
