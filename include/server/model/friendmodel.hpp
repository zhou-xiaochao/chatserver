#pragma once

#include <user.hpp>
#include <vector>

class FriendModel {
public:
    void insert(int userid, int friendid);

    std::vector<User> query(int userid);
};