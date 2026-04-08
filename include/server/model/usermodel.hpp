#pragma once

#include "user.hpp"

class UserModel {
public:
    bool insert(User& user);
    User query(std::string name);
    User query(int id);
    bool updateState(User& user);
    void resetState();
};