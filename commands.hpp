#ifndef C37E4C99_F798_4490_A00F_CD48E100A69D
#define C37E4C99_F798_4490_A00F_CD48E100A69D
#include <command.hpp>
#include <cxxopts.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "spdlog/spdlog.h"

std::string create_response(int32_t error_code, std::string msg = "") {
    std::string rsp;
    rsp.append("MSG ");
    rsp.append(std::to_string(error_code));
    if (msg.size() > 0) {
        rsp.append(" ");
        rsp.append(msg);
    }
    return rsp;
}

std::string get_root() {
    std::string root_dir = "";
    if (std::getenv("ENV_ROOT_PATH") != nullptr) {
        root_dir = std::getenv("ENV_ROOT_PATH");
    }
    return root_dir;
}

void tokenize(std::string &cmd, std::shared_ptr<std::vector<const char *>> &args) {
    std::istringstream commnad(cmd);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{commnad},
                                    std::istream_iterator<std::string>{}};

    args = std::make_shared<std::vector<const char *>>(tokens.size());
    std::transform(tokens.begin(), tokens.end(), args->begin(), [](std::string &tkn) {
        return tkn.c_str();
    });
}

class PartitionCommand : tasker::Command {
   private:
    std::string src_file;
    std::string dst_folder;
    int32_t partitions;

   public:
    PartitionCommand(std::string &cmd) : tasker::Command(cmd) {
    }

    void parse() {
        std::shared_ptr<std::vector<const char *>> args;
        tokenize(this->command, args);

        cxxopts::Options options("Parition", "Parition Command Handler");
        options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("s,source", "Source file", cxxopts::value<std::string>())("d,destination", "Destination folder", cxxopts::value<std::string>());

        auto results = options.parse(args->size(), args->data());

        std::string root_dir = get_root();

        this->src_file = root_dir + results["s"].as<std::string>();
        this->dst_folder = root_dir + results["d"].as<std::string>();
        this->partitions = results["p"].as<std::int32_t>();
    }

    void validate(int32_t *code, std::string *msg) {
        // check source file exists
        if (!std::filesystem::exists(src_file)) {
            *msg = create_response(404, "File " + src_file + " doesn't exists");
            *code = 404;
        } else {
            *code = 0;

            spdlog::info("Creating output directories {}", dst_folder);
            std::filesystem::create_directories(dst_folder);
        }
    }

    std::string &GetSrcFile() {
        return this->src_file;
    }

    std::string &GetDstFolder() {
        return this->dst_folder;
    }

    int32_t &GetPartitions() {
        return this->partitions;
    }
};

void parse_parition_command(std::string &cmd, std::string &root_dir,
                            std::string *src_file,
                            std::string *dst_folder, int32_t *partitions) {
    std::shared_ptr<std::vector<const char *>> args;
    tokenize(cmd, args);

    cxxopts::Options options("Parition", "Parition Command Handler");
    options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("s,source", "Source file", cxxopts::value<std::string>())("d,destination", "Destination folder", cxxopts::value<std::string>());

    auto results = options.parse(args->size(), args->data());

    *src_file = root_dir + results["s"].as<std::string>();
    *dst_folder = root_dir + results["d"].as<std::string>();
    *partitions = results["p"].as<std::int32_t>();
}
#endif /* C37E4C99_F798_4490_A00F_CD48E100A69D */
