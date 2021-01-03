#ifndef D1147A7C_FF42_4F4E_9F40_AAE8053CFAA5
#define D1147A7C_FF42_4F4E_9F40_AAE8053CFAA5
#include "dispatch_job.hpp"
#include "partition_job.hpp"
#include "spdlog/spdlog.h"

void HandleIndex(std::string msg, std::string client_id,
                 std::shared_ptr<tasker::Driver> driver) {
    spdlog::info("Handling parition commnad...");

    std::string job_id = gen_random(16);
    std::shared_ptr<PartitionJob> prt_job = std::make_shared<PartitionJob>(msg, job_id, client_id,
                                                                           driver);

    spdlog::info("Created parition job object");
    driver->GetExecutor()->AddJob(prt_job);
}

void HandleDispatch(std::string msg, std::string client_id,
                    std::shared_ptr<tasker::Driver> driver) {
    spdlog::info("Handling dispatch commnad...");
    std::string job_id = gen_random(16);
    std::shared_ptr<DispatchJob> dsp_job = std::make_shared<DispatchJob>(msg, job_id, client_id,
                                                                         driver);
    spdlog::info("Created dispatch job object");
    driver->GetExecutor()->AddJob(dsp_job);
}
#endif /* D1147A7C_FF42_4F4E_9F40_AAE8053CFAA5 */
