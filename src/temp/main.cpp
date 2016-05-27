#include <wsscheduler/Scheduler.h>
#include <logger/Logging.h>
#include <logger/ConsoleSink.h>
#include <memory>
#include <thread>

namespace ts = ember::task::ws;
using namespace ember;

TASK_ENTRY_POINT(job_one) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
	std::this_thread::sleep_for(std::chrono::seconds(10));
}

TASK_ENTRY_POINT(job_two) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
}

TASK_ENTRY_POINT(job_three) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
}

TASK_ENTRY_POINT(root_job) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
	std::this_thread::sleep_for(std::chrono::seconds(10));
	auto task = scheduler.create_task(job_one);
	scheduler.run(task);
	scheduler.wait(task);
	//scheduler->run_job(job_one);
	//scheduler->run_job(job_two);
	//scheduler->run_job(job_three);
}

TASK_ENTRY_POINT(test_task) {
	LOG_INFO_GLOB << *(int*)arg << LOG_SYNC;
}

int main() {
	// set basic logger up
	auto logger = std::make_unique<log::Logger>();
	auto sink = std::make_unique<log::ConsoleSink>(log::Severity::INFO, log::Filter(0));
	sink->colourise(true);
	logger->add_sink(std::move(sink));
	log::set_global_logger(logger.get());

	// task testing stuff
	auto cores = std::thread::hardware_concurrency();

	ts::Scheduler scheduler(cores - 1, logger.get());
	auto task = scheduler.create_task(root_job);
	scheduler.run(task);
	scheduler.wait(task);
}