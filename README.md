<div align="center">

# 🧠 NeuroPulse

### *Neural Data, Engineered Precision*

**Multimodal Neuro-Health Monitoring — EEG · ECG · SpO₂ · Environment**

[![Platform](https://img.shields.io/badge/Platform-ESP32-blue?style=flat-square&logo=espressif)](https://www.espressif.com/)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-orange?style=flat-square&logo=c)](https://isocpp.org/)
[![IDE](https://img.shields.io/badge/IDE-Arduino-teal?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![Dashboard](https://img.shields.io/badge/Dashboard-Node--RED-red?style=flat-square)](https://nodered.org/)
[![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)](LICENSE)

</div>

---

## 📸 Prototype Gallery

<div align="center">

| Front Panel | ECG Electrodes |
|:-----------:|:--------------:|
| ![Front Panel](assets/prototype_front.jpeg) | ![ECG Electrodes](assets/prototype_electrodes.jpeg) |
| *TFT display, rotary encoder & LED indicators* | *3-lead snap ECG electrodes with 3D-printed housing* |

| Internal — ESP32 Layer | Analog Signal Chain | Full Internal View |
|:----------------------:|:-------------------:|:-----------------:|
| ![ESP32 Layer](assets/prototype_esp32.jpeg) | ![Analog Board](assets/prototype_analog.jpeg) | ![Internal](assets/prototype_internal.jpeg) |
| *ESP32 with heatsink & sensor modules* | *Op-amp signal conditioning board* | *Complete wiring harness* |

</div>

---

## 🔍 Overview

**NeuroPulse** is an open-source, low-cost multimodal neuro-health monitoring system that simultaneously acquires **brain signals (EEG/ECG)**, **cardiovascular data (BPM & SpO₂)**, and **environmental metrics (AQI, temperature, humidity)** — correlating them in real time on a single platform.

By analyzing **alpha and beta brainwave patterns** through FFT-based signal processing, NeuroPulse enables detection of neural distortions, cognitive load anomalies, and abnormal physiological stress responses — all at a fraction of the cost of clinical systems.

---

## ❗ Problem Statement

| Challenge | Description |
|-----------|-------------|
| 🧩 Fragmented monitoring | No existing low-cost platform unifies brain, heart, and environment data |
| 💸 Cost barrier | Clinical EEG systems are prohibitively expensive for research & education |
| ⏱️ Late detection | Neural imbalances are rarely identified before symptoms escalate |
| 📊 No real-time correlation | Existing tools lack simultaneous brain–body–environment analysis |

---

## ✅ Solution

NeuroPulse bridges this gap by providing a **research-grade, open-hardware platform** that:

- Acquires and amplifies biopotential signals with a multi-stage analog front-end
- Performs on-device FFT to extract alpha/beta brainwave components
- Correlates neural patterns with heart rate, SpO₂, and environmental quality
- Streams processed data to a real-time web dashboard via ESP32 Wi-Fi

---

## ⚡ Key Features

- **Multimodal sensing** — EEG, ECG, BPM, SpO₂, AQI, temperature, humidity
- **Multi-stage analog front-end** — AD8232 → AD620 → TL072 → LM358 signal chain
- **High-resolution ADC** — 16-bit ADS1115 for precise biopotential digitization
- **Real-time FFT processing** — Alpha & beta brainwave extraction on-device
- **Brain–Body–Environment correlation** — Simultaneous multi-domain analysis
- **Wi-Fi data streaming** — Live Node-RED dashboard visualization
- **Low cost & open hardware** — Built on standard perfboard with off-the-shelf modules

---

## 🔗 Signal Flow Architecture

```
┌──────────┐    ┌───────┐    ┌────────┐    ┌────────┐    ┌──────────┐    ┌────────┐
│  AD8232  │───►│ AD620 │───►│ TL072  │───►│ LM358  │───►│ ADS1115  │───►│ ESP32  │
│ECG Module│    │Instr. │    │ Op-Amp │    │ Op-Amp │    │ 16-bit   │    │Process │
│          │    │  Amp  │    │Stage 1 │    │Stage 2 │    │   ADC    │    │& Stream│
└──────────┘    └───────┘    └────────┘    └────────┘    └──────────┘    └────────┘
```

---

## 🖥️ System Workflow

```
1. Sensor Acquisition   →  ECG electrodes, MAX30102, DHT11
2. Analog Conditioning  →  Amplification & noise filtering (AD8232 → LM358)
3. ADC Conversion       →  16-bit digitization via ADS1115 (I2C)
4. On-Device Processing →  FFT brainwave extraction on ESP32
5. Visualization        →  TFT display + Node-RED web dashboard
```

---

## 🔌 Hardware Architecture

### Sensor & ADC Interface

| Component | Pin   | Connected To  |
|-----------|-------|---------------|
| AD8232    | OUT   | AD620 (S+)    |
| AD8232    | GND   | AD620 (S-)    |
| AD620     | VOUT  | TL072 (Pin 3) |
| TL072     | Pin 1 | LM358 (Pin 3) |
| LM358     | Pin 1 | ADS1115 (A0)  |
| ADS1115   | SCL   | GPIO 22       |
| ADS1115   | SDA   | GPIO 21       |

### I2C Bus (Shared)

| Component | SDA     | SCL     |
|-----------|---------|---------|
| ADS1115   | GPIO 21 | GPIO 22 |
| MAX30102  | GPIO 21 | GPIO 22 |

### Peripheral GPIO Map

| Module         | Signal | GPIO    |
|----------------|--------|---------|
| DHT11          | DATA   | GPIO 13 |
| TFT Display    | SCK    | GPIO 18 |
| TFT Display    | MOSI   | GPIO 23 |
| TFT Display    | CS     | GPIO 5  |
| TFT Display    | RESET  | GPIO 4  |
| TFT Display    | DC/A0  | GPIO 2  |
| Rotary Encoder | CLK    | GPIO 33 |
| Rotary Encoder | DT     | GPIO 34 |
| Rotary Encoder | SW     | GPIO 35 |
| RGB LED        | Red    | GPIO 25 |
| RGB LED        | Green  | GPIO 26 |
| RGB LED        | Blue   | GPIO 27 |

### Power Rails

| Rail     | Components                                                   |
|----------|--------------------------------------------------------------|
| **3.3V** | AD8232, MAX30102, TFT Display, ADS1115, DHT11, Toggle Switch |
| **5V**   | AD620, TL072, LM358                                          |

> **Note:** Add a **10 kΩ pull-up resistor** between ESP32 EN and 3.3V for reliable reset behavior.

---

## 🧩 Bill of Materials

### Core Controller

| Component      | Description                              |
|----------------|------------------------------------------|
| ESP32 (30-pin) | Dual-core MCU with Wi-Fi & Bluetooth     |

### Analog Front-End

| Component | Role                                              |
|-----------|---------------------------------------------------|
| AD8232    | ECG biopotential acquisition module               |
| AD620     | Instrumentation amplifier (differential input)    |
| TL072     | Low-noise JFET op-amp — Stage 1 gain              |
| LM358     | General-purpose op-amp — Stage 2 gain             |

### Data Acquisition & Sensors

| Component | Role                                              |
|-----------|---------------------------------------------------|
| ADS1115   | 16-bit, 4-channel I2C ADC                         |
| MAX30102  | Pulse oximetry & heart rate (SpO₂ / BPM)          |
| DHT11     | Temperature & relative humidity                   |

### Output & Input

| Component      | Role                                          |
|----------------|-----------------------------------------------|
| TFT SPI Display| Real-time waveform & metrics display          |
| RGB LED        | System status indicator                       |
| Rotary Encoder | Menu navigation & parameter control           |
| Toggle Switch  | Power control (EN pin)                        |

### Passive Components

| Component  | Value   | Purpose                                           |
|------------|---------|---------------------------------------------------|
| Resistors  | 100 kΩ  | Op-amp gain (Pin 1 ↔ Pin 2 on TL072 & LM358)     |
| Resistor   | 10 kΩ   | EN pull-up                                        |
| Resistors  | 220 Ω   | RGB LED current limiting                          |
| Electrodes | 3-lead  | Snap-type Ag/AgCl disposable ECG pads             |

---

## 💻 Software Stack

| Layer              | Technology                                        |
|--------------------|---------------------------------------------------|
| Firmware           | Embedded C/C++ (Arduino framework)                |
| Signal Processing  | FFT for alpha/beta brainwave extraction           |
| Connectivity       | ESP32 native Wi-Fi (TCP/IP)                       |
| Dashboard          | Node-RED real-time flow-based visualization       |
| IDE                | Arduino IDE                                       |

---

## 🌍 Applications

- 🎓 **Academic Research** — Low-cost EEG/ECG platform for neuroscience labs
- 🧘 **Mental Wellness** — Non-clinical cognitive load & stress pattern awareness
- 🔬 **Human Behavior Analysis** — Correlate environment with physiological response
- 🛠️ **Neuro-Tech Prototyping** — Open hardware foundation for BCI experiments

---

## 🔭 Future Roadmap

- [ ] **AI/ML integration** — On-device anomaly detection for neural patterns
- [ ] **Mobile app** — iOS/Android companion with BLE streaming
- [ ] **EDA (Galvanic Skin Response)** — Stress quantification via skin conductance
- [ ] **Custom PCB** — Replace perfboard prototype with compact, shielded PCB
- [ ] **Cloud sync** — Long-term data logging and trend analysis

---

## ⚠️ Design Notes

- I2C bus is **shared** between ADS1115 and MAX30102 — ensure unique I2C addresses
- Keep analog signal wiring **short and shielded** to minimize EMI noise
- Maintain a **solid common ground** across all subsystems to prevent ground loops
- This system is for **research and educational purposes only** — not a medical device

---

## 📁 Repository Structure

```
NeuroPulse/
├── assets/
│   ├── prototype_front.jpeg
│   ├── prototype_electrodes.jpeg
│   ├── prototype_esp32.jpeg
│   ├── prototype_analog.jpeg
│   └── prototype_internal.jpeg
├── firmware/
│   └── neuropulse_main.ino
├── schematics/
│   └── hardware_architecture.md
├── dashboard/
│   └── node_red_flow.json
└── README.md
```

---

<div align="center">

*Designed for Precision · Built for Innovation · Open for Everyone*

**⭐ Star this repo if NeuroPulse inspired you**

</div>
