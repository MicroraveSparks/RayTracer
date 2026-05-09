#ifndef USER_H
#define USER_H

const unsigned char* hash_password(const char* password);

struct User {
  const char* username;
  const unsigned char* password_hash;
  bool admin = false;
};

#endif // USER_H