// scripts/pre-install.mjs
import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..');

const ARM64_ZIP_URL = 'https://gitcode.com/Cangjie/cangjie-stdx-bin/releases/download/v1.0.1.1/cangjie-stdx-ohos-aarch64-1.0.1.1.zip';
const ARM64_ZIP_TEMP_PATH = path.join(PROJECT_ROOT, 'cangjie-stdx-ohos-aarch64-1.0.1.1.zip');

const X64_ZIP_URL = 'https://gitcode.com/Cangjie/cangjie-stdx-bin/releases/download/v1.0.1.1/cangjie-stdx-ohos-x64-1.0.1.1.zip';
const X64_ZIP_TEMP_PATH = path.join(PROJECT_ROOT, 'cangjie-stdx-linux-x64-1.0.1.1.zip');

function persistEnvVar(name, value) {
    if (process.platform === 'win32') {
        try {
            execSync(`setx ${name} "${value}"`, { stdio: 'ignore' });
            console.log(`Persisted ${name}=${value} to user environment`);
        } catch (e) {
            console.warn(`Failed to persist ${name} to user environment: ${e.message}`);
        }
    }
}

function resolveCangjieStdxPath() {
    let stdxPath = process.env.CANGJIE_STDX_PATH;
    if (stdxPath) {
        console.log(`Using CANGJIE_STDX_PATH: ${stdxPath}`);
        return stdxPath;
    }

    const devecoHome = process.env.DEVECO_CANGJIE_HOME;
    if (devecoHome) {
        console.log(`DEVECO_CANGJIE_HOME found: ${devecoHome}`);
        console.log(`Setting CANGJIE_STDX_PATH to ${devecoHome}`);
        process.env.CANGJIE_STDX_PATH = devecoHome;
        persistEnvVar('CANGJIE_STDX_PATH', devecoHome);
        return devecoHome;
    }

    return null;
}

function hasStdxBinaries(basePath) {
    const arm64Dir = path.join(basePath, 'linux_ohos_aarch64_llvm');
    const x64Dir = path.join(basePath, 'linux_ohos_x86_64_llvm');
    return fs.existsSync(arm64Dir) && fs.existsSync(x64Dir);
}

function downloadAndExtract(label, zipUrl, tempPath, extractPath) {
    console.log(`Downloading ${label} version...`);
    execSync(`curl -L -o ${tempPath} ${zipUrl}`, {
        stdio: 'inherit',
        cwd: PROJECT_ROOT
    });

    if (fs.existsSync(tempPath)) {
        if (fs.statSync(tempPath).size === 0) {
            console.error(`${label} ZIP file is empty, download failed`);
            fs.unlinkSync(tempPath);
            process.exit(1);
        }
    } else {
        console.error(`${label} ZIP file not found`);
        process.exit(1);
    }

    try {
        console.log(`Extracting ${label} version...`);
        if (process.platform === 'win32') {
            execSync(`powershell -Command "Expand-Archive -Path '${tempPath}' -DestinationPath '${extractPath}' -Force"`, {
                stdio: 'inherit',
                cwd: PROJECT_ROOT
            });
        } else {
            execSync(`unzip -o ${tempPath} -d ${extractPath}`, {
                stdio: 'inherit',
                cwd: PROJECT_ROOT
            });
        }
    } finally {
        if (fs.existsSync(tempPath)) {
            fs.unlinkSync(tempPath);
        }
    }
}

const STDX_PATH = resolveCangjieStdxPath();
const EXTRACT_PATH = STDX_PATH || path.join(PROJECT_ROOT, 'stdx_bin');

if (STDX_PATH && hasStdxBinaries(STDX_PATH)) {
    console.log(`CANGJIE_STDX_PATH binaries already exist at ${STDX_PATH}, skipping download.`);
} else {
    const needsDownload = STDX_PATH ? !hasStdxBinaries(STDX_PATH) : !fs.existsSync(EXTRACT_PATH);

    if (needsDownload) {
        console.log(`Downloading required files...`);
        try {
            fs.mkdirSync(EXTRACT_PATH, { recursive: true });

            downloadAndExtract('ARM64', ARM64_ZIP_URL, ARM64_ZIP_TEMP_PATH, EXTRACT_PATH);
            downloadAndExtract('X64', X64_ZIP_URL, X64_ZIP_TEMP_PATH, EXTRACT_PATH);

            console.log(`All files downloaded and extracted successfully to ${EXTRACT_PATH}`);
        } catch (error) {
            console.error(`Download or extraction failed: ${error.message}`);
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
}