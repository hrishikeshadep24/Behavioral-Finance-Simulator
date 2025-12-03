Project Title: Behavioral Investment Simulator



Project Overview


A sophisticated C++ stock trading simulator that combines financial market mechanics with behavioral psychology analysis. Unlike traditional simulators that focus only on profits, our platform tracks and analyzes cognitive biases in real-time, providing insights into how emotions influence financial decisions.

Key Features
Realistic Market Simulation: Multi-segment trading (Morning/Day/Close) with historical Indian stock data

Behavioral Tracking: Comprehensive logging of trades, research patterns, and portfolio changes

Bias Detection: Identifies overconfidence, loss aversion, herding behavior, and confirmation bias

Automated Reporting: Generates detailed behavioral analysis reports

Persistent State: Save/resume functionality for longitudinal studies

Educational Feedback: Provides actionable insights for improving trading discipline

Technical Highlights
Language: C++ (C++11/C++17 compatible)

Data Structures: Custom classes for Market, Portfolio, Position, Logger

Analysis Engine: BehavioralAnalyzer class with 8+ behavioral metrics

Persistence: CSV-based logging with timestamped records

Color-Coded UI: ANSI color terminals for intuitive visual feedback

Educational Value
Demonstrates real-world trading psychology concepts

Provides safe environment for learning investment strategies

Enables research on cognitive biases in financial decision-making

Serves as educational tool for behavioral finance courses

Applications
Academic Research: Study trading psychology patterns

Investor Education: Practice trading without financial risk

Behavioral Studies: Analyze decision-making processes

Finance Curriculum: Supplementary tool for economics/finance courses

Team Contribution
Market & Portfolio Engine: Abhishek Jadhav

Behavioral Analysis & Report Generation: Prem Ashtekar

UI/UX & Integration: Abhay Gunjal 

Testing & Documentation: Hrishikesh Adep

Files Included
main.cpp - Complete simulator + analyzer (1030 lines)

dataset/ - Historical stock data and news files

runs/ - User session logs and reports

behavior_report.txt - Sample behavioral analysis output

How to Use
Run Simulator: ./sim.exe --dataset dataset_indian_15days --run-name mysession

Generate Report: Add --analyze flag after trading

Review Results: Check behavior_report.txt for detailed analysis

Key Learnings
Integration of financial algorithms with psychological analysis

Real-time behavioral pattern recognition

Persistent data architecture for longitudinal studies

Educational interface design for complex concepts

Future Enhancements
Web-based GUI interface

Machine learning for predictive behavioral modeling

Multi-user competitive trading

Real-time market data integration
