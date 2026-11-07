#!/usr/bin/env bash
set -euo pipefail
# Run from this source directory; put CMake, Ninja and ARM GCC on PATH.
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to Raspberry Pi Pico SDK 2.2.0}"
cmake -S . -B build-zero -G Ninja \
  -DPICO_SDK_PATH="$PICO_SDK_PATH" \
  -DPICO_BOARD=waveshare_rp2350_zero \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_OATH_APP=OFF -DENABLE_OTP_APP=OFF \
  -DFORCE_BUTTON_WAIT=ON -DENABLE_EDDSA=ON \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build-zero --parallel 4
