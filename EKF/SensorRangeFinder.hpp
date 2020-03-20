/****************************************************************************
 *
 *   Copyright (c) 2020 Estimation and Control Library (ECL). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name ECL nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file SensorRangeFinder.hpp
 * Range finder class containing all the required checks
 *
 * @author Mathieu Bresciani <brescianimathieu@gmail.com>
 *
 */
#pragma once

#include "Sensor.hpp"
#include <matrix/math.hpp>

using estimator::rangeSample;
using matrix::Dcmf;

class SensorRangeFinder : public Sensor
{
public:
	SensorRangeFinder() = default;
	~SensorRangeFinder() override = default;

	void runChecks(uint64_t time_delayed_us, const Dcmf &R_to_earth);
	bool isHealthy() const override { return _rng_hgt_valid; }
	bool isNewHealthyData() const override { return _range_data_ready && _rng_hgt_valid; }
	bool isDelayedHealthyData() const override { return _range_data_ready && _rng_hgt_valid; }
	bool canBeusedAsFailover() const override;
	bool canResetOnSensor() const override;

	void setNewestSample(rangeSample sample) { _newest_sample = sample; }
	void setDelayedSample(rangeSample sample) {
		_range_sample_delayed = sample;
		_range_data_ready = true;
	}

	void setTilt(float new_tilt, float range_cos_max_tilt)
	{
		if (fabsf(_tilt - new_tilt) > FLT_EPSILON) {
			_sin_tilt_rng = sinf(new_tilt);
			_cos_tilt_rng = cosf(new_tilt);
		}
		_range_cos_max_tilt = range_cos_max_tilt;
	};

	void setLimits(float min_distance, float max_distance) {
		_rng_valid_min_val = min_distance;
		_rng_valid_max_val = max_distance;
	};

	float getRToEarth() const { return _R_rng_to_earth_2_2; }

	void setDelayedRng(float rng) { _range_sample_delayed.rng = rng; }
	float getDelayedRng() const { return _range_sample_delayed.rng; }

	void setDataReadiness(bool is_ready) { _range_data_ready = is_ready; }
	void setValidity(bool is_valid) { _rng_hgt_valid = is_valid; }

	bool isStuck() const { return _is_stuck; }
	bool isTiltOk() const { return _R_rng_to_earth_2_2 > _range_cos_max_tilt; }

	bool getValidMinVal() const { return _rng_valid_min_val; }
	bool getValidMaxVal() const { return _rng_valid_max_val; }

	// This is required because of the ring buffer
	rangeSample* getSampleDelayedAddress() { return &_range_sample_delayed; }

private:
	void updateRangeDataValidity(uint64_t time_delayed_us);
	void updateRangeDataContinuity(uint64_t time_delayed_us);
	bool isRangeDataContinuous() const { return _dt_last_range_update_filt_us < 2e6f; }
	void updateRangeDataStuck();

	rangeSample _newest_sample{};
	rangeSample _range_sample_delayed{};
	bool _range_data_ready{false}; ///< true when new range finder data has fallen behind the fusion time horizon and is available to be fused
	bool _rng_hgt_valid{false}; ///< true if range finder sample retrieved from buffer is valid
	bool _is_stuck{};

	float _dt_last_range_update_filt_us{}; ///< filtered value of the delta time elapsed since the last range measurement came into the filter (uSec)
	uint64_t _time_last_rng_ready{};	///< time the last range finder measurement was ready (uSec)
	uint64_t _time_bad_rng_signal_quality{}; ///< timestamp at which range finder signal quality was 0 (used for hysteresis)
	float _rng_stuck_min_val{0.0f};		///< minimum value for new rng measurement when being stuck
	float _rng_stuck_max_val{0.0f};		///< maximum value for new rng measurement when being stuck

	static constexpr float _dt_update{0.01f}; ///< delta time since last ekf update TODO: this should be a parameter

	float _tilt{};
	float _sin_tilt_rng{0.0f}; ///< sine of the range finder tilt rotation about the Y body axis
	float _cos_tilt_rng{0.0f}; ///< cosine of the range finder tilt rotation about the Y body axis

	float _R_rng_to_earth_2_2{0.0f};	///< 2,2 element of the rotation matrix from sensor frame to earth frame

	float _rng_valid_min_val{0.0f};	///< minimum distance that the rangefinder can measure (m)
	float _rng_valid_max_val{0.0f};	///< maximum distance that the rangefinder can measure (m)

	float _range_cos_max_tilt{0.7071f};	///< cosine of the maximum tilt angle from the vertical that permits use of range finder and flow data
	float _range_stuck_threshold{0.1f};	///< minimum variation in range finder reading required to declare a range finder 'unstuck' when readings recommence after being out of range (m)
	uint64_t _range_signal_hysteresis_ms{1000}; 	///< minimum duration during which the reported range finder signal quality needs to be non-zero in order to be declared valid (ms)
};
