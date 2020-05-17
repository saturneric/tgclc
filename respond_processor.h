//
// Created by eric on 5/17/20.
//

#ifndef TGCLIC_RESPOND_PROCESSOR_H
#define TGCLIC_RESPOND_PROCESSOR_H

#include <map>
#include <functional>


#include "tg.h"
#include "utils.h"
#include "info_pool.h"

namespace tgclc {

    // Dealing And Recording With Responds From Client
    class RespondProcessor {
    public:

        void deal(Client::Response response) {
            if (!response.object) {
                return;
            }

            if (response.id == 0) {
                return process_update(std::move(response.object));
            }
            this->record(response.id, std::move(response.object));
        }

    private:

        InfoPool& infoPool = singleton<InfoPool>::instance();

        std::vector<std::function<void(td_api::Update &)>> event_processor;

        void record(std::uint64_t id, object_p object) {
            infoPool.setHandlerRespondObject(id, std::move(object));
        }

        void process_update(td_api::object_ptr<td_api::Object> update) {
            td_api::downcast_call(
                    *update, overloaded(
                            [this](td_api::updateAuthorizationState &update_authorization_state) {
                                event_processor[0](update_authorization_state);
                            },
                            [this](td_api::updateNewChat &update_new_chat) {
                                event_processor[1](update_new_chat);
                            },
                            [this](td_api::updateChatTitle &update_chat_title) {

                            },
                            [this](td_api::updateUser &update_user) {

                            },
                            [this](td_api::updateNewMessage &update_new_message) {

                            },
                            [](auto &update) {}));
        }

    };

}


#endif //TGCLIC_RESPOND_PROCESSOR_H
