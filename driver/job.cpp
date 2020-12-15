#include "job.hpp"

#include "spdlog/spdlog.h"

tasker::Job::Job(std::string job_id, std::string client_id,
                 std::shared_ptr<tasker::Driver> driver) : job_id(job_id), client_id(client_id), driver(driver) {
}

std::string &tasker::Job::GetId() {
    return this->job_id;
}

bool tasker::Job::Progress() {
    spdlog::info("Called super progress");
    return true;
}

void tasker::Job::Finalize() {
    // nothing will be done
}

void tasker::Job::OnWorkerMessage(std::string &worker_id, std::string &msg) {
    // nothing will be done
}

void tasker::Job::OnWorkerRevoked(std::string &worker_id) {
    // nothing will be done
}