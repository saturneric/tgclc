//
// Created by eric on 5/13/20.
//

#ifndef TGCLIC_APPLICATION_H
#define TGCLIC_APPLICATION_H

#include <memory>
#include <map>
#include <cstdint>
#include <functional>
#include <iostream>

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <limits>
#include <sstream>

#include "utils.h"

#include "request_processor.h"
#include "respond_processor.h"

using std::unique_ptr;
using std::map;
using namespace td;

namespace tgclc {

    class Application {
    public:
        Application() {

        }

        void loop() {

            send_query(td_api::make_object<td_api::addProxy>(
                    "192.168.20.1",
                    7891,
                    true,
                    td_api::make_object<td_api::proxyTypeSocks5>()),
                       [this](Object object) { std::cout << to_string(object) << std::endl; });

            while (true) {

                if (need_restart_) {
                    restart();
                } else if (!are_authorized_) {
                    process_response(infoPool.getClient().receive(10));
                } else {
                    std::cout
                            << "Enter action [q] quit [u] check for updates and request results [c] show chats [m <id> <text>] "
                               "send message [me] show self [l] logout: "
                            << std::endl;
                    std::string line;
                    std::getline(std::cin, line);
                    std::istringstream ss(line);
                    std::string action;
                    if (!(ss >> action)) {
                        continue;
                    }
                    if (action == "q") {
                        return;
                    }
                    if (action == "u") {
                        std::cout << "Checking for updates..." << std::endl;
                        while (true) {
                            auto response = infoPool.getClient().receive(0);
                            if (response.object) {
                                respondProcessor.deal(std::move(response));
                            } else {
                                break;
                            }
                        }
                    } else if (action == "close") {
                        std::cout << "Closing..." << std::endl;
                        requestProcessor.send(td_api::make_object<td_api::close>(), {});
                    } else if (action == "me") {
                        requestProcessor.send(td_api::make_object<td_api::getMe>(),
                                   [this](Object object) { std::cout << to_string(object) << std::endl; });
                    } else if (action == "l") {
                        std::cout << "Logging out..." << std::endl;
                        requestProcessor.send(td_api::make_object<td_api::logOut>(), {});
                    } else if (action == "m") {
                        std::int64_t chat_id;
                        ss >> chat_id;
                        ss.get();
                        std::string text;
                        std::getline(ss, text);

                        std::cout << "Sending message to chat " << chat_id << "..." << std::endl;
                        auto send_message = td_api::make_object<td_api::sendMessage>();
                        send_message->chat_id_ = chat_id;
                        auto message_content = td_api::make_object<td_api::inputMessageText>();
                        message_content->text_ = td_api::make_object<td_api::formattedText>();
                        message_content->text_->text_ = std::move(text);
                        send_message->input_message_content_ = std::move(message_content);

                        requestProcessor.send(std::move(send_message), {});
                    } else if (action == "c") {
                        std::cout << "Loading chat list..." << std::endl;
                        requestProcessor.send(
                                td_api::make_object<td_api::getChats>(nullptr, std::numeric_limits<std::int64_t>::max(),
                                                                      0, 20),
                                [this](Object object) {
                                    if (object->get_id() == td_api::error::ID) {
                                        return;
                                    }
                                    auto chats = td::move_tl_object_as<td_api::chats>(object);
                                    for (auto chat_id : chats->chat_ids_) {
                                        std::cout << "[id:" << chat_id << "] [title:" << chat_title_[chat_id] << "]"
                                                  << std::endl;
                                    }
                                });
                    }
                }
            }
        }

    private:
        using Object = td_api::object_ptr<td_api::Object>;

        td_api::object_ptr<td_api::AuthorizationState> authorization_state_;

        bool are_authorized_{false};
        bool need_restart_{false};
        std::uint64_t current_query_id_{0};
        std::uint64_t authentication_query_id_{0};

        std::map<std::int32_t, td_api::object_ptr<td_api::user>> users_;

        std::map<std::int64_t, std::string> chat_title_;

        std::map<std::uint64_t, std::function<void(Object)>> handlers_;

        RespondProcessor& respondProcessor =  singleton<RespondProcessor>::instance();

        RequestProcessor& requestProcessor = singleton<RequestProcessor>::instance();

        InfoPool& infoPool = singleton<InfoPool>::instance();

        void restart() {

        }

        std::string get_user_name(std::int32_t user_id) {
            auto it = users_.find(user_id);
            if (it == users_.end()) {
                return "unknown user";
            }
            return it->second->first_name_ + " " + it->second->last_name_;
        }

        void send_query(td_api::object_ptr<td_api::Function> f, std::function<void(Object)> handler) {
            auto query_id = infoPool.createHandler(handler);
            if (handler) {
                // handlers_.emplace(query_id, std::move(handler));
            }
            infoPool.getClient().send({query_id, std::move(f)});
        }

        void process_response(td::Client::Response response) {
            if (!response.object) {
                return;
            }

            if (response.id == 0) {
                return process_update(std::move(response.object));
            }

            infoPool.setHandlerRespondObject(response.id, std::move(response.object));
        }

