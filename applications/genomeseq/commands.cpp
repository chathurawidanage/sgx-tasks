#include "commands.hpp"

#include <cxxopts.hpp>
#include <filesystem>

#include "spdlog/spdlog.h"

std::string create_response(int32_t error_code, std::string msg) {
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
        // TODO: creating a copy, change this
        auto str = new std::string();
        *str = tkn;
        return str->c_str();
    });
}

/*Partition Command*/
void PartitionCommand::Validate(int32_t *code, std::string *msg) {
    // check source file exists
    if (!std::filesystem::exists(src_file)) {
        *msg = create_response(404, "File " + src_file + " doesn't exists");
        *code = 404;
    } else {
        *code = 0;

        spdlog::info("Creating output directories {}", this->dst_folder);
        std::filesystem::create_directories(this->dst_folder);
    }
}

void PartitionCommand::Parse(int32_t *code, std::string *msg) {
    std::shared_ptr<std::vector<const char *>> args;
    tokenize(this->command, args);

    cxxopts::Options options("prt", "Parition Command Handler");
    options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("s,source", "Source file", cxxopts::value<std::string>())("d,destination", "Destination folder", cxxopts::value<std::string>());

    auto results = options.parse(args->size(), args->data());

    spdlog::info("Args parsed : {} {} {}", results["s"].as<std::string>(), results["p"].as<std::int32_t>(), results["d"].as<std::string>());

    std::string root_dir = get_root();

    this->src_file = root_dir + results["s"].as<std::string>();
    this->dst_folder = root_dir + results["d"].as<std::string>();
    this->partitions = results["p"].as<std::int32_t>();

    this->Validate(code, msg);
}

/*Index Command*/
void IndexCommand::Validate(int32_t *code, std::string *msg) {
    // check source file exists
    if (!std::filesystem::exists(src_file)) {
        *msg = create_response(404, "File " + src_file + " doesn't exists");
        *code = 404;
    } else {
        *code = 0;
    }
}

void IndexCommand::Parse(int32_t *code, std::string *msg) {
    std::shared_ptr<std::vector<const char *>> args;
    tokenize(this->command, args);

    cxxopts::Options options("idx", "Index Command Handler");
    options.add_options()("s,source", "Source file", cxxopts::value<std::string>());

    spdlog::info("Parsing command {}", this->command);
    auto results = options.parse(args->size(), args->data());

    std::string root_dir = get_root();

    this->src_file = root_dir + results["s"].as<std::string>();
    this->Validate(code, msg);
}

/*Clinet Index Command*/
void ClientIndexCommand::Validate(int32_t *code, std::string *msg) {
    // check source file exists
    if (!std::filesystem::exists(src_file)) {
        *msg = create_response(404, "File " + src_file + " doesn't exists");
        *code = 404;
    } else {
        *code = 0;
    }
}

void ClientIndexCommand::Parse(int32_t *code, std::string *msg) {
    std::shared_ptr<std::vector<const char *>> args;
    tokenize(this->command, args);

    cxxopts::Options options("index", "Index Command Handler");
    options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("s,source", "Source file", cxxopts::value<std::string>());

    auto results = options.parse(args->size(), args->data());

    std::string root_dir = get_root();

    this->src_file = root_dir + results["s"].as<std::string>();
    this->relative_src_file = results["s"].as<std::string>();
    this->partitions = results["p"].as<std::int32_t>();
    this->Validate(code, msg);
}

/* Dispatch Command */
void DispatchCommand::Validate(int32_t *code, std::string *msg) {
    *code = 0;

    spdlog::info("Creating destination folder {}", this->destination);
    std::filesystem::create_directories(this->destination);

    // TODO: copy max inf from one place to another

    // index exists
    // if (!std::filesystem::exists(this->GetIndexFolder())) {
    //     *code = 404;
    //     *msg = "The index " + this->index_id + " doesn't exists";
    //     return;
    // }
}

void DispatchCommand::Parse(int32_t *code, std::string *msg) {
    std::shared_ptr<std::vector<const char *>> args;
    tokenize(this->command, args);

    cxxopts::Options options("dsp", "Dispatch Command Handler");
    options.add_options()("p,partitions", "No of partitions", cxxopts::value<int32_t>())("b,bmer", "Size of su", cxxopts::value<std::int32_t>())("i,index", "Index ID", cxxopts::value<std::string>())("s,source", "Source File", cxxopts::value<std::vector<std::string>>())("d,destination", "Destination Folder", cxxopts::value<std::string>());

    auto results = options.parse(args->size(), args->data());

    std::string root_dir = get_root();

    this->bmer = results["b"].as<std::int32_t>();
    this->pnum = results["p"].as<std::int32_t>();
    this->index_id = results["i"].as<std::string>();
    this->destination = root_dir + results["d"].as<std::string>();

    auto inputs = results["s"].as<std::vector<std::string>>();
    this->input1 = root_dir + inputs[0];
    if (inputs.size() > 1) {
        this->se = 0;
        this->input2 = root_dir + inputs[1];
    }
    this->Validate(code, msg);
}

std::string DispatchCommand::GetIndexFolder() {
    return get_root() + "/" + this->index_id;
}