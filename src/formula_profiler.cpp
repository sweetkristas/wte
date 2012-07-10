#ifndef DISABLE_FORMULA_PROFILER

#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#if !defined(WIN32)
#include <sys/time.h>
#else
#include <time.h>
#endif // defined(WIN32)

#include "custom_object_type.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula_profiler.hpp"
#include "object_events.hpp"

namespace formula_profiler
{

event_call_stack_type event_call_stack;

namespace {
bool handler_disabled = false;
bool profiler_on = false;
std::string output_fname;
pthread_t main_thread;

int empty_samples = 0;

std::vector<custom_object_event_frame> event_call_stack_samples;
int num_samples = 0;
const size_t max_samples = 10000;

#ifdef WIN32
SDL_TimerID sdl_profile_timer;
#endif

#ifdef WIN32
Uint32 sdl_timer_callback(Uint32 interval, void *param)
#else
void sigprof_handler(int sig)
#endif
{
#ifdef WIN32
	if(handler_disabled) {
		return interval;
	}
#else
	if(handler_disabled || !pthread_equal(main_thread, pthread_self())) {
		return;
	}
#endif

	if(num_samples == max_samples) {
#ifdef WIN32
		return interval;
#else
		return;
#endif
	}

	if(event_call_stack.empty()) {
		++empty_samples;
	} else {
		event_call_stack_samples[num_samples++] = event_call_stack.back();
	}
#ifdef WIN32
	return interval;
#endif
}

}

manager::manager(const char* output_file)
{
	if(output_file) {
		event_call_stack_samples.resize(max_samples);

		main_thread = pthread_self();

		fprintf(stderr, "SETTING UP PROFILING...\n");
		profiler_on = true;
		output_fname = output_file;

#ifdef WIN32
		// Crappy windows approximation.
		sdl_profile_timer = SDL_AddTimer(10, sdl_timer_callback, 0);
		if(sdl_profile_timer == NULL) {
			std::cerr << "Couldn't create a profiling timer!" << std::endl;
		}
#else
		signal(SIGPROF, sigprof_handler);

		struct itimerval timer;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 10000;
		timer.it_value = timer.it_interval;
		setitimer(ITIMER_PROF, &timer, 0);
#endif
	}
}

manager::~manager()
{
	if(profiler_on){
#ifdef WIN32
		SDL_RemoveTimer(sdl_profile_timer);
#else
		struct itimerval timer;
		memset(&timer, 0, sizeof(timer));
		setitimer(ITIMER_PROF, &timer, 0);
#endif

		std::map<std::string, int> samples_map;


		for(int n = 0; n != num_samples; ++n) {
			const custom_object_event_frame& frame = event_call_stack_samples[n];

			std::string str = formatter() << frame.type->id() << ":" << get_object_event_str(frame.event_id) << ":" << (frame.executing_commands ? "CMD" : "FFL");

			samples_map[str]++;
		}

		std::vector<std::pair<int, std::string> > sorted_samples;
		for(std::map<std::string, int>::const_iterator i = samples_map.begin(); i != samples_map.end(); ++i) {
			sorted_samples.push_back(std::pair<int, std::string>(i->second, i->first));
		}

		std::sort(sorted_samples.begin(), sorted_samples.end());
		std::reverse(sorted_samples.begin(), sorted_samples.end());

		const int total_samples = empty_samples + num_samples;
		if(!total_samples) {
			return;
		}

		std::ostringstream s;
		s << "TOTAL SAMPLES: " << total_samples << "\n";
		s << (100*empty_samples)/total_samples << "% (" << empty_samples << ") CORE ENGINE (non-FFL processing)\n";

		for(int n = 0; n != sorted_samples.size(); ++n) {
			s << (100*sorted_samples[n].first)/total_samples << "% (" << sorted_samples[n].first << ") " << sorted_samples[n].second << "\n";
		}

		if(!output_fname.empty()) {
			sys::write_file(output_fname, s.str());
		} else {
			std::cerr << "===\n=== PROFILE REPORT ===\n";
			std::cerr << s.str();
			std::cerr << "=== END PROFILE REPORT ===\n";
		}
	}
}

bool custom_object_event_frame::operator<(const custom_object_event_frame& f) const
{
	return type < f.type || type == f.type && event_id < f.event_id ||
	       type == f.type && event_id == f.event_id && executing_commands < f.executing_commands;
}

std::string get_profile_summary()
{
	if(!profiler_on) {
		return "";
	}

	handler_disabled = true;

	static int last_empty_samples = 0;
	static int last_num_samples = 0;

	const int nsamples = num_samples - last_num_samples;
	const int nempty = empty_samples - last_empty_samples;

	std::sort(event_call_stack_samples.end() - nsamples, event_call_stack_samples.end());

	std::ostringstream s;

	s << "PROFILE: " << (nsamples + nempty) << " CPU. " << nsamples << " IN FFL ";


	std::vector<std::pair<int, std::string> > samples;
	int count = 0;
	for(int n = last_num_samples; n < num_samples; ++n) {
		if(n+1 == num_samples || event_call_stack_samples[n].type != event_call_stack_samples[n+1].type) {
			samples.push_back(std::pair<int, std::string>(count + 1, event_call_stack_samples[n].type->id()));
			count = 0;
		} else {
			++count;
		}
	}

	std::sort(samples.begin(), samples.end());
	std::reverse(samples.begin(), samples.end());
	for(int n = 0; n != samples.size(); ++n) {
		s << samples[n].second << " " << samples[n].first << " ";
	}

	last_empty_samples = empty_samples;
	last_num_samples = num_samples;

	handler_disabled = false;

	return s.str();
}

}

#endif
