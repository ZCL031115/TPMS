# TPMS — 直接式汽车胎压监测系统

基于 STM32F103C8T6 的胎压监测系统仿真原型，Proteus 8.17 + CubeMX + Keil MDK。

## 文件结构

```
TPMS/
├── Master_Proteus/              # 主机工程（仪表台显示报警单元）
│   ├── Master.ioc               # CubeMX 配置文件
│   └── Core/
│       ├── Src/
│       │   ├── main.c           # 主程序（SPI主机、按键、报警、OLED显示）
│       │   ├── ssd1306.c        # OLED 软件I2C驱动
│       │   └── tim.c            # TIM2 PWM 配置
│       └── Inc/
│           ├── ssd1306.h        # OLED 驱动头文件
│           └── OLED_Font.h      # 8×16 点阵字库
│
├── Slave_Proteus/               # 从机工程（轮胎传感器采集节点）
│   ├── Slave.ioc                # CubeMX 配置文件
│   └── Core/
│       └── Src/
│           ├── main.c           # 主程序（ADC采集、SPI从机）
│           └── adc.c            # ADC1 单次转换配置
│
├── poteusProject/               # Proteus 仿真工程
│   └── TPMS.pdsprj
│
├── 毕业设计文档.md               # 毕业论文
└── README.md
```

## 快速开始

### 1. 编译固件

用 Keil MDK 分别打开 `Master_Proteus/Master.ioc` 和 `Slave_Proteus/Slave.ioc`，CubeMX → Generate Code，然后编译。

- Master hex → `Master_Proteus/MDK-ARM/Master/Master.hex`
- Slave hex → `Slave_Proteus/MDK-ARM/Slave/Slave.hex`

### 2. 运行仿真

1. 打开 `poteusProject/TPMS.pdsprj`
2. 双击 Master STM32 → Program File 选 `Master.hex`，Clock Frequency = **8MHz**
3. 双击 Slave STM32 → Program File 选 `Slave.hex`，Clock Frequency = **8MHz**
4. 两个终端均设为 **9600 bps**
5. 运行 ▶

### 3. 交互说明

| 操作 | 效果 |
|------|------|
| 拧气压电位器 (PA1, Slave) | 气压 0~600 kPa 变化 |
| 拧温度电位器 (PA2, Slave) | 温度 0~60°C 变化 |
| 按 KEY_MENU (PA0, Master) | OLED 概览/详情切换 |
| 按 KEY_ALARM (PA2, Master) | 强制报警测试 |

### 4. 报警阈值

| 条件 | OLED 显示 | LED/蜂鸣器 |
|------|----------|-----------|
| 气压 < 150 kPa | PRES DANGER! | 红灯 + 蜂鸣器 |
| 气压 < 180 kPa | PRES ALARM | 红灯 + 蜂鸣器 |
| 气压 > 350 kPa | PRES ALARM | 红灯 + 蜂鸣器 |
| 温度 < 0°C | TEMP ALARM | 红灯 + 蜂鸣器 |
| 温度 > 45°C | TEMP ALARM | 红灯 + 蜂鸣器 |

## 技术要点

- **通信**：软件 GPIO 模拟 SPI（500Hz SCK，6 字节数据帧 + 累加和校验）
- **显示**：软件 GPIO 模拟 I2C 驱动 SSD1306 OLED（8×16 ASCII 字体，4 行显示）
- **ADC**：单次转换模式，双通道（CH1 气压、CH2 温度）分时复用
- **时钟**：HSI 8MHz 内部振荡器，全速运行无需外部晶振
- **蜂鸣器**：TIM2 PWM 278Hz 驱动，正常态 PA1 切为模拟输入消除串扰噪声
