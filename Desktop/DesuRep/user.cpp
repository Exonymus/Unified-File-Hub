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
    data.secret_num = set_data.secret_num;
    data.secret_answer = set_data.secret_answer;
    data.is_banned = set_data.is_banned;
}
