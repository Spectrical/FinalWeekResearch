#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

bool initFileSystem() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error occurred while mounting SPIFFS");
    return false;
  }
  Serial.println("SPIFFS mounted successfully");
  return true;
}


bool decodeJpeg(File &file, uint8_t *rawData, size_t rawSize) {
  memset(rawData, 127, rawSize);
  return true; 
}

void extractFeatures(const uint8_t *rawData, size_t width, size_t height, float *features, size_t featureCount) {
  size_t totalPixels = width * height;
  unsigned long sum = 0;
  
  for (size_t i = 0; i < totalPixels * 3; i++) {
    sum += rawData[i];
  }
  
  float avgVal = (float)sum / (float)(totalPixels * 3);
  features[0] = avgVal;
}

void saveDataset(const char *path, float *features, size_t featureCount) {
  File datasetFile = SPIFFS.open(path, FILE_APPEND);
  if (!datasetFile) {
    Serial.println("Failed to open dataset file for appending");
    return;
  }

  for (size_t i = 0; i < featureCount; i++) {
    datasetFile.print(features[i], 6);
    if (i < featureCount - 1) {
      datasetFile.print(",");
    }
  }
  datasetFile.println();
  datasetFile.close();
}

void convertAllImagesToDataset() {
  File root = SPIFFS.open("/images");
  if (!root || !root.isDirectory()) {
    Serial.println("Could not open /images folder");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();

    if (!file.isDirectory() && fileName.endsWith(".jpg")) {
      Serial.print("Processing: ");
      Serial.println(fileName);

      size_t fileSize = file.size();
      if (fileSize == 0) {
        Serial.println("File is empty, skipping.");
        file = root.openNextFile();
        continue;
      }
      uint8_t *inputBuffer = (uint8_t *)malloc(fileSize);
      if (!inputBuffer) {
        Serial.println("Failed to allocate inputBuffer");
        file = root.openNextFile();
        continue;
      }
      file.read(inputBuffer, fileSize);
      size_t width = 320;
      size_t height = 240;
      size_t rawSize = width * height * 3; 
      uint8_t *rawData = (uint8_t *)malloc(rawSize);

      if (!rawData) {
        Serial.println("Failed to allocate rawData buffer");
        free(inputBuffer);
        file = root.openNextFile();
        continue;
      }
      bool decodeOk = decodeJpeg(file, rawData, rawSize);
      if (!decodeOk) {
        Serial.println("JPEG decode failed");
        free(rawData);
        free(inputBuffer);
        file = root.openNextFile();
        continue;
      }
      const size_t featureCount = 1;
      float features[featureCount];
      extractFeatures(rawData, width, height, features, featureCount);
      saveDataset("/dataset.csv", features, featureCount);
      free(rawData);
      free(inputBuffer);
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!initFileSystem()) {
    Serial.println("SPIFFS init failed!");
    return;
  }
  convertAllImagesToDataset();

  Serial.println("Done converting images to dataset.");
}

void loop() {

}
