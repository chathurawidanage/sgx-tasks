#ifndef C31578BB_94F7_4CB6_92CB_D3384BC4AE3C
#define C31578BB_94F7_4CB6_92CB_D3384BC4AE3C
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <random>
#include <string>

using namespace std;

// todo this has to be replaced with a proper uuid generator

string gen_random(const int length) {
    static const std::string allowed_chars{"123456789abcdfghjklmnpqrstvwxz"};

    static thread_local std::default_random_engine randomEngine(std::random_device{}());
    static thread_local std::uniform_int_distribution<int> randomDistribution(0, allowed_chars.size() - 1);

    std::string id(length ? length : 32, '\0');

    for (std::string::value_type& c : id) {
        c = allowed_chars[randomDistribution(randomEngine)];
    }

    return id;
}
#endif /* C31578BB_94F7_4CB6_92CB_D3384BC4AE3C */
