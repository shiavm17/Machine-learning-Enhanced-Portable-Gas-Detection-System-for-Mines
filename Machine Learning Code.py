import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score
from joblib import dump, load
import serial

# -------------------------------------------
# 1. Dataset Preparation
# -------------------------------------------
# Load historical sensor data
# Columns: ['Temperature', 'Humidity', 'Pressure', 'Light', 'MQ135', 'MQ7', 'MQ2', 'TGS2600', 'TGS822', 'Label']
# Labels: 0 = Safe, 1 = Moderate, 2 = Hazardous
data = pd.read_csv("gas_sensor_data.csv")

# Features and Labels
X = data[['Temperature', 'Humidity', 'Pressure', 'Light', 'MQ135', 'MQ7', 'MQ2', 'TGS2600', 'TGS822']]
y = data['Label']

# Train-Test Split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# -------------------------------------------
# 2. Model Training
# -------------------------------------------
# Random Forest Classifier
clf = RandomForestClassifier(n_estimators=100, random_state=42)
clf.fit(X_train, y_train)

# Evaluate Model
y_pred = clf.predict(X_test)
print("Model Accuracy: {:.2f}%".format(accuracy_score(y_test, y_pred) * 100))

# Save the Model
dump(clf, "gas_detection_model.joblib")

# -------------------------------------------
# 3. Real-time Inference
# -------------------------------------------
# Load the trained model
model = load("gas_detection_model.joblib")

# Serial Communication Setup (ESP32)
ser = serial.Serial('COM3', 115200, timeout=1)  # Update with your COM port
print("Connected to ESP32")

def classify_gas_levels(data):
    """Classify gas levels based on the trained model."""
    prediction = model.predict([data])[0]
    return prediction  # 0 = Safe, 1 = Moderate, 2 = Hazardous

# Gas Level Mapping
level_mapping = {0: "SAFE", 1: "MODERATE", 2: "HAZARDOUS"}

while True:
    # Read sensor data from ESP32
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        try:
            # Parse incoming data: Temp, Humidity, Pressure, Light, MQ135, MQ7, MQ2, TGS2600, TGS822
            sensor_data = list(map(float, line.split(',')))

            # Perform prediction
            gas_level = classify_gas_levels(sensor_data)

            # Display results
            print(f"Sensor Data: {sensor_data}")
            print(f"Gas Level: {level_mapping[gas_level]}")

            # Send response to ESP32
            ser.write(f"{level_mapping[gas_level]}\n".encode())

        except Exception as e:
            print(f"Error processing data: {e}")
