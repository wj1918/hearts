/*
 * $Id: timer.cpp,v 1.6 2006/11/30 20:33:25 nathanst Exp $
 *
 * timer.cpp
 * HOG file
 *
 * Written by Renee Jansen on 08/28/06
 *
 * This file is part of HOG.
 *
 * HOG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HOG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HOG; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//#include "UnitSimulation.h"
#include "Timer.h"
#include <stdint.h>
#include <cstring>
#include <atomic>
#include <mutex>

namespace hearts {

Timer::Timer()
{
	elapsedTime = 0;
}

void Timer::StartTimer()
{
#ifdef OS_MAC
	startTime = UpTime();
#elif defined( TIMER_USE_CYCLE_COUNTER )
	CycleCounter c;
	startTime = c.count();
#elif defined(_WIN32)
	startTime = std::chrono::high_resolution_clock::now();
#else
	gettimeofday( &startTime, NULL );
#endif
}

#ifdef linux

// Thread-safe CPU speed caching
static std::atomic<float> g_cpuSpeedCache{-1.0f};
static std::mutex g_cpuSpeedMutex;

float Timer::getCPUSpeed()
{
	// Fast path: check if already cached
	float cached = g_cpuSpeedCache.load(std::memory_order_acquire);
	if (cached >= 0.0f)
		return cached;

	// Slow path: compute and cache (with mutex to prevent redundant reads)
	std::lock_guard<std::mutex> lock(g_cpuSpeedMutex);

	// Double-check after acquiring lock
	cached = g_cpuSpeedCache.load(std::memory_order_relaxed);
	if (cached >= 0.0f)
		return cached;

	FILE *f = fopen("/proc/cpuinfo", "r");
	if (f)
	{
		while (!feof(f))
		{
			char entry[1024];
			char temp[1024];
			fgets(entry, 1024, f);
			if (strstr(entry, "cpu MHz"))
			{
				float speed;
				sscanf(entry, "%[^0-9:] : %f", temp, &speed);
				fclose(f);
				g_cpuSpeedCache.store(speed, std::memory_order_release);
				return speed;
			}
		}
		fclose(f);
	}

	g_cpuSpeedCache.store(0.0f, std::memory_order_release);
	return 0.0f;
}

#endif

double Timer::EndTimer()
{
#ifdef OS_MAC
	AbsoluteTime stopTime = UpTime();
	Nanoseconds diff = AbsoluteDeltaToNanoseconds(stopTime, startTime);
	uint64_t nanosecs = UnsignedWideToUInt64(diff);
	//cout << nanosecs << " ns elapsed (" << (double)nanosecs/1000000.0 << " ms)" << endl;
	return elapsedTime = (double)(nanosecs/1000000000.0);
#elif defined( TIMER_USE_CYCLE_COUNTER )
	Timer::CycleCounter c;
	double diffTime = (double)(c.count() - startTime);
	const static double ClocksPerSecond = getCPUSpeed() * 1000000.0;
	elapsedTime = diffTime / ClocksPerSecond;
	return elapsedTime;
#elif defined(_WIN32)
	auto stopTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime);
	elapsedTime = duration.count() / 1000000000.0;
	return elapsedTime;
#else
	struct timeval stopTime;

	gettimeofday( &stopTime, NULL );
	uint64_t microsecs = stopTime.tv_sec - startTime.tv_sec;
	microsecs = microsecs * 1000000 + stopTime.tv_usec - startTime.tv_usec;
	elapsedTime = (double)microsecs / 1000000.0;
	return elapsedTime;
#endif
}

} // namespace hearts
