#pragma once
#include "Singleton.h"

namespace Framework
{
	class TimeManager :
		Singleton<TimeManager>
	{
	public:
		static inline float GetDeltaTime() { return Inst().mDeltaTime; }
		static inline float GetRawDeltaTime() { return Inst().mRawDeltaTime; }
		static inline float GetTimeScale() { return Inst().mTimescale; }
		static inline float GetTotalTimePassed() { return Inst().mTotalTimePassed; }
		static inline uint GetCurrentFrame() { return Inst().mCurrentFrame; }

		static inline void SetTimeScale(float scale) { Inst().mTimescale = scale; }

		static inline void UpdateDeltaTime(float rawDeltaTime)
		{
			TimeManager& inst = Inst();
			rawDeltaTime = std::min(rawDeltaTime, inst.sMaxDeltaTime);
			inst.mDeltaTime = rawDeltaTime * inst.mTimescale;
			inst.mRawDeltaTime = rawDeltaTime;
			inst.mTotalTimePassed += inst.mDeltaTime;

			++inst.mCurrentFrame;
		}

	private:
		static constexpr float sMaxDeltaTime = 10.0f;

		float mTimescale = 1.0f;

		float mDeltaTime{};
		float mRawDeltaTime{};
		float mTotalTimePassed{};

		uint mCurrentFrame{};
	};
}