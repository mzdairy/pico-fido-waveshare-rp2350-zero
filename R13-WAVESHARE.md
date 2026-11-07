# Waveshare RP2350-Zero — r13 final

## 中文使用说明

本仓库的 `r13-final` 分支是 Waveshare（微雪）RP2350-Zero 的 r13 最终定制版，提供 FIDO2/WebAuthn 安全密钥功能，不是上游官方发行版。不要直接用于其他型号或引脚配置不同的开发板。本构建关闭 OATH、OTP 应用；FIDO2 不等于 U 盘存储功能。

### 下载与刷写

1. 打开 [r13 固件文件](firmware/pico_fido_zero_fido2_v8.0_FINAL-r13.uf2)，点击 **Download raw file** 下载 UF2，而不是保存网页。
2. 先保存已绑定账号的恢复码，并准备其他登录方式。不要假定升级一定保留已有凭据；不要对已绑定的密钥随意清空或重置。
3. 按住开发板 **BOOT**，用支持数据传输的 USB 线连接电脑，出现 RP2350 启动盘后松开。
4. 将 UF2 复制到启动盘，等待复制完成及开发板重启。不要在复制过程中拔掉设备。
5. 正常运行时，系统应将其作为安全密钥/HID 设备使用，不再显示启动盘。若出现“未知 USB 设备”，先检查数据线和接口，不要继续绑定重要账号。

本次文档更新没有重新编译或更改固件，仍使用下面 SHA256 对应的 r13 文件。

### 最终灯效

以下是亮 / 灭时间，不是单次闪烁的总时长。

| 状态 | 颜色 | 亮 / 灭 | 周期或次数 |
| --- | --- | --- | --- |
| 普通闲置 | 绿色 | 0.5 秒 / 4.5 秒 | 5 秒循环 |
| PIN 验证准备 | 红色 | 0.1 秒 / 2.9 秒 | 3 秒循环 |
| 等待 BOOT 确认 | 红色 | 0.1 秒 / 0.9 秒 | 1 秒循环 |
| 处理操作中 | 红色 | 0.05 秒 / 0.05 秒 | 连续快闪 |
| 操作完成 / 维护完成提示 | 红色 | 0.1 秒 / 0.1 秒 | 仅快速闪 3 下 |
| USB 挂起 | 蓝色 | 1 秒 / 2 秒 | 主机仍供电并运行调度时的配置 |

PIN 准备状态根据 FIDO 协议活动推断，无法直接读取电脑是否弹出 PIN 窗口；提示有超时限制，可能与窗口出现、关闭的时刻不完全同步。操作时按主机提示短按 BOOT 确认，不要为了确认操作按复位键。以上颜色包含对本板实际观察到的红绿通道颠倒的修正。

### PIN 与账号绑定

PIN 是安全密钥本身的 PIN，不是网站登录密码。不要假定已有默认 PIN；首次使用时按系统提示设置，已有 PIN 时使用原 PIN。Windows 可通过“设置 → 账户 → 登录选项 → 安全密钥 → 管理”进行管理，入口名称可能随系统版本变化。

GitHub 可在 Settings → Password and authentication 中添加 Passkey 或两步验证安全密钥，按提示选择 USB 安全密钥、输入 PIN 并按 BOOT。先测试新密钥能正常登录，保留恢复码和备用验证方式，勿将此 DIY 开发板作为唯一登录途径。

忘记 PIN 不等于可以找回 PIN；重置安全密钥通常会删除凭据。首次初始化中，上游 RP2350 代码可能写入不可逆 OTP 设置；不要随意启用安全启动、锁定或执行维护命令。DIY 固件不代表取得商用安全密钥认证。

### 源码与验证范围

修改过的 Pico Keys SDK 已作为普通源码纳入仓库，保留原始许可证和版权信息。编译入口为 `build-zero-fido2.sh`，依赖与基线提交见下方英文记录。仅修改文档不会改变固件灯效。

已完成 UF2 结构检查及主机侧灯效时序测试；助手未连接实物验证。具体电脑上的 USB、按键及账号兼容性仍须实际测试。

---

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
