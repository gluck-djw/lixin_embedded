# Keil MDK 无法识别 SW Device 解决方案

## 🔍 问题现象

**在 Keil 中点击 Debug 按钮后：**
- J-Link 序列号能识别（SN: 69409324）
- 但是连接不到目标芯片（SW Device）
- 提示类似 "Can not connect to target" 或无法下载

---

## 🛠️ 解决步骤

### 步骤 1: 先用 J-Link Commander 测试连接

**目的**：确认目标板当前状态

```bash
# 打开 J-Link Commander
JLink.exe

# 尝试连接
connect
Device: STM32F411CE
TIF: SWD
Speed: 4000
```

**如果连接失败**：
1. **使用 BOOT0 方法进入系统 Bootloader**：
   - BOOT0 接 3.3V → 上电 → `connect` → `erase`
   
2. **或者用硬件复位配合快速连接**：
   - 在 `connect` 提示时按复位按钮
   - 上电后 100ms 内疯狂按回车

3. **擦除 Flash**：
   ```
   halt
   erase
   ```

4. **确认连接成功**：
   ```
   > mem 0x08000000 10
   应该显示：FFFFFFFF FFFFFFFF ... （擦除后全是 0xFF）
   ```

---

### 步骤 2: 配置 Keil Debug Settings

#### 2.1 打开 Debug 配置

在 Keil MDK 中：
```
Project → Options for Target → Debug 选项卡
```

#### 2.2 选择调试器

- **Use**: 选择 `J-LINK / J-TRACE Cortex`（不是 ST-Link！）
- 点击右侧 `Settings` 按钮

#### 2.3 J-Link Debug 配置

**Port 选项卡**：
```
Port: SW (不是 JTAG！)
Max Clock: 5 MHz (或 4000 kHz，稳定后可以提高到 10 MHz)
□ Auto Clk (不要勾选，先用固定时钟)
```

**SW Device 显示**：
- **如果正常**：会显示 `ARM CoreSight SW-DP` 和 `IDCODE: 0x2BA01477`
- **如果显示 "No Device"**：说明物理连接有问题或芯片状态异常

**Flash Download 选项卡**：
```
☑ Download Function
☑ Verify
☑ Reset and Run

Programming Algorithm:
  Name: STM32F4xx 512kB Flash
  Start: 0x08000000
  Size: 0x00080000
  
☑ 确保勾选了该算法（如果没有，点击 Add 添加）
```

**Reset 配置**（在 Flash Download 下方或单独选项卡）：
```
Reset: SYSRESETREQ (推荐)
或者: Hardware (使用 NRST 引脚)

☑ Reset after download (下载后复位)
```

---

### 步骤 3: 修复常见配置错误

#### 3.1 检查 RAM 和 ROM 设置

```
Target 选项卡:

IROM1:
  Start: 0x08000000
  Size: 0x80000  (512KB)
  ☑ Startup (应用程序在 0x08003000 时不勾选，后面会处理)

IRAM1:
  Start: 0x20000000
  Size: 0x20000  (128KB)
```

**重要**：如果应用程序运行在非 0x08000000 地址（例如 0x08003000），需要特殊处理：
- 在 `Target → C/C++ → Preprocessor Symbols` 中定义：
  ```
  VECT_TAB_OFFSET=0x3000
  ```
- 或在 `system_stm32f4xx.c` 中设置：
  ```c
  #define VECT_TAB_SRAM
  #define VECT_TAB_OFFSET  0x3000
  ```

#### 3.2 检查 Linker Script

确认 `.sct` 文件中的地址与 Debug 设置一致：
```
LR_IROM1 0x08003000 0x0003A000  {
  ER_IROM1 0x08003000 0x0003A000  {
    ...
  }
}
```

#### 3.3 关闭所有断点

有时候旧的断点会导致连接问题：
```
Debug → Breakpoints → 全选 → Delete All
```

---

### 步骤 4: 特殊情况处理

#### 情况 A: "No Algorithm found for: 0x08010000"

**原因**：Flash 下载算法只覆盖 0x08000000-0x08080000，但代码在其他地址

**解决**：
1. 检查 Linker Script 地址是否正确
2. 确保 Flash Download 算法的 Start 地址和 Size 覆盖你的代码区域

#### 情况 B: 下载成功但无法运行

**检查**：
```c
// 在 main.c 开头确认向量表地址
SCB->VTOR = 0x08003000;  // 必须与 Linker Script 一致
```

