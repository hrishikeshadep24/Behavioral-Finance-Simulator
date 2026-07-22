# 📈 Behavioral Investment Simulator

<p align="center">

![C++](https://img.shields.io/badge/C++-17-blue)
![Finance](https://img.shields.io/badge/Domain-Behavioral%20Finance-success)
![Simulation](https://img.shields.io/badge/Project-Stock%20Trading-orange)
![CSV](https://img.shields.io/badge/Data-CSV-red)
![Status](https://img.shields.io/badge/Status-Completed-success)
![License](https://img.shields.io/badge/License-MIT-blue)

</p>

---

# 📖 Overview

Behavioral Investment Simulator is a C++-based stock market simulation platform that combines realistic trading mechanics with behavioral finance analysis. Unlike conventional trading simulators that primarily evaluate financial performance, this system continuously monitors investor actions to identify cognitive biases that influence decision-making.

The simulator provides a safe environment for learning investment strategies while generating detailed behavioral reports that help users understand how emotions and psychology affect trading performance.

---

# ✨ Key Features

- 📈 Historical Stock Market Simulation
- 💹 Portfolio Management
- 📰 News-Driven Trading Decisions
- 🧠 Behavioral Bias Detection
- 📊 Investor Psychology Analysis
- 📋 Automated Behavioral Reports
- 💾 Persistent Portfolio State
- 📁 CSV-Based Market Data
- 📉 Multi-Segment Trading Sessions
- 🎓 Educational Trading Environment

---

# 🎯 Behavioral Finance Module

The simulator evaluates investor behavior using multiple behavioral finance indicators.

### Supported Bias Detection

- Overconfidence Bias
- Loss Aversion
- Confirmation Bias
- Herding Behaviour
- Excessive Trading
- Risk-Taking Behaviour
- Portfolio Concentration
- Decision Consistency

The generated behavioral report provides personalized insights that encourage disciplined investment practices.

---

# 📊 Trading Workflow

The simulator follows a realistic trading workflow:

```text
Load Historical Market Data
        │
        ▼
Display Market Session
        │
        ▼
Read Market News
        │
        ▼
User Executes Trades
        │
        ▼
Portfolio Update
        │
        ▼
Behavior Logging
        │
        ▼
Bias Detection
        │
        ▼
Behavior Report Generation
```

---

# 💹 Market Simulation

The platform simulates multiple trading sessions using historical Indian stock market data.

### Trading Segments

- Morning Session
- Day Session
- Closing Session

Each trading decision influences both portfolio performance and behavioral metrics.

---

# 🧠 Behavioral Analysis Engine

The simulator continuously records:

- Buy Decisions
- Sell Decisions
- Portfolio Allocation
- Research Behaviour
- Trade Frequency
- Holding Period
- Profit/Loss Patterns
- News Reading Activity

These observations are used to generate an investor psychology profile.

---

# 📂 Project Structure

```text
Behavioral-Investment-Simulator/
│
├── main.cpp
├── deep.cpp
│
├── global_news.csv
├── stock_history.csv
├── stocks_master.csv
├── stock_daily_news.csv
├── portfolio_state.csv
├── portfolio_snapshots.csv
├── segment_news.csv
├── trades.csv
├── views.csv
│
├── behavior_report.txt
├── README.md
└── LICENSE
```

---

# 🛠 Tech Stack

### Programming Language

- C++

### Standard

- C++11
- C++17 Compatible

### Storage

- CSV Files

### Core Components

- Market Engine
- Portfolio Engine
- Behavioral Analyzer
- Logger
- Report Generator

---

# 🚀 How to Run

Compile

```bash
g++ main.cpp -o simulator
```

Run

```bash
./simulator --dataset dataset_indian_15days --run-name mysession
```

Generate Behavioral Report

```bash
./simulator --analyze
```

---

# 📈 Applications

- Behavioral Finance Education
- Investor Training
- Academic Research
- Financial Psychology Studies
- Stock Market Simulation
- Decision-Making Analysis

---

# 📈 Future Enhancements

- Machine Learning-Based Bias Prediction
- Real-Time Stock Market APIs
- Interactive Dashboard
- Multi-User Trading Sessions
-
