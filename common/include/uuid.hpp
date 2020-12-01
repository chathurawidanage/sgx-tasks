#ifndef C31578BB_94F7_4CB6_92CB_D3384BC4AE3C
#define C31578BB_94F7_4CB6_92CB_D3384BC4AE3C
#include <unistd.h>

#include <ctime>
#include <iostream>

using namespace std;

// todo this has to be replaced with a proper uuid generator

string gen_random(const int len) {
    string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz";

    srand((unsigned)time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}
#endif /* C31578BB_94F7_4CB6_92CB_D3384BC4AE3C */
