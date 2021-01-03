#ifndef FD60FBAC_2128_4D00_A89D_54196ADD6845
#define FD60FBAC_2128_4D00_A89D_54196ADD6845

#include <string>

void decode_response(std::string &rsp, std::string *cmd, int32_t *error_code, std::string *msg) {
    *cmd = rsp.substr(0, 3);
    std::string err_code = rsp.substr(4, rsp.find(' ', 5) - 4);
    *error_code = std::stoi(err_code);

    int32_t msg_start = 5 + err_code.size();
    if (msg_start < rsp.size()) {
        *msg = rsp.substr(msg_start, rsp.size());
    } else {
        *msg = "";
        if (*error_code != 0) {
            *msg = "Unknown error occurred. Error code " + err_code;
        } else {
            *msg = "";
        }
    }
}
#endif /* FD60FBAC_2128_4D00_A89D_54196ADD6845 */
