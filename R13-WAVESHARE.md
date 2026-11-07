# Waveshare RP2350-Zero — r13 final

Custom FIDO2 firmware; not an official upstream release. Target: Waveshare RP2350-Zero only.

Firmware: `firmware/pico_fido_zero_fido2_v8.0_FINAL-r13.uf2`

SHA256: `9302ab143bb5dfba7dbd80d1e0264af686ff27d4d38a072a8bb1a955e438dcf0`

| State | Color | On / off |
| --- | --- | --- |
| Idle | Green | 0.5 / 4.5 seconds |
| PIN preparation | Red | 0.1 / 2.9 seconds |
| Waiting for BOOT confirmation | Red | 0.1 / 0.9 seconds |
| Processing | Red | 0.05 / 0.05 seconds |
| Completion / maintenance completion | Red | Three flashes, 0.1 / 0.1 seconds |

PIN preparation is inferred from protocol activity, not direct observation of the host PIN dialog.

## Source and build

Pico Fido base: `6ffcb81191c7f4e44337e0eaecc788ba6db03b75`.
Pico Keys SDK base: `7c5f4d9b95d5c17531a45c74c673822242d4d376`.
The modified Pico Keys SDK is included as ordinary source files, not a submodule.
Original copyright notices and license files are preserved.

Install CMake, Ninja, ARM GNU toolchain and Raspberry Pi Pico SDK 2.2.0.
Set `PICO_SDK_PATH` to that SDK, then run `bash build-zero-fido2.sh`.
Dependencies fetched by the SDK include mbedTLS and TinyCBOR; the original build used
mbedTLS `fc39404125e93ee00e6758605e6c6f2c8db440f1` and
TinyCBOR `c0aad2fb2137a31b9845fbaae3653540c410f215`.

The adaptation includes USB descriptor compatibility fixes, board-specific LED color correction,
and the r13 LED state timing. OATH and OTP applications are disabled in the build script.

UF2 structure and host-side LED timing tests passed. No physical device was attached during assistant validation.
Back up account recovery methods before flashing. Firmware replacement/reset may affect stored credentials;
upstream RP2350 initialization can write OTP settings that cannot be undone. Do not enable secure boot or lock options casually.