#### 情况 C: 连接时卡住或超时

**在 J-Link Settings 中**：
```
Trace 选项卡:
□ 不要启用 Trace（会降低连接速度）

Autodetect ROM:
□ 取消勾选（手动配置更可靠）
```

---

### 步骤 5: 测试连接

#### 5.1 编译项目
```
Project → Rebuild All
检查输出地址：
  FromELF: creating hex file...
  Program Size: Code=xxx RO-data=xxx ...
```

#### 5.2 下载测试
```
Flash → Download
```

**成功标志**：
```
Load "D:\...\17_app.axf"
Erase Done.
Programming Done.
Verify OK.
Application running...
```

#### 5.3 启动调试
```
Debug → Start/Stop Debug Session (Ctrl+F5)
```

**成功标志**：
- 停在 `main()` 函数入口
- 可以设置断点
- 可以单步执行

---

## 🎯 完整检查清单

### 硬件连接
- [ ] SWDIO (PA13) → J-Link Pin 7
- [ ] SWCLK (PA14) → J-Link Pin 9  
- [ ] GND → J-Link Pin 4
- [ ] 3.3V 供电稳定

### J-Link Commander 测试
- [ ] 能识别 J-Link 硬件（SN: 69409324）
- [ ] 能识别目标芯片（IDCODE: 0x2BA01477）
- [ ] 能读取内存（`mem 0x08000000 10`）
- [ ] 能擦除 Flash（`erase`）

### Keil 配置
- [ ] Debug → Use: J-LINK / J-TRACE Cortex
- [ ] Port: SW
- [ ] SW Device: ARM CoreSight SW-DP 显示正常
- [ ] Flash Download Algorithm: STM32F4xx 512kB 已勾选
- [ ] Target → IROM1 Start: 0x08000000 或正确的偏移
- [ ] Linker Script (.sct) 地址正确

### 代码配置
- [ ] `SCB->VTOR` 地址与 Linker Script 一致
- [ ] 删除了 `HAL_DeInit()` 调用
- [ ] 没有禁用 SWD 引脚（PA13/PA14）
- [ ] 没有无条件进入低功耗模式

---

## 🔧 终极解决方案（如果上述都失败）

### 方法 1: 使用 J-Link 直接烧录

不通过 Keil，直接用 J-Link Commander：

```bash
JLink.exe

# 连接
connect
Device: STM32F411CE
TIF: SWD
Speed: 4000

# 擦除
erase

# 烧录 (假设文件在当前目录)
loadfile "D:\ecnu\work\work_space\lixin_embedded\17_app\MDK-ARM\17_app\17_app.bin" 0x08003000

# 验证
verifybin "D:\ecnu\work\work_space\lixin_embedded\17_app\MDK-ARM\17_app\17_app.bin" 0x08003000

# 复位运行
reset
go
```

### 方法 2: 使用 ST-Link Utility

如果有 ST-Link V2：
1. 安装 ST-Link Utility
2. Connect to Target
3. Program → 选择 .hex 或 .bin 文件
4. Start Address: 0x08003000
5. Start Programming

### 方法 3: 更新 J-Link 固件

虽然你的 V9 固件是 2021 年的，但可以尝试更新：
1. 下载 J-Link Software Pack（最新版）
2. 安装后会自动检测并提示更新固件
3. 更新后重新配置 Keil

---

## 📞 排查步骤总结

```
1. J-Link Commander 能连接吗？
   ├─ 能 → 问题在 Keil 配置
   └─ 不能 → 问题在硬件或芯片状态
       └─ 用 BOOT0 进入系统 Bootloader → 擦除 Flash

2. Keil 中 SW Device 显示什么？
   ├─ "No Device" → 检查 Port 配置（必须是 SW）
   ├─ "ARM CoreSight SW-DP" → 正常，检查 Flash 算法
   └─ 超时/卡住 → 降低时钟速度，关闭 Trace

3. 下载时报错什么？
   ├─ "No Algorithm" → 检查 Flash Download 配置
   ├─ "Can't access memory" → 检查地址是否正确
   └─ "Verify Failed" → 检查代码是否损坏

4. 下载成功但不运行？
   └─ 检查 SCB->VTOR 和 Linker Script 地址
```

---

**关键配置参数（适用于你的项目）**：
```
芯片: STM32F411CE
接口: SWD
速度: 4000 kHz
应用地址: 0x08003000
Flash 算法: STM32F4xx 512kB
J-Link SN: 69409324
```

