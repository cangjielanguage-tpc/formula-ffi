// scripts/pre-install.mjs
import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');

// stdx configuration for download
const ARM64_ZIP_URL = 'https://gitcode.com/Cangjie/cangjie_stdx/releases/download/v1.1.3.1/cangjie-stdx-ohos-aarch64-1.1.3.1.zip';
const ARM64_ZIP_FILENAME = 'cangjie-stdx-ohos-aarch64-1.1.3.1.zip';
const ARM64_ZIP_TEMP_PATH = path.join(PROJECT_ROOT, ARM64_ZIP_FILENAME);

const X64_ZIP_URL = 'https://gitcode.com/Cangjie/cangjie_stdx/releases/download/v1.1.3.1/cangjie-stdx-ohos-x64-1.1.3.1.zip';
const X64_ZIP_FILENAME = 'cangjie-stdx-linux-x64-1.1.3.1.zip';
const X64_ZIP_TEMP_PATH = path.join(PROJECT_ROOT, X64_ZIP_FILENAME);

const EXTRACT_PATH = path.join(PROJECT_ROOT, 'stdx_bin');

// Download and extract stdx bin file
if (!fs.existsSync(EXTRACT_PATH)) {
  console.log(`Downloading required files...`);
  try {
      // Create directory if it doesn't exist
      fs.mkdirSync(EXTRACT_PATH, { recursive: true });

      // Download ARM64 version
      console.log(`Downloading ARM64 version...`);
      execSync(`curl -L -o ${ARM64_ZIP_TEMP_PATH} ${ARM64_ZIP_URL}`, {
          stdio: 'inherit',
          cwd: PROJECT_ROOT
      });

      // Check if file exists and has content
      if (fs.existsSync(ARM64_ZIP_TEMP_PATH)) {
          const stats = fs.statSync(ARM64_ZIP_TEMP_PATH);
          if (stats.size === 0) {
              console.error(`ARM64 ZIP file is empty, download failed`);
              fs.unlinkSync(ARM64_ZIP_TEMP_PATH);
              process.exit(1);
          }
      } else {
          console.error(`ARM64 ZIP file not found`);
          process.exit(1);
      }

      // Extract ARM64 version
      console.log(`Extracting ARM64 version...`);
      execSync(`tar -xf ${ARM64_ZIP_TEMP_PATH} -C ${EXTRACT_PATH}`, {
          stdio: 'inherit',
          cwd: PROJECT_ROOT
      });

      // Clean up ARM64 temp file
      fs.unlinkSync(ARM64_ZIP_TEMP_PATH);

      // Download X64 version if on Windows
      if (process.platform === 'win32') {
          console.log(`Downloading X64 version...`);
          execSync(`curl -L -o ${X64_ZIP_TEMP_PATH} ${X64_ZIP_URL}`, {
              stdio: 'inherit',
              cwd: PROJECT_ROOT
          });

          // Check if file exists and has content
          if (fs.existsSync(X64_ZIP_TEMP_PATH)) {
              const stats = fs.statSync(X64_ZIP_TEMP_PATH);
              if (stats.size === 0) {
                  console.error(`X64 ZIP file is empty, download failed`);
                  fs.unlinkSync(X64_ZIP_TEMP_PATH);
                  process.exit(1);
              }
          } else {
              console.error(`X64 ZIP file not found`);
              process.exit(1);
          }

          // Extract X64 version
          console.log(`Extracting X64 version...`);
          execSync(`tar -xf ${X64_ZIP_TEMP_PATH} -C ${EXTRACT_PATH}`, {
              stdio: 'inherit',
              cwd: PROJECT_ROOT
          });

          // Clean up X64 temp file
          fs.unlinkSync(X64_ZIP_TEMP_PATH);
      }

      console.log(`All files downloaded and extracted successfully`);

  } catch (error) {
      console.error(`Download or extraction failed: ${error.message}`);
      // Clean up if partially downloaded
      if (fs.existsSync(ARM64_ZIP_TEMP_PATH)) {
          fs.unlinkSync(ARM64_ZIP_TEMP_PATH);
      }
      if (fs.existsSync(X64_ZIP_TEMP_PATH)) {
          fs.unlinkSync(X64_ZIP_TEMP_PATH);
      }
      process.exit(1);
  }
} else {
  console.log(`Directory ${EXTRACT_PATH} already exists, skipping.`);
}