        void process_update(td_api::object_ptr<td_api::Object> update) {
            td_api::downcast_call(
                    *update, overloaded(
                            [this](td_api::updateAuthorizationState &update_authorization_state) {
                                authorization_state_ = std::move(update_authorization_state.authorization_state_);
                                on_authorization_state_update();
                            },
                            [this](td_api::updateNewChat &update_new_chat) {
                                chat_title_[update_new_chat.chat_->id_] = update_new_chat.chat_->title_;
                            },
                            [this](td_api::updateChatTitle &update_chat_title) {
                                chat_title_[update_chat_title.chat_id_] = update_chat_title.title_;
                            },
                            [this](td_api::updateUser &update_user) {
                                auto user_id = update_user.user_->id_;
                                users_[user_id] = std::move(update_user.user_);
                            },
                            [this](td_api::updateNewMessage &update_new_message) {
                                auto chat_id = update_new_message.message_->chat_id_;
                                auto sender_user_name = get_user_name(update_new_message.message_->sender_user_id_);
                                std::string text;
                                if (update_new_message.message_->content_->get_id() == td_api::messageText::ID) {
                                    text = static_cast<td_api::messageText &>(*update_new_message.message_->content_).text_->text_;
                                }
                                std::cout << "Got message: [chat_id:" << chat_id << "] [from:" << sender_user_name
                                          << "] ["
                                          << text << "]" << std::endl;
                            },
                            [](auto &update) {}));
        }

        auto create_authentication_query_handler() {
            return [this, id = authentication_query_id_](Object object) {
                if (id == authentication_query_id_) {
                    check_authentication_error(std::move(object));
                }
            };
        }

        void on_authorization_state_update() {
            authentication_query_id_++;
            td_api::downcast_call(
                    *authorization_state_,
                    overloaded(
                            [this](td_api::authorizationStateReady &) {
                                are_authorized_ = true;
                                std::cout << "Got authorization" << std::endl;
                            },
                            [this](td_api::authorizationStateLoggingOut &) {
                                are_authorized_ = false;
                                std::cout << "Logging out" << std::endl;
                            },
                            [this](td_api::authorizationStateClosing &) { std::cout << "Closing" << std::endl; },
                            [this](td_api::authorizationStateClosed &) {
                                are_authorized_ = false;
                                need_restart_ = true;
                                std::cout << "Terminated" << std::endl;
                            },
                            [this](td_api::authorizationStateWaitCode &) {
                                std::cout << "Enter authentication code: " << std::flush;
                                std::string code;
                                std::cin >> code;
                                requestProcessor.send(td_api::make_object<td_api::checkAuthenticationCode>(code),
                                           create_authentication_query_handler());
                            },
                            [this](td_api::authorizationStateWaitRegistration &) {
                                std::string first_name;
                                std::string last_name;
                                std::cout << "Enter your first name: " << std::flush;
                                std::cin >> first_name;
                                std::cout << "Enter your last name: " << std::flush;
                                std::cin >> last_name;
                                requestProcessor.send(td_api::make_object<td_api::registerUser>(first_name, last_name),
                                           create_authentication_query_handler());
                            },
                            [this](td_api::authorizationStateWaitPassword &) {
                                std::cout << "Enter authentication password: " << std::flush;
                                std::string password;
                                std::cin >> password;
                                requestProcessor.send(td_api::make_object<td_api::checkAuthenticationPassword>(password),
                                           create_authentication_query_handler());
                            },
                            [this](td_api::authorizationStateWaitOtherDeviceConfirmation &state) {
                                std::cout << "Confirm this login link on another device: " << state.link_ << std::endl;
                            },
                            [this](td_api::authorizationStateWaitPhoneNumber &) {
                                std::cout << "Enter phone number: " << std::flush;
                                std::string phone_number;
                                std::cin >> phone_number;
                                requestProcessor.send(td_api::make_object<td_api::setAuthenticationPhoneNumber>(phone_number,
                                                                                                     nullptr),
                                           create_authentication_query_handler());
                            },
                            [this](td_api::authorizationStateWaitEncryptionKey &) {
                                std::cout << "Enter encryption key or DESTROY: " << std::flush;
                                std::string key;
                                std::getline(std::cin, key);
                                if (key == "DESTROY") {
                                    requestProcessor.send(td_api::make_object<td_api::destroy>(),
                                               create_authentication_query_handler());
                                } else {
                                    requestProcessor.send(td_api::make_object<td_api::checkDatabaseEncryptionKey>(std::move(key)),
                                               create_authentication_query_handler());
                                }
                            },
                            [this](td_api::authorizationStateWaitTdlibParameters &) {
                                auto parameters = td_api::make_object<td_api::tdlibParameters>();
                                parameters->database_directory_ = "tdlib";
                                parameters->use_message_database_ = true;
                                parameters->use_secret_chats_ = true;
                                parameters->api_id_ = 1368212;
                                parameters->api_hash_ = "8f5f44883e9d82d8112390fea55dbf47";
                                parameters->system_language_code_ = "zh";
                                parameters->device_model_ = "Desktop";
                                parameters->system_version_ = "Unknown";
                                parameters->application_version_ = "1.0";
                                parameters->enable_storage_optimizer_ = true;
                                requestProcessor.send(td_api::make_object<td_api::setTdlibParameters>(std::move(parameters)),
                                           create_authentication_query_handler());
                            }));
        }

        void check_authentication_error(Object object) {
            if (object->get_id() == td_api::error::ID) {
                auto error = td::move_tl_object_as<td_api::error>(object);
                std::cout << "Error: " << to_string(error) << std::flush;
                on_authorization_state_update();
            }
        }

        std::uint64_t next_query_id() {
            return ++current_query_id_;
        }
    };

}

#endif //TGCLIC_APPLICATION_H
