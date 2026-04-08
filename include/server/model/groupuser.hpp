#pragma once

#include "user.hpp"

class GroupUser : public User {
public:
    GroupUser(int id, std::string name, std::string state, std::string role = "normal") : User(id, name, "", state) , role(role) {}
    void setRole(std::string role) {this->role = role;}
    std::string getRole() {return role;}
private:
    std::string role;
};