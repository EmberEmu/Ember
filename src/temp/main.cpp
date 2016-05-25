#include <wsscheduler/Scheduler.h>
#include <logger/Logging.h>
#include <logger/ConsoleSink.h>
#include <memory>
#include <thread>

namespace ts = ember::task::ws;
using namespace ember;

TASK_ENTRY_POINT(job_one) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
}

TASK_ENTRY_POINT(job_two) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
}

TASK_ENTRY_POINT(job_three) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
}

TASK_ENTRY_POINT(root_job) {
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
	//scheduler->run_job(job_one);
	//scheduler->run_job(job_two);
	//scheduler->run_job(job_three);
	LOG_INFO_GLOB << __func__ << LOG_SYNC;
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

	//scheduler.run_job(root_job, counter);

	int test_data[100] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	ts::Task tasks[100];
	ts::Counter counter = 100;

	for(int i = 0; i < 100; ++i) {
		tasks[i] = ts::Task{test_task, &test_data[i]};
	}

	scheduler.run_jobs(tasks, 100, counter);
	//scheduler.wait(counter);
}