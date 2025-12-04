# mini_ids_0.2

Project Overview

This project implements a portable Wi-Fi security assessment device using the ESP32-S3 in promiscuous mode. The system passively monitors wireless traffic without joining any network, extracts low-level information from 802.11 management frames, and evaluates the security posture of nearby access points. The goal is to provide users with a pre-connection risk assessment for any public or private Wi-Fi network.

The device is designed to function as a lightweight, low-power Wi-Fi IDS and later evolve into a complete consumer security tool through machine learning, mobile integration, and an embedded coprocessor.

Purpose

The purpose of this project is to build a small, portable security device that can:

Analyze Wi-Fi traffic before the user connects to the network

Detect security risks such as:

Deauthentication attacks

Evil Twin access points

Weak or outdated encryption

Abnormal beacon activity

Suspicious channel or BSSID behavior

Help users make informed decisions about whether a network is safe to join

Provide a hands-on learning platform for wireless security and embedded systems

The long-term objective is to turn this into a user-friendly product rather than just a technical demo.

System Architecture
ESP32-S3 (Wireless Analysis Core)

The ESP32-S3 handles:

Promiscuous mode packet capture

Beacon parsing (SSID, BSSID, channel, security information)

Deauthentication monitoring

Channel scanning (1–13)

Per-access point statistics

Basic anomaly detection rules

Data transmission to the mobile application (via Bluetooth LE)

STM32 (Security and Processing Coprocessor)

In the final design, an STM32 microcontroller is planned to:

Perform additional security checks

Run supplemental analysis algorithms

Serve as a reliability and safety layer

Optionally handle firmware updates, sensor aggregation, or power management

The STM32 acts as a dedicated embedded security engine to complement the ESP32’s wireless capabilities.

Machine Learning Layer

Two alternative approaches are planned:

TinyML running on the ESP32-S3 or STM32

Classifies network behavior as normal or anomalous

Extracts lightweight features such as packet rates, beacon intervals, and security tags

Cloud-based analysis

ESP32 sends summarized data to the mobile app

The app forwards it to a backend for deeper ML-based evaluation

Results are returned to the user as a risk score

Mobile Application (Bluetooth Integration)

The device communicates with a mobile app through Bluetooth Low Energy (BLE).
The app is intended to:

Pair with the device

Receive analyzed Wi-Fi environment data

Display network risk assessments

Show warnings about unsafe networks

Help the user decide whether to connect

Current Features (Implemented)

Full ESP32-S3 promiscuous Wi-Fi sniffer

Initial channel scan (1–13) to discover all nearby access points

SSID/BSSID extraction from beacon frames

Channel identification

Deauthentication frame detection

Per-network statistics table (beacon count, deauth count, channel)

Real-time console output in CSV and formatted table formats

Modular architecture for future expansion

Planned Features (Future Work)

TinyML-based anomaly detection

Risk scoring model for public Wi-Fi networks

Bluetooth mobile app for user interaction

BLE-based network selection and monitoring

Integration of STM32 as a secondary processing unit

Secure over-the-air updates

Detection of advanced attacks such as:

Beacon flood

Probe response spoofing

Evil Twin access points

Rogue AP identification

Exportable logs and JSON-based API

Educational Value

This project provides a practical learning platform for:

802.11 frame analysis

Embedded security

ESP-IDF and low-level Wi-Fi APIs

Network forensics

Real-time data processing

Microcontroller integration

Machine learning on edge devices

Status

The current version is a functional prototype focusing on:

Accurate Wi-Fi environment mapping

Real-time IDS-style monitoring

Building the foundation for a portable security product

Further development will shift toward ML-based analysis, mobile integration, and embedding the STM32 security coprocessor.
