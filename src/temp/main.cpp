#include <wsscheduler/Scheduler.h>
#include <iostream>
#include <thread>

namespace ts = ember::task::ws;


TASK_ENTRY_POINT(job_one) {
	std::cout << "One\n";
}

TASK_ENTRY_POINT(job_two) {
	std::cout << "Two\n";
}

TASK_ENTRY_POINT(job_three) {
	std::cout << "Three\n";
}

TASK_ENTRY_POINT(root_job) {
	std::cout << "Root start\n";
	scheduler->run_job(job_one);
	scheduler->run_job(job_two);
	scheduler->run_job(job_three);
	std::cout << "Root end\n";
}

int main() {
	auto cores = std::thread::hardware_concurrency();

	ts::Scheduler scheduler(cores - 1);
	scheduler.run_job(root_job);
}