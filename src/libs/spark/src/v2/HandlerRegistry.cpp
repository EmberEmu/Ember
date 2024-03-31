/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/v2/HandlerRegistry.h>
#include <spark/v2/Handler.h>

namespace ember::spark::v2 {

void HandlerRegistry::deregister_service(Handler* service) {
	std::lock_guard<std::mutex> guard(mutex_);
	
	if(!services_.contains(service->type())) {
		return;
	}

	auto& handlers = services_.at(service->type());

	for(auto i = handlers.begin(); i != handlers.end(); ++i) {
		if(*i == service) {
			handlers.erase(i);
			break;
		}
	}

	if(handlers.empty()) {
		services_.erase(service->type());
	}
}

void HandlerRegistry::deregister_client(Handler* service) {
	std::lock_guard<std::mutex> guard(mutex_);

	if(!clients_.contains(service->type())) {
		return;
	}

	auto& handlers = clients_.at(service->type());

	for(auto i = handlers.begin(); i != handlers.end(); ++i) {
		if(*i == service) {
			handlers.erase(i);
			break;
		}
	}

	if(handlers.empty()) {
		clients_.erase(service->type());
	}
}

void HandlerRegistry::register_service(Handler* service) {
	std::lock_guard<std::mutex> guard(mutex_);
	auto type = service->type();
	services_[type].emplace_back(service);
}

void HandlerRegistry::register_client(Handler* client) {
	std::lock_guard<std::mutex> guard(mutex_);
	auto type = client->type();
	clients_[type].emplace_back(client);
}

std::vector<Handler*> HandlerRegistry::services(const std::string& name) const {
	std::lock_guard<std::mutex> guard(mutex_);
	return services_.at(name);
}

std::vector<Handler*> HandlerRegistry::clients(const std::string& name) const {
	std::lock_guard<std::mutex> guard(mutex_);
	return clients_.at(name);
}

std::vector<std::string> HandlerRegistry::services() const {
	std::lock_guard<std::mutex> guard(mutex_);
	std::vector<std::string> types;

	for(auto& [k, _]: services_) {
		types.emplace_back(k);
	}

	return types;
}

std::vector<std::string> HandlerRegistry::clients() const {
	std::lock_guard<std::mutex> guard(mutex_);
	std::vector<std::string> types;

	for(auto& [k, _]: clients_) {
		types.emplace_back(k);
	}

	return types;
}


} // v2, spark, ember