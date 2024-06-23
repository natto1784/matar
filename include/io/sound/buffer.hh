#pragma once

#include <util/spsc.hh>

namespace matar {
namespace sound {

template<typename T>
struct Sample {
    T left;
    T right;
};

/* 16 bit samples */
using AudioSample = Sample<int16_t>;
using AudioBuffer = SPSCBuffer<AudioSample>;

}
}
