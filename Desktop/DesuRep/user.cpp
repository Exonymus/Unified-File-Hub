#include "user.h"

User::User(const User::Data &userData)
    : data(userData)
{

}

User::User() {

}

User::~User() { }

void User::setData(Data set_data)
{
    data.id = set_data.id;
    data.username = set_data.username;
    data.email = set_data.email;
    data.is_actual = set_data.is_actual;
    data.is_banned = set_data.is_banned;
    data.on_active = set_data.on_active;
    data.role = set_data.role;
}
