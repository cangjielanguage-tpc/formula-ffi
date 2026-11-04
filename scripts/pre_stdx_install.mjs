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

const EXTRACT_PATH = path.join(PROJECT_ROOT, 'formula', 'stdx_bin');


function extractZip(zipPath, destPath) {
    execSync(`unzip -o ${zipPath} -d ${destPath}`, {
        stdio: 'inherit',
        cwd: PROJECT_ROOT
    });
}

// Download and extract stdx bin file
// 下载/解压失败时会残留空的 stdx_bin 目录，仅判断存在会永久跳过下载，
// 因此目录存在但为空时也重新下载
const stdxBinReady = fs.existsSync(EXTRACT_PATH) && fs.readdirSync(EXTRACT_PATH).length > 0;
if (!stdxBinReady) {
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
      extractZip(ARM64_ZIP_TEMP_PATH, EXTRACT_PATH);

      // Clean up ARM64 temp file
      fs.unlinkSync(ARM64_ZIP_TEMP_PATH);

      // Download X64 version
      // cjpm.toml 的 target.x86_64-linux-ohos.bin-dependencies 同样引用
      // stdx_bin/linux_ohos_x86_64_cjnative（HAR 默认编译 arm64 + x86_64 两个 ABI），
      // Linux CI 也需要，不能只在 Windows 下载
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
      extractZip(X64_ZIP_TEMP_PATH, EXTRACT_PATH);

      // Clean up X64 temp file
      fs.unlinkSync(X64_ZIP_TEMP_PATH);

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
