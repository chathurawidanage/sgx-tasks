#include "job.hpp"

#include "executor.hpp"
#include "spdlog/spdlog.h"

tasker::Job::Job(std::string job_id, std::string client_id,
                 std::shared_ptr<tasker::Driver> driver) : job_id(job_id), client_id(client_id), driver(driver) {
}

void tasker::Job::NotifyCompletion(int32_t code, std::string msg) {
    for (auto cb : this->on_complete_cbs) {
        (*(cb))(this->job_id, code, msg);
    }
    this->completed = true;
}

bool tasker::Job::IsCompleted() {
    return this->completed;
}

void tasker::Job::OnComplete(std::shared_ptr<std::function<void(std::string, int32_t, std::string)>> cb) {
    this->on_complete_cbs.push_back(cb);
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

void tasker::Jobs::ReportJobCompletion(std::string job_id, int32_t code, std::string msg) {
    if (code != 0) {
        this->failed_count++;
        this->latest_code = code;
        this->latest_msg = msg;
    }
    this->completions++;
    (*this->on_job_completed)(job_id, code, msg, this->failed_count, this->completions);
    if (completions == this->jobs.size()) {
        (*this->on_all_completed)(this->failed_count, this->latest_code, this->latest_msg);
    }
}

void tasker::Jobs::Execute() {
    for (auto job : this->jobs) {
        driver->GetExecutor()->AddJob(job);
    }
}

void tasker::Jobs::AddJob(std::shared_ptr<tasker::Job> job) {
    this->jobs.push_back(job);
    job->OnComplete(std::make_shared<std::function<void(std::string, int32_t, std::string)>>([&](std::string job_id, int32_t code, std::string msg) {
        this->ReportJobCompletion(job_id, code, msg);
    }));
}