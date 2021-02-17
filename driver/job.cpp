#include "job.hpp"
#include "messages.hpp"

#include "executor.hpp"
#include "spdlog/spdlog.h"

tasker::Job::Job(std::string job_id, std::string client_id, std::shared_ptr<tasker::Driver> driver) : job_id(job_id), client_id(client_id), driver(driver) {
    this->job_scheduled_time = std::chrono::high_resolution_clock::now();
    this->job_name = job_id;
}

void tasker::Job::NotifyCompletion(int32_t code, std::string msg) {
    auto now = std::chrono::high_resolution_clock::now();
    long total_time = std::chrono::duration_cast<std::chrono::seconds>(now - this->job_scheduled_time).count();
    long processing_time = std::chrono::duration_cast<std::chrono::seconds>(now - this->worker_start_time).count();

    this->driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::UPDATE), "Job [" + this->GetName() + "] completed. Total time : " + std::to_string(total_time) + "sec. Processng Time : " + std::to_string(processing_time) + "sec.");

    for (auto cb : this->on_complete_cbs) {
        (*(cb))(this->job_id, code, msg);
    }
    this->reported_code = code;
    this->reported_msg = msg;
    this->completed = true;

}

void tasker::Job::NotifyWorkerAllocated() {
    this->worker_start_time = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    long wait_time_for_allocation = std::chrono::duration_cast<std::chrono::seconds>(now - this->job_scheduled_time).count();
    this->driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::UPDATE), "Worker allocated for [" + this->GetName() + "] after " + std::to_string(wait_time_for_allocation) + "sec");
}

bool tasker::Job::IsCompleted() { return this->completed; }

void tasker::Job::OnComplete(std::shared_ptr<std::function<void(std::string, int32_t, std::string)>> cb) {
    if (this->completed) {
        (*cb)(this->job_id, this->reported_code, this->reported_msg);
    }
    this->on_complete_cbs.push_back(cb);
}

void tasker::Job::SetName(std::string name) { this->job_name = name; }

std::string tasker::Job::GetName() { return this->job_name; }

std::string &tasker::Job::GetId() { return this->job_id; }

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
        auto now = std::chrono::high_resolution_clock::now();
        long total_time = std::chrono::duration_cast<std::chrono::seconds>(now - this->jobs_scheduled_time).count();
        this->driver->SendToClient(this->client_id, tasker::GetCommand(tasker::Commands::UPDATE), "Jobs group [" + this->jobs_name + "] completed in  " + std::to_string(total_time) + "sec");

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
    job->OnComplete(std::make_shared<std::function<void(std::string, int32_t, std::string)>>([&](std::string job_id, int32_t code, std::string msg) { this->ReportJobCompletion(job_id, code, msg); }));
}