/***********************************************************
 *  PreSetupCode.ino
 *  --------------------------------
 *  Demonstration of how to:
 *   1) Initialize SPIFFS.
 *   2) Enumerate JPEG images in a folder.
 *   3) Decode each image to a raw buffer (placeholder here).
 *   4) Extract features or store raw data.
 *   5) Append the resulting numeric data to a CSV file.
 ***********************************************************/

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

// If you want to use ESP32 camera functions or camera_pins.h, you can include them:
// #include "esp_camera.h"
// #include "camera_pins.h"

/***********************************************************
 * Initialize the file system (SPIFFS)
 ***********************************************************/
bool initFileSystem() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error occurred while mounting SPIFFS");
    return false;
  }
  Serial.println("SPIFFS mounted successfully");
  return true;
}

/***********************************************************
 * PSEUDO: Decode JPEG from File
 * NOTE: You need an actual decoder (e.g. TJpgDec, ESP32's
 *       built-in JPEG decoder, or any library) to fill
 *       the 'rawData' buffer with uncompressed pixel data.
 * 
 * For demonstration, we simply fill 'rawData' with dummy
 * values to mimic an actual decode.
 ***********************************************************/
bool decodeJpeg(File &file, uint8_t *rawData, size_t rawSize) {
  // In real usage, read and decode file contents into rawData
  // e.g. using JPEGDEC or built-in ESP32 decode functions
  // This is just a placeholder:
  memset(rawData, 127, rawSize); // fill with "gray" dummy data
  return true; 
}

/***********************************************************
 * PSEUDO: Extract Features from Raw Image
 * For advanced image processing or a neural net, you might:
 *   - Convert to grayscale
 *   - Downscale or crop
 *   - Extract edges, color histograms, etc.
 *   - Possibly run a small CNN or other ML model
 * 
 * Here we simply compute a trivial "average color" as a demo.
 ***********************************************************/
void extractFeatures(const uint8_t *rawData, size_t width, size_t height, float *features, size_t featureCount) {
  // Example: compute a single average pixel value
  // (assuming 3 bytes/pixel for RGB in rawData)
  size_t totalPixels = width * height;
  unsigned long sum = 0;
  
  for (size_t i = 0; i < totalPixels * 3; i++) {
    sum += rawData[i];
  }
  
  float avgVal = (float)sum / (float)(totalPixels * 3);

  // Fill the features array. 
  // If featureCount == 1, we store just the average.
  // You could store more complex features if desired.
  features[0] = avgVal;

  // If you have more features, set them here, e.g.:
  // features[1] = someHistogramValue;
  // features[2] = edgeCount;
  // etc.
}

/***********************************************************
 * Save extracted features to a CSV file in SPIFFS
 ***********************************************************/
void saveDataset(const char *path, float *features, size_t featureCount) {
  File datasetFile = SPIFFS.open(path, FILE_APPEND);
  if (!datasetFile) {
    Serial.println("Failed to open dataset file for appending");
    return;
  }

  // Write features in CSV format
  for (size_t i = 0; i < featureCount; i++) {
    datasetFile.print(features[i], 6); // 6 decimal places
    if (i < featureCount - 1) {
      datasetFile.print(",");
    }
  }
  datasetFile.println();
  datasetFile.close();
}

/***********************************************************
 * Convert all .jpg images to dataset
 * Iterates over files in SPIFFS, checks if .jpg, decodes,
 * extracts features, and appends them to /dataset.csv
 ***********************************************************/
void convertAllImagesToDataset() {
  // For subfolder usage, e.g. "/images", you might open that directory:
  File root = SPIFFS.open("/images");
  if (!root || !root.isDirectory()) {
    Serial.println("Could not open /images folder");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();

    // Check if it's a JPEG
    if (!file.isDirectory() && fileName.endsWith(".jpg")) {
      Serial.print("Processing: ");
      Serial.println(fileName);

      // Read file size
      size_t fileSize = file.size();
      if (fileSize == 0) {
        Serial.println("File is empty, skipping.");
        file = root.openNextFile();
        continue;
      }

      // Allocate buffer for the file data
      uint8_t *inputBuffer = (uint8_t *)malloc(fileSize);
      if (!inputBuffer) {
        Serial.println("Failed to allocate inputBuffer");
        file = root.openNextFile();
        continue;
      }

      // Read the entire file into memory
      file.read(inputBuffer, fileSize);

      // For demonstration, let's assume we want a 320x240 raw buffer
      // (3 bytes per pixel = 320 * 240 * 3 = 230,400 bytes).
      // For real usage, adjust to match your camera or decode result.
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

      // Decode the JPEG to rawData
      bool decodeOk = decodeJpeg(file, rawData, rawSize);
      if (!decodeOk) {
        Serial.println("JPEG decode failed");
        free(rawData);
        free(inputBuffer);
        file = root.openNextFile();
        continue;
      }

      // Extract features (example: 1 feature = average pixel)
      const size_t featureCount = 1;
      float features[featureCount];
      extractFeatures(rawData, width, height, features, featureCount);

      // Save to dataset (append CSV)
      saveDataset("/dataset.csv", features, featureCount);

      // Cleanup
      free(rawData);
      free(inputBuffer);
    }

    // Move to the next file
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

  // This will create (or append to) /dataset.csv with
  // your extracted features from each .jpg
  convertAllImagesToDataset();

  Serial.println("Done converting images to dataset.");
}

void loop() {
  // Nothing to do here.
  // In real usage, you might want to do this only once,
  // or upon a certain trigger, or while receiving new images.
}
